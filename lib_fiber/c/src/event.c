#include "stdafx.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
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
	ev->timeout = -1;
	acl_ring_init(&ev->pevents_list);

	/* Events with mask == AE_NONE are not set. So let's initialize the
	 * vector with it.
	 */
	for (i = 0; i < size; i++) {
		ev->events[i].mask        = EVENT_NONE;
		ev->events[i].mask_fired  = EVENT_NONE;
		ev->events[i].defer       = NULL;
	}

	return ev;
}

/* Return the current set size. */
int event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
	FILE_EVENT *events   = ev->events;
	DEFER_DELETE *defers = ev->defers;
	FIRED_EVENT *fired   = ev->fired;

	ev->free(ev);

	acl_myfree(events);
	acl_myfree(defers);
	acl_myfree(fired);
}

void event_poll(EVENT *ev, POLL_EVENTS *pe, int timeout)
{
	int i;

	acl_ring_prepend(&ev->pevents_list, &pe->me);
	pe->nready = 0;
	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN) {
			event_add(ev, pe->fds[i].fd, EVENT_READABLE, NULL, pe);
			ev->events[pe->fds[i].fd].pevents = pe;
			ev->events[pe->fds[i].fd].pfd = &pe->fds[i];
		}

		if (pe->fds[i].events & POLLOUT) {
			event_add(ev, pe->fds[i].fd, EVENT_WRITABLE, NULL, pe);
			ev->events[pe->fds[i].fd].pevents = pe;
			ev->events[pe->fds[i].fd].pfd = &pe->fds[i];
		}

		pe->fds[i].revents = 0;
	}

	if (timeout > 0) {
		if (ev->timeout < 0 || timeout < ev->timeout)
			ev->timeout = timeout;
	}
}

static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		acl_msg_info("fd: %d fstat error", fd);
		return -1;
	}

	/*
	acl_msg_info("fd: %d, S_ISSOCK: %s, S_ISFIFO: %s, S_ISCHR: %s, "
		"S_ISBLK: %s, S_ISREG: %s", fd,
		S_ISSOCK(s.st_mode) ? "yes" : "no",
		S_ISFIFO(s.st_mode) ? "yes" : "no",
		S_ISCHR(s.st_mode) ? "yes" : "no",
		S_ISBLK(s.st_mode) ? "yes" : "no",
		S_ISREG(s.st_mode) ? "yes" : "no");
	*/

	if (S_ISSOCK(s.st_mode) || S_ISFIFO(s.st_mode) || S_ISCHR(s.st_mode))
		return 0;

	return -1;

}

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return -1;
	}

	fe = &ev->events[fd];

	if (fe->defer != NULL) {
		int fd2, pos = fe->defer->pos;
		int to_mask = mask | (fe->mask & ~(ev->defers[pos].mask));

		assert(to_mask != 0);

		ev->ndefer--;
		fd2 = ev->defers[ev->ndefer].fd;

		if (ev->ndefer > 0) {
			ev->defers[pos].mask  = ev->defers[ev->ndefer].mask;
			ev->defers[pos].pos   = pos;
			ev->defers[pos].fd    = fd2;

			ev->events[fd2].defer = &ev->defers[pos];
		} else {
			if (fd2 >= 0)
				ev->events[fd2].defer = NULL;
			ev->defers[0].mask = EVENT_NONE;
			ev->defers[0].pos  = 0;
		}

		if (ev->add(ev, fd, to_mask) == -1) {
			acl_msg_error("mod fd(%d) error: %s",
				fd, acl_last_serror());
			return -1;
		}

		ev->defers[ev->ndefer].fd  = -1;
		fe->defer = NULL;
		fe->mask  = to_mask;
	} else {
		if (fe->type == TYPE_NONE) {
			if (check_fdtype(fd) < 0) {
				fe->type = TYPE_NOSOCK;
				return 0;
			}

			fe->type = TYPE_SOCK;
		} else if (fe->type == TYPE_NOSOCK)
			return 0;

		if (ev->add(ev, fd, mask) == -1) {
			acl_msg_error("add fd(%d) error: %s",
				fd, acl_last_serror());
			return -1;
		}

		fe->mask |= mask;
	}

	if (mask & EVENT_READABLE)
		fe->r_proc = proc;
	if (mask & EVENT_WRITABLE)
		fe->w_proc = proc;

	fe->ctx     = ctx;
	fe->pevents = NULL;
	fe->pfd     = NULL;

	if (fd > ev->maxfd)
		ev->maxfd = fd;

	return 1;
}

static void __event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	fe             = &ev->events[fd];
	fe->type       = TYPE_NONE;
	fe->defer      = NULL;
	fe->pevents    = NULL;
	fe->pfd        = NULL;
	fe->mask_fired = EVENT_NONE;

	if (fe->mask == EVENT_NONE) {
		acl_msg_info("----mask NONE, fd: %d----", fd);
		return;
	}

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

#define DEL_DELAY

void event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	fe = &ev->events[fd];
	if (fe->type == TYPE_NOSOCK) {
		fe->type = TYPE_NONE;
		return;
	}

#ifdef DEL_DELAY
	if ((mask & EVENT_ERROR) == 0) {
		ev->defers[ev->ndefer].fd   = fd;
		ev->defers[ev->ndefer].mask = mask;
		ev->defers[ev->ndefer].pos  = ev->ndefer;
		ev->events[fd].defer        = &ev->defers[ev->ndefer];

		ev->ndefer++;
		return;
	}
#endif

	if (fe->defer != NULL) {
		int fd2;

		ev->ndefer--;
		fd2 = ev->defers[ev->ndefer].fd;

		if (ev->ndefer > 0) {
			int pos = fe->defer->pos;

			ev->defers[pos].mask  = ev->defers[ev->ndefer].mask;
			ev->defers[pos].pos   = fe->defer->pos;
			ev->defers[pos].fd    = fd2;

			ev->events[fd2].defer = &ev->defers[pos];
		} else {
			if (fd2 >= 0)
				ev->events[fd2].defer = NULL;
			ev->defers[0].mask = EVENT_NONE;
			ev->defers[0].pos = 0;
		}

		ev->defers[ev->ndefer].fd  = -1;
		fe->defer = NULL;
	}

#ifdef DEL_DELAY
	__event_del(ev, fd, fe->mask);
#else
	__event_del(ev, fd, mask);
#endif
}

int event_process(EVENT *ev, int left)
{
	int processed = 0, numevents, j;
	struct timeval tv, *tvp;
	int mask, fd, rfired, ndefer;
	FILE_EVENT *fe;

	if (ev->timeout < 0) {
		if (left < 0) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
		} else {
			tv.tv_sec  = left / 1000;
			tv.tv_usec = (left - tv.tv_sec * 1000) * 1000;
		}
	} else if (left < 0) {
		tv.tv_sec = ev->timeout / 1000;
		tv.tv_usec = (ev->timeout - tv.tv_sec * 1000) * 1000;
	} else if (left < ev->timeout) {
		tv.tv_sec  = left / 1000;
		tv.tv_usec = (left - tv.tv_sec * 1000) * 1000;
	} else {
		tv.tv_sec = ev->timeout / 1000;
		tv.tv_usec = (ev->timeout - tv.tv_sec * 1000) * 1000;
	}

	/* limit the event wait time just for fiber schedule exiting
	 * quickly when no tasks left
	 */
	if (tv.tv_sec > 1)
		tv.tv_sec = 1;

	tvp = &tv;

	ndefer = ev->ndefer;

	for (j = 0; j < ndefer; j++) {
		__event_del(ev, ev->defers[j].fd, ev->defers[j].mask);
		ev->events[ev->defers[j].fd].defer = NULL;
		ev->defers[j].fd = -1;
		ev->ndefer--;
	}

	numevents = ev->loop(ev, tvp);

	for (j = 0; j < numevents; j++) {
		fd             = ev->fired[j].fd;
		mask           = ev->fired[j].mask;
		fe             = &ev->events[fd];
		fe->mask_fired = mask;

		if (fe->pevents != NULL) {
			if (fe->mask & mask & EVENT_READABLE) {
				fe->pfd->revents |= POLLIN;
				fe->pevents->nready++;
			}

			if (fe->mask & mask & EVENT_WRITABLE) {
				fe->pfd->revents |= POLLOUT;
				fe->pevents->nready++;
			}

			continue;
		}

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

	acl_ring_foreach(ev->iter, &ev->pevents_list) {
		POLL_EVENTS *pe = acl_ring_to_appl(ev->iter.ptr,
				POLL_EVENTS, me);

		pe->proc(ev, pe);
		processed++;
	}

	acl_ring_init(&ev->pevents_list);

	/* return the number of processed file/time events */
	return processed;
}

int event_readable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return 0;
	}

	return ev->events[fd].mask_fired & EVENT_READABLE;
}

int event_writeable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return 0;
	}

	return ev->events[fd].mask_fired & EVENT_WRITABLE;
}

void event_clear_readable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	ev->events[fd].mask_fired &= ~EVENT_READABLE;
}

void event_clear_writeable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	ev->events[fd].mask_fired &= ~ EVENT_WRITABLE;
}

void event_clear(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}
	
	ev->events[fd].mask_fired = EVENT_NONE;
}
