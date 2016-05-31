#include "stdafx.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "event_epoll.h"
#include "event.h"

EVENT *event_create(int size)
{
	int i;
	EVENT *ev = event_epoll_create(size);

	ev->events  = (FILE_EVENT *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->defers  = (DEFER_DELETE *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->fired   = (FIRED_EVENT *) acl_mycalloc(size, sizeof(FIRED_EVENT));
	ev->setsize = size;
	ev->maxfd   = -1;
	ev->ndefer  = 0;

	/* Events with mask == AE_NONE are not set. So let's initialize the
	 * vector with it.
	 */
	for (i = 0; i < size; i++)
		ev->events[i].mask = EVENT_NONE;

	return ev;
}

/* Return the current set size. */
int event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
	acl_myfree(ev->events);
	acl_myfree(ev->defers);
	acl_myfree(ev->fired);

	ev->free(ev);
}

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		errno = ERANGE;
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		return -1;
	}

	fe = &ev->events[fd];

	if (fe->defer != NULL) {
		ev->ndefer--;
		ev->defers[fe->defer->pos] = ev->defers[ev->ndefer];
		fe->defer = NULL;
	} else if (ev->add(ev, fd, mask) == -1) {
		acl_msg_error("add fd(%d) error", fd);
		return -1;
	}

	fe->mask |= mask;
	if (mask & EVENT_READABLE)
		fe->r_proc = proc;
	if (mask & EVENT_WRITABLE)
		fe->w_proc = proc;

	fe->ctx = ctx;
	if (fd > ev->maxfd)
		ev->maxfd = fd;

	return 0;
}

static void __event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize)
		return;

	fe = &ev->events[fd];
	fe->defer = NULL;

	if (fe->mask == EVENT_NONE)
		return;

	ev->del(ev, fd, mask);
	fe->mask = fe->mask & (~mask);
	if (fd == ev->maxfd && fe->mask == EVENT_NONE) {
		/* Update the max fd */
		int j;

		for (j = ev->maxfd - 1; j >= 0; j--)
			if (ev->events[j].mask != EVENT_NONE)
				break;
		ev->maxfd = j;
	}
}

void event_del(EVENT *ev, int fd, int mask)
{
	ev->defers[ev->ndefer].fd   = fd;
	ev->defers[ev->ndefer].mask = mask;
	ev->defers[ev->ndefer].pos  = ev->ndefer;
	ev->events[fd].defer        = &ev->defers[ev->ndefer];

	ev->ndefer++;
}

int event_mask(EVENT *ev, int fd)
{
	if (fd >= ev->setsize)
		return 0;

	return ev->events[fd].mask;
}

int event_process(EVENT *ev, acl_int64 left)
{
	int processed = 0, numevents, j;
	struct timeval tv, *tvp;
	FILE_EVENT *fe;
	int mask, fd, rfired;

	if (left < 0) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec  = left / 1000000;
		tv.tv_usec = left % 1000000;
	}

	tvp = &tv;

	for (j = 0; j < ev->ndefer; j++)
		__event_del(ev, ev->defers[j].fd, ev->defers[j].mask);

	ev->ndefer = 0;

	numevents = ev->loop(ev, tvp);

	for (j = 0; j < numevents; j++) {
		fe   = &ev->events[ev->fired[j].fd];
		mask = ev->fired[j].mask;
		fd   = ev->fired[j].fd;

		/* note the fe->mask & mask & ... code: maybe an already
		 * processed event removed an element that fired and we
		 * still didn't processed, so we check if the event is
		 * still valid.
		 */
		if (fe->mask & mask & EVENT_READABLE) {
			rfired = 1;
			fe->r_proc(ev, fd, fe->ctx, mask);
		} else
			rfired = 0;

		if (fe->mask & mask & EVENT_WRITABLE) {
			if (!rfired || fe->w_proc != fe->r_proc)
				fe->w_proc(ev, fd, fe->ctx, mask);
		}

		processed++;
	}

	/* return the number of processed file/time events */
	return processed;
}
