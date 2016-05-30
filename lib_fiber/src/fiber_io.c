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

static read_fn   __sys_read   = NULL;
static write_fn  __sys_write  = NULL;
static accept_fn __sys_accept = NULL;
static EVENT    *__event      = NULL;
static FIBER   **__io_fibers  = NULL;
static size_t    __io_count   = 0;
static FIBER    *__ev_fiber   = NULL;

static void fiber_io_loop(void *ctx);

#define MAXFD	1024

void fiber_io_hook(void)
{
	__sys_read   = (read_fn) dlsym(RTLD_NEXT, "read");
	__sys_write  = (write_fn) dlsym(RTLD_NEXT, "write");
	__sys_accept = (accept_fn) dlsym(RTLD_NEXT, "accept");

	__event = event_create(MAXFD);
	__io_fibers = (FIBER **) acl_mycalloc(MAXFD, sizeof(FIBER *));
}

static void fiber_io_loop(void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	int  n;

	fiber_system();

	for (;;) {
		while ((n = fiber_yield()) > 0) {
			// do nothing;
		}

		event_process(ev);
	}
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
		__ev_fiber = fiber_create(fiber_io_loop, __event, 32768);

	event_add(__event, fd, EVENT_READABLE, accept_callback, NULL);

	__io_fibers[fd] = fiber_running();
	__io_count++;
	fiber_switch();
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int   clifd;

	fiber_wait_accept(sockfd);

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	return -1;
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
