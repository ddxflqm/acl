#include "stdafx.h"
#include <poll.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#define __USE_GNU
#include <dlfcn.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int (*socket_fn)(int, int, int);
typedef int (*socketpair_fn)(int, int, int, int sv[2]);
typedef int (*bind_fn)(int, const struct sockaddr *, socklen_t);
typedef int (*listen_fn)(int, int);
typedef int (*accept_fn)(int, struct sockaddr *, socklen_t *);
typedef int (*connect_fn)(int, const struct sockaddr *, socklen_t);

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
typedef int (*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
	size_t, struct hostent **, int *);

typedef int (*epoll_create_fn)(int);
typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);

static socket_fn     __sys_socket   = NULL;
static socketpair_fn __sys_socketpair = NULL;
static bind_fn       __sys_bind     = NULL;
static listen_fn     __sys_listen   = NULL;
static accept_fn     __sys_accept   = NULL;
static connect_fn    __sys_connect  = NULL;

static poll_fn       __sys_poll     = NULL;
static select_fn     __sys_select   = NULL;
static gethostbyname_r_fn __sys_gethostbyname_r = NULL;

static epoll_create_fn __sys_epoll_create = NULL;
static epoll_wait_fn   __sys_epoll_wait   = NULL;
static epoll_ctl_fn    __sys_epoll_ctl    = NULL;

void hook_net(void)
{
	static int __called = 0;

	if (__called)
		return;

	__called++;

	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	__sys_socketpair = (socketpair_fn) dlsym(RTLD_NEXT, "socketpair");
	__sys_bind       = (bind_fn) dlsym(RTLD_NEXT, "bind");
	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");

	__sys_poll       = (poll_fn) dlsym(RTLD_NEXT, "poll");
	__sys_select     = (select_fn) dlsym(RTLD_NEXT, "select");
	__sys_gethostbyname_r = (gethostbyname_r_fn) dlsym(RTLD_NEXT,
			"gethostbyname_r");

	__sys_epoll_create = (epoll_create_fn) dlsym(RTLD_NEXT, "epoll_create");
	__sys_epoll_wait   = (epoll_wait_fn) dlsym(RTLD_NEXT, "epoll_wait");
	__sys_epoll_ctl    = (epoll_ctl_fn) dlsym(RTLD_NEXT, "epoll_ctl");
}

int socket(int domain, int type, int protocol)
{
	int sockfd = __sys_socket(domain, type, protocol);

	if (!acl_var_hook_sys_api)
		return sockfd;

	if (sockfd >= 0)
		acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	else
		fiber_save_errno();
	return sockfd;
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
	int ret = __sys_socketpair(domain, type, protocol, sv);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (__sys_bind(sockfd, addr, addrlen) == 0)
		return 0;

	if (!acl_var_hook_sys_api)
		return -1;

	fiber_save_errno();
	return -1;
}

int listen(int sockfd, int backlog)
{
	if (!acl_var_hook_sys_api)
		return __sys_listen(sockfd, backlog);

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	if (__sys_listen(sockfd, backlog) == 0)
		return 0;

	fiber_save_errno();
	return -1;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int   clifd;

	if (!acl_var_hook_sys_api)
		return __sys_accept(sockfd, addr, addrlen);

	fiber_wait_read(sockfd);
	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err;
	socklen_t len = sizeof(err);

	if (!acl_var_hook_sys_api)
		return __sys_connect(sockfd, addr, addrlen);

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	int ret = __sys_connect(sockfd, addr, addrlen);
	if (ret >= 0) {
		acl_tcp_nodelay(sockfd, 1);
		return ret;
	}

	fiber_save_errno();

	if (errno != EINPROGRESS)
		return -1;

	fiber_wait_write(sockfd);

	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);

	if (ret == 0 && err == 0)
		return 0;

	acl_set_error(err);

	acl_msg_error("%s(%d): getsockopt error: %s, ret: %d, err: %d",
		__FUNCTION__, __LINE__, acl_last_serror(), ret, err);

	return -1;
}

/****************************************************************************/

static void poll_callback(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN)
			event_del(ev, pe->fds[i].fd, EVENT_READABLE);
		if (pe->fds[i].events & POLLOUT)
			event_del(ev, pe->fds[i].fd, EVENT_WRITABLE);

		fiber_io_dec();
	}

	acl_fiber_ready(pe->fiber);
}

#define SET_TIME(x) do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	(x) = ((acl_int64) tv.tv_sec) * 1000 + ((acl_int64) tv.tv_usec)/ 1000; \
} while (0)

static void event_poll_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
	int i;

	acl_ring_prepend(&ev->poll_list, &pe->me);
	pe->nready = 0;

	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN) {
			event_add(ev, pe->fds[i].fd, EVENT_READABLE, NULL, NULL);
			ev->events[pe->fds[i].fd].pe    = pe;
			ev->events[pe->fds[i].fd].v.pfd = &pe->fds[i];
		}

		if (pe->fds[i].events & POLLOUT) {
			event_add(ev, pe->fds[i].fd, EVENT_WRITABLE, NULL, NULL);
			ev->events[pe->fds[i].fd].pe    = pe;
			ev->events[pe->fds[i].fd].v.pfd = &pe->fds[i];
		}

		pe->fds[i].revents = 0;
	}

	if (timeout > 0) {
		if (ev->timeout < 0 || timeout < ev->timeout)
			ev->timeout = timeout;
	}
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	POLL_EVENT pe;
	EVENT *event;
	acl_int64 begin, now;

	if (!acl_var_hook_sys_api)
		return __sys_poll(fds, nfds, timeout);

	fiber_io_check();

	event     = fiber_io_event();

	pe.fds    = fds;
	pe.nfds   = nfds;
	pe.fiber  = fiber_running();
	pe.proc   = poll_callback;
	pe.nready = 0;

	SET_TIME(begin);

	while (1) {
		event_poll_set(event, &pe, timeout);
		fiber_io_inc();
		acl_fiber_switch();

		if (pe.nready != 0)
			break;

		SET_TIME(now);
		if (now - begin >= timeout)
			break;
	}

	return pe.nready;
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout)
{
	struct pollfd *fds;
	int fd, timo, n, nready = 0;

	if (!acl_var_hook_sys_api)
		return __sys_select(nfds, readfds, writefds,
				exceptfds, timeout);

	fds = (struct pollfd *) acl_mycalloc(nfds + 1, sizeof(struct pollfd));

	for (fd = 0; fd < nfds; fd++) {
		if (readfds && FD_ISSET(fd, readfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLIN;
		}

		if (writefds && FD_ISSET(fd, writefds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLOUT;
		}

		if (exceptfds && FD_ISSET(fd, exceptfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLERR | POLLHUP;
		}
	}

	if (timeout != NULL)
		timo = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
	else
		timo = -1;

	n = poll(fds, nfds, timo);

	if (readfds)
		FD_ZERO(readfds);
	if (writefds)
		FD_ZERO(writefds);
	if (exceptfds)
		FD_ZERO(exceptfds);

	for (fd = 0; fd < nfds && nready < n; fd++) {
		if (fds[fd].fd < 0 || fds[fd].fd != fd)
			continue;

		if (readfds && (fds[fd].revents & POLLIN)) {
			FD_SET(fd, readfds);
			nready++;
		}
		if (writefds && (fds[fd].revents & POLLOUT)) {
			FD_SET(fd, writefds);
			nready++;
		}
		if (exceptfds && (fds[fd].revents & (POLLERR | POLLHUP))) {
			FD_SET(fd, exceptfds);
			nready++;
		}
	}

	acl_myfree(fds);

	return nready;
}

/****************************************************************************/

static void epfd_create(EPOLL_EVENT *ee)
{
	int  maxfd = acl_open_limit(0), i;

	if (maxfd <= 0)
		acl_msg_fatal("%s(%d), %s: acl_open_limit error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	++maxfd;
	ee->fds  = (EPOLL_CTX *) acl_mymalloc(maxfd * sizeof(EPOLL_CTX));
	ee->nfds = maxfd;

	for (i = 0; i < maxfd; i++)
		ee->fds[i].fd = -1;
}

static void epfd_reset(EPOLL_EVENT *ee)
{
	size_t i;

	for (i = 0; i < ee->nfds; i++)
		ee->fds[i].fd = -1;
}

static EPOLL_EVENT *__main_epfds = NULL;
static __thread EPOLL_EVENT *__epfds = NULL;
static __thread int __nepfds;

static acl_pthread_key_t  __once_key;
static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void thread_free(void *ctx acl_unused)
{
	int  i;

	if (__epfds == NULL)
		return;

	if (__epfds == __main_epfds)
		__main_epfds = NULL;

	for (i = 0; i < __nepfds; i++)
		acl_myfree(__epfds[i].fds);
	acl_myfree(__epfds);
	__epfds = NULL;
}

static void main_thread_free(void)
{
	if (__main_epfds) {
		thread_free(__main_epfds);
		__main_epfds = NULL;
	}
}

static void thread_init(void)
{
	acl_assert(acl_pthread_key_create(&__once_key, thread_free) == 0);
}

static void check_epfds(void)
{ 
	int i;

	if (__epfds != NULL)
		return;

	acl_assert(acl_pthread_once(&__once_control, thread_init) == 0);

	__nepfds = acl_open_limit(0);
	if (__nepfds <= 0)
		acl_msg_fatal("%s(%d), %s: acl_open_limit error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	__nepfds++;
	__epfds = (EPOLL_EVENT *) acl_mycalloc(__nepfds, sizeof(EPOLL_EVENT));
	printf("---%s--%d, ne: %d---\r\n", __FUNCTION__, __LINE__,
			__nepfds);
	for (i = 0; i < __nepfds; i++)
	{
		printf("---%s--%d, i: %d---\r\n", __FUNCTION__, __LINE__, i);
		epfd_create(&__epfds[i]);
	}

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_epfds = __epfds;
		atexit(main_thread_free);
	} else if (acl_pthread_setspecific(__once_key, __epfds) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

int epoll_create(int size acl_unused)
{
	EVENT *ev;
	int epfd;

	fiber_io_check();
	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: create_event failed %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	epfd = event_handle(ev);
	if (epfd < 0) {
		acl_msg_error("%s(%d), %s: invalid event_handle %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return epfd;
	}

	epfd = dup(epfd);
	if (epfd < 0)
		acl_msg_error("%s(%d), %s: dup epfd %d error %s", __FILE__,
			__LINE__, __FUNCTION__, epfd, acl_last_serror());

	check_epfds();
	if (epfd >= __nepfds)
		acl_msg_fatal("%s(%d), %s: epfd: %d >= __nepfds: %d",
			__FILE__, __LINE__, __FUNCTION__, epfd, __nepfds);

	epfd_reset(&__epfds[epfd]);
	return epfd;
}

static void epfd_callback(EVENT *ev acl_unused, int fd, void *ctx, int mask)
{
	int  n;
	EPOLL_EVENT *ee = (EPOLL_EVENT *) ctx;

	printf("epfd_callback called!\r\n");

	for (; ee->nevents < ee->maxevents;) {
		n = 0;
		if (mask & EVENT_READABLE) {
			ee->events[ee->nevents].events = EPOLLIN;
			n++;
		}

		if (mask & EVENT_WRITABLE) {
			ee->events[ee->nevents].events = EPOLLOUT;
			n++;
		}

		if (n == 0) /* xxx */
			continue;

		memcpy(&ee->events[ee->nevents].data, &ee->fds[fd].data,
			sizeof(ee->fds[fd].data));

		ee->nevents++;

		fiber_io_dec();
	}
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	EVENT *ev;
	int    mask = 0;

	printf("---%s--%d---\r\n", __FUNCTION__, __LINE__);

	if (epfd < 0) {
		acl_msg_error("%s(%d), %s: invalid epfd %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	if (__epfds == NULL) {
		acl_msg_error("%s(%d), %s: call epoll_create first",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else if (epfd >= __nepfds) {
		acl_msg_error("%s(%d), %s: too large epfd %d >= __nepfds %d",
			__FILE__, __LINE__, __FUNCTION__, epfd, __nepfds);
		return -1;
	}

	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: EVENT NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (event->events & EPOLLIN)
		mask |= EVENT_READABLE;
	if (event->events & EPOLLOUT)
		mask |= EVENT_WRITABLE;

	if (op == EPOLL_CTL_ADD && op == EPOLL_CTL_MOD) {
		EPOLL_EVENT *ee = &__epfds[epfd];

		ee->fds[fd].fd    = fd;
		ee->fds[fd].op    = op;
		ee->fds[fd].mask  = mask;
		ee->fds[fd].rmask = EVENT_NONE;
		memcpy(&ee->fds[fd].data, &event->data, sizeof(event->data));

		ev->events[fd].ee    = ee;
		ev->events[fd].v.epx = &ee->fds[fd];

		return event_add(ev, fd, mask, epfd_callback, ee);
	} else if (op == EPOLL_CTL_DEL) {
		EPOLL_EVENT *ee = &__epfds[epfd];

		event_del(ev, fd, mask);
		ee->fds[fd].fd    = -1;
		ee->fds[fd].op    = 0;
		ee->fds[fd].mask  = EVENT_NONE;
		ee->fds[fd].rmask = EVENT_NONE;
		memset(&ee->fds[fd].data, 0, sizeof(ee->fds[fd].data));

		return 0;
	} else {
		acl_msg_error("%s(%d), %s: invalid op %d",
			__FILE__, __LINE__, __FUNCTION__, op);
		return -1;
	}
}

static void epoll_callback(EVENT *ev acl_unused, EPOLL_EVENT *ee)
{
	acl_fiber_ready(ee->fiber);
}

static void event_epoll_set(EVENT *ev, EPOLL_EVENT *ee, int timeout)
{
	acl_ring_prepend(&ev->epoll_list, &ee->me);

	if (timeout > 0) {
		if (ev->timeout < 0 || timeout < ev->timeout)
			ev->timeout = timeout;
	}
}

int epoll_wait(int epfd, struct epoll_event *events,
	int maxevents, int timeout)
{
	EVENT *ev;
	EPOLL_EVENT *ee;
	acl_int64 begin, now;

	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: EVENT NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (epfd < 0) {
		acl_msg_error("%s(%d), %s: invalid epfd %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	if (__epfds == NULL) {
		acl_msg_error("%s(%d), %s: call epoll_create first",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else if (epfd >= __nepfds) {
		acl_msg_error("%s(%d), %s: too large epfd %d >= __nepfds %d",
			__FILE__, __LINE__, __FUNCTION__, epfd, __nepfds);
		return -1;
	}

	ee            = &__epfds[epfd];
	ee->events    = events;
	ee->maxevents = maxevents;
	ee->fiber     = fiber_running();
	ee->proc      = epoll_callback;
	ee->nevents   = 0;

	SET_TIME(begin);

	while (1) {
		event_epoll_set(ev, ee, timeout);
		fiber_io_inc();
		acl_fiber_switch();

		if (ee->nevents != 0)
			break;

		SET_TIME(now);
		if (now - begin >= timeout)
			break;
	}

	return ee->nevents;
}

/****************************************************************************/

struct hostent *gethostbyname(const char *name)
{
	static __thread struct hostent ret, *result;
#define BUF_LEN	4096
	static __thread char buf[BUF_LEN];

	return gethostbyname_r(name, &ret, buf, BUF_LEN, &result, &h_errno)
		== 0 ? result : NULL;
}

static char dns_ip[128] = "8.8.8.8";
static int dns_port = 53;

void acl_fiber_set_dns(const char* ip, int port)
{
	snprintf(dns_ip, sizeof(dns_ip), "%s", ip);
	dns_port = port;
}

int gethostbyname_r(const char *name, struct hostent *ret,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
	ACL_RES *ns = NULL;
	ACL_DNS_DB *res = NULL;
	size_t n = 0, len, i = 0;
	ACL_ITER iter;

#define	RETURN(x) do { \
	if (res) \
		acl_netdb_free(res); \
	if (ns) \
		acl_res_free(ns); \
	return (x); \
} while (0)

	if (!acl_var_hook_sys_api)
		return __sys_gethostbyname_r(name, ret, buf, buflen, result,
				h_errnop);

	ns = acl_res_new(dns_ip, dns_port);

	memset(ret, 0, sizeof(struct hostent));
	memset(buf, 0, buflen);

	if (ns == NULL) {
		acl_msg_error("%s(%d), %s: acl_res_new NULL, name: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, name, dns_ip, dns_port);
		RETURN (-1);
	}

	res = acl_res_lookup(ns, name);
	if (res == NULL) {
		acl_msg_error("%s(%d), %s: acl_res_lookup NULL, name: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, name, dns_ip, dns_port);
		if (h_errnop)
			*h_errnop = HOST_NOT_FOUND;
		RETURN (-1);
	}

	len = strlen(name);
	n += len;
	if (n >= buflen) {
		acl_msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop)
			*h_errnop = ERANGE;
		RETURN (-1);
	}
	memcpy(buf, name, len);
	buf[len] = 0;
	ret->h_name = buf;
	buf += len + 1;

#define MAX_COUNT	64
	len = 8 * MAX_COUNT;
	n += len;
	if (n >= buflen) {
		acl_msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop)
			*h_errnop = ERANGE;
		RETURN (-1);
	}
	ret->h_addr_list = (char**) buf;
	buf += len;

	acl_foreach(iter, res) {
		ACL_HOSTNAME *h = (ACL_HOSTNAME*) iter.data;

		len = strlen(h->ip);
		n += len;
		memcpy(buf, h->ip, len);
		buf[len] = 0;

		if (i >= MAX_COUNT)
			break;
		ret->h_addr_list[i++] = buf;
		buf += len + 1;
		ret->h_length += len;
	}

	if (i == 0) {
		acl_msg_error("%s(%d), %s: i == 0",
			__FILE__, __LINE__, __FUNCTION__);
		if (h_errnop)
			*h_errnop = ERANGE;
		RETURN (-1);
	}

	*result = ret;

	RETURN (0);
}
