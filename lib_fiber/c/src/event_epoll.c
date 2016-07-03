#include "stdafx.h"
#define __USE_GNU
#include <dlfcn.h>
#include <sys/epoll.h>
#include "fiber.h"
#include "event.h"
#include "event_epoll.h"

typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);

static epoll_wait_fn __sys_epoll_wait = NULL;
static epoll_ctl_fn  __sys_epoll_ctl  = NULL;

void hook_epoll(void)
{
	static int __called = 0;

	if (__called)
		return;

	__called++;

	__sys_epoll_wait = (epoll_wait_fn) dlsym(RTLD_NEXT, "epoll_wait");
	__sys_epoll_ctl  = (epoll_ctl_fn) dlsym(RTLD_NEXT, "epoll_ctl");
}

typedef struct EVENT_EPOLL {
	EVENT event;
	int epfd;
	struct epoll_event *epoll_events;
} EVENT_EPOLL;

static void epoll_event_free(EVENT *ev)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

	close(ep->epfd);
	acl_myfree(ep->epoll_events);
	acl_myfree(ep);
}

static int epoll_event_add(EVENT *ev, int fd, int mask)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	struct epoll_event ee;
	int  op;

	if ((ev->events[fd].mask & mask) == mask)
		return 0;

	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	op = ev->events[fd].mask == EVENT_NONE
		? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.ptr = NULL;
	ee.data.fd  = fd;

	mask |= ev->events[fd].mask; /* Merge old events */
	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;

#if 0
#ifdef	EPOLLRDHUP
	ee.events |= EPOLLERR | EPOLLRDHUP | EPOLLHUP;
#else
	ee.events |= EPOLLERR | EPOLLHUP;
#endif
#endif

	if (__sys_epoll_ctl(ep->epfd, op, fd, &ee) == -1) {
		fiber_save_errno();
		acl_msg_error("%s, %s(%d): epoll_ctl error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

static void epoll_event_del(EVENT *ev, int fd, int delmask)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	struct epoll_event ee;
	int mask = ev->events[fd].mask & (~delmask);

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.ptr = NULL;
	ee.data.fd  = fd;

	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;

	if (mask != EVENT_NONE) {
		if (__sys_epoll_ctl(ep->epfd, EPOLL_CTL_MOD, fd, &ee) < 0) {
			fiber_save_errno();
			acl_msg_error("%s(%d), epoll_ctl error: %s, fd: %d",
				__FUNCTION__, __LINE__, acl_last_serror(), fd);
		}
	} else {
		/* Note, Kernel < 2.6.9 requires a non null event pointer
		 * even for EPOLL_CTL_DEL.
		 */
		if (__sys_epoll_ctl(ep->epfd, EPOLL_CTL_DEL, fd, &ee) < 0) {
			fiber_save_errno();
			acl_msg_error("%s(%d), epoll_ctl error: %s, fd: %d",
				__FUNCTION__, __LINE__, acl_last_serror(), fd);
		}
	}
}

static int epoll_event_loop(EVENT *ev, struct timeval *tv)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	int retval, j, mask;
	struct epoll_event *e;

	retval = __sys_epoll_wait(ep->epfd, ep->epoll_events, ev->setsize,
			tv ? (tv->tv_sec * 1000 + tv->tv_usec / 1000) : -1);

	if (retval <= 0)
		return retval;

	for (j = 0; j < retval; j++) {
		mask = 0;
		e = ep->epoll_events + j;

		if (e->events & EPOLLIN)
			mask |= EVENT_READABLE;
		if (e->events & EPOLLOUT)
			mask |= EVENT_WRITABLE;
		if (e->events & EPOLLERR || e->events & EPOLLHUP ) {
			if (ev->events[e->data.fd].mask & EVENT_READABLE)
				mask |= EVENT_READABLE;
			if (ev->events[e->data.fd].mask & EVENT_WRITABLE)
				mask |= EVENT_WRITABLE;
		}

		ev->fired[j].fd = e->data.fd;
		ev->fired[j].mask = mask;
	}

	return retval;
}

static const char *epoll_event_name(void)
{
	return "epoll";
}

EVENT *event_epoll_create(int setsize)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) acl_mymalloc(sizeof(EVENT_EPOLL));

	ep->epoll_events = (struct epoll_event *)
		acl_mymalloc(sizeof(struct epoll_event) * setsize);

	ep->epfd = epoll_create(1024);
	acl_assert(ep->epfd >= 0);

	ep->event.name = epoll_event_name;
	ep->event.loop = epoll_event_loop;
	ep->event.add  = epoll_event_add;
	ep->event.del  = epoll_event_del;
	ep->event.free = epoll_event_free;

	return (EVENT*) ep;
}
