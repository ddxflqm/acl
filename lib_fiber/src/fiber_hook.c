#include "stdafx.h"
#include <fcntl.h>
#define __USE_GNU
#include <dlfcn.h>
#include <sys/stat.h>
#include "acl_hook.h"

typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*write_fn)(int, const void *, size_t);

static read_fn  sys_read  = NULL;
static write_fn sys_write = NULL;

#ifdef ACL_UNIX
void fiber_sys_hook(void) __attribute__ ((constructor));
#endif

void fiber_sys_hook(void)
{
	sys_read  = (read_fn) dlsym(RTLD_NEXT, "read");
	sys_write = (write_fn) dlsym(RTLD_NEXT, "write");
}

ssize_t read(int fd, void *buf, size_t count)
{
	ssize_t n = sys_read(fd, buf, count);

	printf("ok, read n: %d\r\n", (int) n);

	if (n >= 0)
		return n;

#if EAGAIN == EWOULDBLOCK
	if (errno != EAGAIN)
#else
	if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
		return -1;

	return n;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	ssize_t n = sys_write(fd, buf, count);

	printf("ok, write n: %d\r\n", (int) n);
	if (n >= 0)
		return n;

#if EAGAIN == EWOULDBLOCK
	if (errno != EAGAIN)
#else
	if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
		return -1;

	return n;
}
