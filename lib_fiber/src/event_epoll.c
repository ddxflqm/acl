#include "stdafx.h"
#include <sys/epoll.h>
#include "event.h"
#include "event_epoll.h"

typedef struct EVENT_EPOLL {
	EVENT event;
	int epfd;
	struct epoll_event *events;
} EVENT_EPOLL;

static void event_free(EVENT *event)
{
	EVENT_EPOLL *ee = (EVENT_EPOLL *) event;

	close(ee->epfd);
	acl_myfree(ee->events);
	acl_myfree(ee);
}

static int event_add(EVENT *event, int fd, int mask)
{
	EVENT_EPOLL *ee = (EVENT_EPOLL *) event;
	struct epoll_event ee = {0}; /* avoid valgrind warning */
	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	int op = ee->events[fd].mask == EVENT_NONE ?
		EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= ee->events[fd].mask; /* Merge old events */
	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.fd = fd;

	if (epoll_ctl(ee->epfd, op, fd, &ee) == -1) {
		acl_msg_error("%s, %s(%%d): epoll_ctl error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

static void event_del(EVENT *event, int fd, int delmask)
{
	EVENT_EPOLL *ee = (EVENT_EPOLL *) event;
	struct epoll_event ee = {0}; /* avoid valgrind warning */
	int mask = ee->events[fd].mask & (~delmask);

	ee.events = 0;
	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.fd = fd;

	if (mask != EVENT_NONE)
		epoll_ctl(ee->epfd, EPOLL_CTL_MOD, fd, &ee);
	else {
		/* Note, Kernel < 2.6.9 requires a non null event pointer
		 * even for EPOLL_CTL_DEL.
		 */
		epoll_ctl(ee->epfd, EPOLL_CTL_DEL, fd, &ee);
	}
}

static int event_loop(EVENT *event, struct timeval *tvp)
{
	EVENT_EPOLL *ee = (EVENT_EPOLL *) event;
	int retval, numevents = 0;

	retval = epoll_wait(ee->epfd, ee->events, event->setsize,
			tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
	if (retval > 0) {
		int j, mask;
		struct epoll_event *e;

		numevents = retval;
		for (j = 0; j < numevents; j++) {
			mask = 0;
			e = state->events + j;

			if (e->events & EPOLLIN)
				mask |= EVENT_READABLE;
			if (e->events & EPOLLOUT)
				mask |= EVENT_WRITABLE;
			if (e->events & EPOLLERR)
				mask |= EVENT_WRITABLE;
			if (e->events & EPOLLHUP)
				mask |= EVENT_WRITABLE;

			event->fired[j].fd = e->data.fd;
			event->fired[j].mask = mask;
		}
	}

	return numevents;
}

static const char *event_name(void)
{
	return "epoll";
}

EVENT *event_epoll_create(int setsize)
{
	EVENT_EPOLL *ee = (EVENT_EPOLL *) acl_mymalloc(sizeof(EVENT_EPOLL));

	ee->events = (struct EVENT_EPOLL *)
		acl_mymalloc(sizeof(struct epoll_event) * setsize);

	ee->epfd = epoll_create(1024);
	acl_assert(ee->epfd >= 0);

	ee->event.name = event_name;
	ee->event.loop = event_loop;
	ee->event.add  = event_add;
	ee->event.del  = event_del;
	ee->event.free = event_free;

	return ee;
}
