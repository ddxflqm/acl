#include "stdafx.h"
#include <fcntl.h>
#define __USE_GNU
#include <dlfcn.h>
#include <sys/stat.h>
#include "event.h"
#include "fiber_schedule.h"
#include "fiber_io.h"

typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*write_fn)(int, const void *, size_t);
typedef int (*accept_fn)(int, struct sockaddr *, socklen_t *);

static read_fn    __sys_read   = NULL;
static write_fn   __sys_write  = NULL;
static accept_fn  __sys_accept = NULL;
static EVENT     *__event      = NULL;
static FIBER    **__io_fibers  = NULL;
static size_t     __io_count   = 0;
static FIBER     *__ev_fiber   = NULL;
static ACL_RING   __ev_timer;
static int        __sleeping_count;

static void fiber_io_loop(void *ctx);

#define MAXFD		1024
#define STACK_SIZE	819200

void fiber_io_hook(void)
{
	__sys_read   = (read_fn) dlsym(RTLD_NEXT, "read");
	__sys_write  = (write_fn) dlsym(RTLD_NEXT, "write");
	__sys_accept = (accept_fn) dlsym(RTLD_NEXT, "accept");

	__event      = event_create(MAXFD);
	__io_fibers  = (FIBER **) acl_mycalloc(MAXFD, sizeof(FIBER *));
	acl_ring_init(&__ev_timer);
}

#define RING_TO_FIBER(r) \
	((FIBER *) ((char *) (r) - offsetof(FIBER, me)))

#define FIRST_FIBER(head) \
	(acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

#define SET_TIME(x) {  \
	gettimeofday(&tv, NULL);  \
	(x) = ((acl_int64) tv.tv_sec) * 1000000 + ((acl_int64) tv.tv_usec); \
}

static void fiber_io_loop(void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	acl_int64 timer_left;
	FIBER *fiber;
	acl_int64 now, last = 0;
	struct timeval tv;

	fiber_system();

	for (;;) {
		while (fiber_yield() > 0) {}

		fiber = FIRST_FIBER(&__ev_timer);

		if (fiber == NULL)
			timer_left = -1;
		else {
			SET_TIME(now);
			last = now;
			if (now >= fiber->when)
				timer_left = 0;
			else
				timer_left = fiber->when - now;
		}

		/* add 1000 just for the deviation of epoll_wait */
		event_process(ev, timer_left > 0 ?
			timer_left + 1000 : timer_left);

		if (fiber == NULL)
			continue;

		SET_TIME(now);

		if (now - last < timer_left)
			continue;

		do {
			acl_ring_detach(&fiber->me);

			if (!fiber->sys && --__sleeping_count == 0)
				fiber_count_dec();

			fiber_ready(fiber);

			fiber = FIRST_FIBER(&__ev_timer);
		} while (fiber != NULL && now >= fiber->when);
	}
}

acl_int64 fiber_delay(acl_int64 n)
{
	acl_int64 when, now;
	struct timeval tv;
	FIBER *fiber, *next = NULL;
	ACL_RING_ITER iter;

	if (__ev_fiber == NULL)
		__ev_fiber = fiber_create(fiber_io_loop, __event, STACK_SIZE);

	SET_TIME(when);
	when += n;

	acl_ring_foreach(iter, &__ev_timer) {
		fiber = acl_ring_to_appl(iter.ptr, FIBER, me);
		if (fiber->when >= when) {
			next = fiber;
			break;
		}
	}

	fiber = fiber_running();
	fiber->when = when;
	acl_ring_detach(&fiber->me);

	if (next)
		acl_ring_prepend(&next->me, &fiber->me);
	else
		acl_ring_prepend(&__ev_timer, &fiber->me);

	if (!fiber->sys && __sleeping_count++ == 0)
		fiber_count_inc();

	fiber_switch();

	SET_TIME(now);

	now -= when;
	return now < 0 ? 0 : now;
}

unsigned int sleep(unsigned int seconds)
{
	return fiber_delay(seconds * 1000000);
}

static void accept_callback(EVENT *ev, int fd, void *ctx acl_unused, int mask)
{
	event_del(ev, fd, mask);
	fiber_ready(__io_fibers[fd]);

	__io_count--;
	__io_fibers[fd] = __io_fibers[__io_count];
}

static void fiber_wait_accept(int fd)
{
	if (__ev_fiber == NULL)
		__ev_fiber = fiber_create(fiber_io_loop, __event, STACK_SIZE);

	event_add(__event, fd, EVENT_READABLE, accept_callback, NULL);

	__io_fibers[fd] = fiber_running();
	__io_count++;

	fiber_switch();
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int   clifd;

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	while (1) {
		clifd = __sys_accept(sockfd, addr, addrlen);

		if (clifd >= 0) {
			acl_non_blocking(clifd, ACL_NON_BLOCKING);
			acl_tcp_nodelay(clifd, 1);
			return clifd;
		}

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_accept(sockfd);
	}
}

static void read_callback(EVENT *ev, int fd, void *ctx acl_unused, int mask)
{
	event_del(ev, fd, mask);
	fiber_ready(__io_fibers[fd]);

	__io_count--;
	__io_fibers[fd] = __io_fibers[__io_count];
}

static void fiber_wait_read(int fd)
{
	if (__ev_fiber == NULL)
		__ev_fiber = fiber_create(fiber_io_loop, __event, STACK_SIZE);

	event_add(__event, fd, EVENT_READABLE, read_callback, NULL);

	__io_fibers[fd] = fiber_running();
	__io_count++;

	fiber_switch();
}

ssize_t read(int fd, void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_read(fd, buf, count);

		if (n >= 0)
			return n;

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(fd);
	}
}

static void write_callback(EVENT *ev, int fd, void *ctx acl_unused, int mask)
{
	event_del(ev, fd, mask);
	fiber_ready(__io_fibers[fd]);

	__io_count--;
	__io_fibers[fd] = __io_fibers[__io_count];
}

static void fiber_wait_write(int fd)
{
	if (__ev_fiber == NULL)
		__ev_fiber = fiber_create(fiber_io_loop, __event, STACK_SIZE);

	event_add(__event, fd, EVENT_WRITABLE, write_callback, NULL);

	__io_fibers[fd] = fiber_running();
	__io_count++;

	fiber_switch();
}

ssize_t write(int fd, const void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_write(fd, buf, count);

		if (n >= 0)
			return n;

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(fd);
	}
}
