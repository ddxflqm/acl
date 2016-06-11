#include "stdafx.h"

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "fiber/fiber_io.h"
#include "fiber/fiber_schedule.h"
#include "fiber.h"

typedef struct {
	ACL_RING queue;
	ACL_RING dead;     //dead fiber queue
	FIBER  **fibers;
	size_t   size;
	int      exitcode;
	FIBER   *running;
	FIBER    schedule;
	size_t   idgen;
	int      count;
	int      switched;
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;

static acl_pthread_key_t __fiber_key;

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL)
		return;

	acl_myfree(tf->fibers);
	acl_myfree(tf);
	if (__main_fiber == __thread_fiber)
		__main_fiber = NULL;
	__thread_fiber = NULL;
}

static void fiber_schedule_main_free(void)
{
	if (__main_fiber) {
		thread_free(__main_fiber);
		if (__thread_fiber == __main_fiber)
			__thread_fiber = NULL;
		__main_fiber = NULL;
	}
}

static void thread_init(void)
{
	acl_assert(acl_pthread_key_create(&__fiber_key, thread_free) == 0);
}

static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void fiber_check(void)
{
	if (__thread_fiber != NULL)
		return;

	acl_assert(acl_pthread_once(&__once_control, thread_init) == 0);

	__thread_fiber = (FIBER_TLS *) acl_mycalloc(1, sizeof(FIBER_TLS));
	__thread_fiber->fibers = NULL;
	__thread_fiber->size   = 0;
	__thread_fiber->idgen  = 0;
	__thread_fiber->count  = 0;
	acl_ring_init(&__thread_fiber->queue);
	acl_ring_init(&__thread_fiber->dead);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

static void fiber_swap(FIBER *from, FIBER *to)
{
	if (swapcontext(&from->uctx, &to->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

FIBER *fiber_running(void)
{
	fiber_check();
	return __thread_fiber->running;
}

void fiber_exit(int exit_code)
{
	fiber_check();

	__thread_fiber->exitcode = exit_code;
	__thread_fiber->running->status = FIBER_STATUS_EXITING;

	fiber_switch();
}

union cc_arg
{
	void *p;
	int   i[2];
};

static void fiber_start(unsigned int x, unsigned int y)
{
	union cc_arg arg;
	FIBER *fiber;

	arg.i[0] = x;
	arg.i[1] = y;
	
	fiber = (FIBER *) arg.p;

	fiber->fn(fiber, fiber->arg);
	fiber_exit(0);
}

static FIBER *fiber_alloc(void (*fn)(FIBER *, void *), void *arg, size_t size)
{
	FIBER *fiber;
	sigset_t zero;
	union cc_arg carg;
	ACL_RING *head;

	fiber_check();

	head = acl_ring_pop_head(&__thread_fiber->dead);
	if (head == NULL) {
		fiber = (FIBER *) acl_mycalloc(1, sizeof(FIBER) + size);
	} else if ((fiber = ACL_RING_TO_APPL(head, FIBER, me))->size < size) {
		fiber_free(fiber);
		fiber = (FIBER *) acl_mycalloc(1, sizeof(FIBER) + size);
	} else
		size = fiber->size;

	fiber->fn    = fn;
	fiber->arg   = arg;
	fiber->stack = fiber->buf;
	fiber->size  = size;
	fiber->id    = ++__thread_fiber->idgen;

	sigemptyset(&zero);
	sigprocmask(SIG_BLOCK, &zero, &fiber->uctx.uc_sigmask);

	if (getcontext(&fiber->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	fiber->uctx.uc_stack.ss_sp   = fiber->stack + 8;
	fiber->uctx.uc_stack.ss_size = fiber->size - 64;
	fiber->uctx.uc_link = &__thread_fiber->schedule.uctx;

#ifdef USE_VALGRIND
	fiber->vid = VALGRIND_STACK_REGISTER(fiber->uctx.uc_stack.ss_sp,
			fiber->uctx.uc_stack.ss_sp
			+ fiber->uctx.uc_stack.ss_size);
#endif

	carg.p = fiber;
	makecontext(&fiber->uctx, (void(*)(void)) fiber_start,
		2, carg.i[0], carg.i[1]);

	return fiber;
}

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size)
{
	FIBER *fiber = fiber_alloc(fn, arg, size);

	__thread_fiber->count++;
	if (__thread_fiber->size % 64 == 0)
		__thread_fiber->fibers = (FIBER **) acl_myrealloc(
			__thread_fiber->fibers, 
			(__thread_fiber->size + 64) * sizeof(FIBER *));

	fiber->slot = __thread_fiber->size;
	__thread_fiber->fibers[__thread_fiber->size++] = fiber;
	fiber_ready(fiber);

	return fiber;
}

void fiber_free(FIBER *fiber)
{
	acl_myfree(fiber);
}

int fiber_id(const FIBER *fiber)
{
	return fiber->id;
}

void fiber_init(void) __attribute__ ((constructor));

void fiber_init(void)
{
	static int __called = 0;

	if (__called != 0)
		return;

	__called++;
	fiber_io_hook();
	fiber_net_hook();
}

void fiber_schedule(void)
{
	FIBER *fiber;
	ACL_RING *head;

	for (;;) {
		head = acl_ring_pop_head(&__thread_fiber->queue);
		if (head == NULL) {
			printf("------- NO FIBER NOW --------\r\n");
			break;
		}

		fiber = ACL_RING_TO_APPL(head, FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(&__thread_fiber->schedule, fiber);
		__thread_fiber->running = NULL;
	}

	// release dead fiber 
	for (;;) {
		head = acl_ring_pop_head(&__thread_fiber->dead);
		if (head == NULL)
			break;

		fiber = ACL_RING_TO_APPL(head, FIBER, me);
		fiber_free (fiber);
	}
}

void fiber_ready(FIBER *fiber)
{
	fiber->status = FIBER_STATUS_READY;
	acl_ring_prepend(&__thread_fiber->queue, &fiber->me);
}

int fiber_yield(void)
{
	int  n = __thread_fiber->switched;

	fiber_ready(__thread_fiber->running);
	fiber_switch();

	return __thread_fiber->switched - n - 1;
}

void fiber_system(void)
{
	if (!__thread_fiber->running->sys) {
		__thread_fiber->running->sys = 1;
		__thread_fiber->count--;
	}
}

void fiber_count_inc(void)
{
	__thread_fiber->count++;
}

void fiber_count_dec(void)
{
	__thread_fiber->count--;
}

void fiber_switch(void)
{
	FIBER *fiber, *current = __thread_fiber->running;
	ACL_RING *head;

#ifdef _DEBUG
	acl_assert(current);
#endif

	if (current->status == FIBER_STATUS_EXITING) {
		size_t slot = current->slot;

		if (!current->sys)
			__thread_fiber->count--;

		__thread_fiber->fibers[slot] =
			__thread_fiber->fibers[--__thread_fiber->size];
		__thread_fiber->fibers[slot]->slot = slot;
		acl_ring_append(&__thread_fiber->dead, &current->me);
	}

	head = acl_ring_pop_head(&__thread_fiber->queue);

	if (head == NULL) {
		fiber_swap(current, &__thread_fiber->schedule);
		return;
	}

	fiber = ACL_RING_TO_APPL(head, FIBER, me);
	fiber->status = FIBER_STATUS_READY;

	__thread_fiber->running = fiber;
	__thread_fiber->switched++;
	fiber_swap(current, __thread_fiber->running);
}
