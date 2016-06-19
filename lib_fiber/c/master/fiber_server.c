#include "stdafx.h"
#include <stdarg.h>

/* including the internal headers from lib_acl/src/master */
#include "master_proto.h"
#include "master_params.h"
#include "template/master_log.h"

#include "fiber/lib_fiber.h"
#include "fiber.h"

#define STACK_SIZE	320000

int   acl_var_fibers_pid;
char *acl_var_fibers_procname = NULL;
char *acl_var_fibers_log_file = NULL;

int   acl_var_fibers_buf_size;
int   acl_var_fibers_rw_timeout;
int   acl_var_fibers_max_debug;
int   acl_var_fibers_enable_core;
static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ "fiber_buf_size", 8192, &acl_var_fibers_buf_size, 0, 0 },
	{ "fiber_rw_timeout", 120, &acl_var_fibers_rw_timeout, 0, 0 },
	{ "fiber_max_debug", 1000, &acl_var_fibers_max_debug, 0, 0 },
	{ "fibers_enable_core", 1, &acl_var_fibers_enable_core, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

char *acl_var_fibers_queue_dir;
char *acl_var_fibers_log_debug;
char *acl_var_fibers_deny_banner;
char *acl_var_fibers_access_allow;
char *acl_var_fibers_owner;
static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ "fibers_queue_dir", "", &acl_var_fibers_queue_dir },
	{ "fibers_log_debug", "all:1", &acl_var_fibers_log_debug },
	{ "fibers_deny_banner", "Denied!\r\n", &acl_var_fibers_deny_banner },
	{ "fibers_access_allow", "all", &acl_var_fibers_access_allow },
	{ "fibers_owner", "", &acl_var_fibers_owner },

	{ 0, 0, 0 },
};

static int    __argc;
static char **__argv;
static int    __daemon_mode = 0;
static void (*__service)(FIBER*, ACL_VSTREAM*, void*) = NULL;
static int   *__service_ctx = NULL;
static char   __service_name[256];
static void (*__service_onexit) = NULL;
static char  *__deny_info = NULL;

static unsigned      __server_generation;
static ACL_VSTREAM **__sstreams;

static void server_exit(void)
{
}

static void server_abort(FIBER *fiber, void *ctx)
{
	(void) fiber;
	(void) ctx;
}

static void fiber_client(FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	const char *peer = ACL_VSTREAM_PEER(cstream);
	char  addr[256];

	if (peer) {
		char *ptr;
		ACL_SAFE_STRNCPY(addr, peer, sizeof(addr));
		ptr = strchr(addr, ':');
		if (ptr)
			*ptr = 0;
	} else
		addr[0] = 0;

	if (addr[0] != 0 && !acl_access_permit(addr)) {
		if (__deny_info && *__deny_info)
			acl_vstream_fprintf(cstream, "%s\r\n", __deny_info);
		acl_vstream_close(cstream);
		return;
	}

	__service(fiber, cstream, ctx);
}

static void fiber_accept_main(FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx, *cstream;
	char  ip[64];

	while (1) {
		cstream = acl_vstream_accept(sstream, ip, sizeof(ip));
		if (cstream != NULL) {
			fiber_create(fiber_client, cstream, STACK_SIZE);
			continue;
		}

#if ACL_EAGAIN == ACL_EWOULDBLOCK
		if (errno == ACL_EAGAIN || errno == ACL_EINTR)
#else
		if (errno == ACL_EAGAIN || errno == ACL_EWOULDBLOCK
			|| errno == ACL_EINTR)
#endif
			continue;

		acl_msg_warn("accept connection: %s(%d, %d), stoping ...",
			acl_last_serror(), errno, ACL_EAGAIN);
		server_abort(fiber_running(), sstream);
		return;
	}
}

#ifdef ACL_UNIX

static ACL_VSTREAM **server_daemon_open(int count, int fdtype)
{
	const char *myname = "server_daemon_open";
	ACL_VSTREAM *sstream, **sstreams;
	ACL_SOCKET fd;
	int i;

	/* socket count is as same listen_fd_count in parent process */

	sstreams = (ACL_VSTREAM **)
		acl_mycalloc(count + 1, sizeof(ACL_VSTREAM *));

	for (i = 0; i < count + 1; i++)
		sstreams[i] = NULL;

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + count; fd++) {
		sstream = acl_vstream_fdopen(fd, O_RDWR,
				acl_var_fibers_buf_size,
				acl_var_fibers_rw_timeout, fdtype);
		if (sstream == NULL)
			acl_msg_fatal("%s(%d)->%s: stream null, fd = %d",
				__FILE__, __LINE__, myname, fd);

		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		fiber_create(fiber_accept_main, sstream, STACK_SIZE);
		sstreams[i++] = sstream;
	}

	fiber_create(server_abort, ACL_MASTER_STAT_STREAM, STACK_SIZE);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	return sstreams;
}

#endif

static ACL_VSTREAM **server_alone_open(const char *addrs)
{
	const char   *myname = "server_alone_open";
	ACL_ARGV*     tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER      iter;
	int           i;
	ACL_VSTREAM **streams = (ACL_VSTREAM **)
		acl_mycalloc(tokens->argc + 1, sizeof(ACL_VSTREAM *));

	for (i = 0; i < tokens->argc + 1; i++)
		streams[i] = NULL;

	i = 0;
	acl_foreach(iter, tokens) {
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
		if (sstream == NULL) {
			acl_msg_error("%s(%d): listen %s error(%s)",
				myname, __LINE__, addr, acl_last_serror());
			exit(1);
		}

		streams[i++] = sstream;

		fiber_create(fiber_accept_main, sstream, STACK_SIZE);
	}

	acl_argv_free(tokens);
	return streams;
}

static void open_service_log(void)
{
	/* first, close the master's log */
#ifdef ACL_UNIX
	master_log_close();
#endif

	/* second, open the service's log */
	acl_msg_open(acl_var_fibers_log_file, acl_var_fibers_procname);

	if (acl_var_fibers_log_debug && *acl_var_fibers_log_debug
		&& acl_var_fibers_max_debug >= 100)
	{
		acl_debug_init2(acl_var_fibers_log_debug,
			acl_var_fibers_max_debug);
	}
}

static void server_init(const char *procname)
{
	const char *myname = "server_init";
	static int inited = 0;
	const char* ptr;

	if (inited)
		return;

	inited = 1;

	if (procname == NULL || *procname == 0)
		acl_msg_fatal("%s(%d); procname null", myname, __LINE__);

	/*
	 * Don't die when a process goes away unexpectedly.
	 */
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif

	/*
	 * Don't die for frivolous reasons.
	 */
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_IGN);
#endif

	/*
	 * May need this every now and then.
	 */
#ifdef ACL_UNIX
	acl_var_fibers_pid = getpid();
#elif defined(ACL_WINDOWS)
	acl_var_fibers_pid = _getpid();
#else
	acl_var_fibers_pid = 0;
#endif
	acl_var_fibers_procname = acl_mystrdup(acl_safe_basename(procname));

	ptr = acl_getenv("SERVICE_LOG");
	if ((ptr = acl_getenv("SERVICE_LOG")) != NULL && *ptr != 0)
		acl_var_fibers_log_file = acl_mystrdup(ptr);
	else {
		acl_var_fibers_log_file = acl_mystrdup("acl_master.log");
		acl_msg_info("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			" use %s log", __FILE__, __LINE__, myname,
			acl_var_fibers_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

#ifdef ACL_UNIX
	acl_master_vars_init(acl_var_fibers_buf_size, acl_var_fibers_rw_timeout);
#endif

	if (__deny_info == NULL)
		__deny_info = acl_var_fibers_deny_banner;
	if (acl_var_fibers_access_allow && *acl_var_fibers_access_allow)
		acl_access_add(acl_var_fibers_access_allow, ", \t", ":");
}

static void usage(int argc, char * argv[])
{
	int   i;
	const char *service_name;

	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc(%d) invalid",
			__FILE__, __LINE__, argc);

	service_name = acl_safe_basename(argv[0]);

	for (i = 0; i < argc; i++)
		acl_msg_info("argv[%d]: %s", i, argv[i]);

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -f conf_file"
		" -L listen_addrs",
		service_name);
}

static va_list __ap_dest;

static void fiber_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	const char *myname = "fiber_main";
	const char *service_name = acl_safe_basename(__argv[0]);
	char *root_dir = NULL, *user = NULL, *addrs = NULL;
	int   c, fdtype = 0, socket_count = 1, name = -1000;
	char *generation, conf_file[1024];
	void *pre_jail_ctx = NULL, *post_init_ctx = NULL;
	ACL_MASTER_SERVER_INIT_FN pre_jail = NULL;
	ACL_MASTER_SERVER_INIT_FN post_init = NULL;

	master_log_open(__argv[0]);

	conf_file[0] = 0;

	while ((c = getopt(__argc, __argv, "hc:n:s:t:uvf:L:")) > 0) {
		switch (c) {
		case 'h':
			usage(__argc, __argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			ACL_SAFE_STRNCPY(conf_file, optarg, sizeof(conf_file));
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			socket_count = atoi(optarg);
			break;
		case 'u':
			user = "setme";
			break;
		case 't':
			/* deprecated, just go through */
			break;
		case 'v':
			acl_msg_verbose++;
			break;
		case 'L':
			addrs = optarg;
			break;
		default:
			break;
		}
	}

	if (conf_file[0] == 0)
		acl_msg_info("%s(%d)->%s: no configure file",
			__FILE__, __LINE__, myname);
	else
		acl_msg_info("%s(%d)->%s: configure file = %s", 
			__FILE__, __LINE__, myname, conf_file);

	ACL_SAFE_STRNCPY(__service_name, service_name, sizeof(__service_name));

	if (addrs && *addrs)
		__daemon_mode = 0;
	else
		__daemon_mode = 1;

	/*******************************************************************/

	/* Application-specific initialization. */

	/* load configure, set signal */
	server_init(__argv[0]);

	for (; name != ACL_APP_CTL_END; name = va_arg(__ap_dest, int)) {
		switch (name) {
		case ACL_MASTER_SERVER_BOOL_TABLE:
			acl_get_app_conf_bool_table(
				va_arg(__ap_dest, ACL_CONFIG_BOOL_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT_TABLE:
			acl_get_app_conf_int_table(
				va_arg(__ap_dest, ACL_CONFIG_INT_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT64_TABLE:
			acl_get_app_conf_int64_table(
				va_arg(__ap_dest, ACL_CONFIG_INT64_TABLE *));
			break;
		case ACL_MASTER_SERVER_STR_TABLE:
			acl_get_app_conf_str_table(
				va_arg(__ap_dest, ACL_CONFIG_STR_TABLE *));
			break;
		case ACL_MASTER_SERVER_PRE_INIT:
			pre_jail = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_POST_INIT:
			post_init = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_EXIT:
			__service_onexit =
				va_arg(__ap_dest, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_DENY_INFO:
			__deny_info = acl_mystrdup(va_arg(__ap_dest, const char*));
			break;
		default:
			acl_msg_fatal("%s: bad name(%d)", myname, name);
		}
	}

	/*******************************************************************/

	if (root_dir)
		root_dir = acl_var_fibers_queue_dir;

	if (user)
		user = acl_var_fibers_owner;

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &__server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, __server_generation);
	}

	/*******************************************************************/

	/* Run pre-jail initialization. */
	if (__daemon_mode && *acl_var_fibers_queue_dir
		&& chdir(acl_var_fibers_queue_dir) < 0)
	{
		acl_msg_fatal("chdir(\"%s\"): %s", acl_var_fibers_queue_dir,
			acl_last_serror());
	}

	/* open all listen streams */

	if (__daemon_mode == 0)
		__sstreams = server_alone_open(addrs);
#ifdef ACL_UNIX
	else if (socket_count <= 0)
		acl_msg_fatal("%s(%d): invalid socket_count: %d",
			myname, __LINE__, socket_count);
	else
		__sstreams = server_daemon_open(socket_count, fdtype);
#else
	else
		acl_msg_fatal("%s(%d): addrs NULL", myname, __LINE__);
#endif

	if (pre_jail)
		pre_jail(pre_jail_ctx);

#ifdef ACL_UNIX
	if (user && *user)
		acl_chroot_uid(root_dir, user);
#endif

	/* open the server's log */
	open_service_log();

#ifdef ACL_UNIX
	/* if enable dump core when program crashed ? */
	if (acl_var_fibers_enable_core)
		acl_set_core_limit(0);
#endif

	/* Run post-jail initialization. */
	if (post_init)
		post_init(post_init_ctx);

	acl_msg_info("%s(%d), %s daemon started, log: %s",
		myname, __LINE__, __argv[0], acl_var_fibers_log_file);

	/* not reached here */
	server_exit();
}

void fiber_server_main(int argc, char *argv[],
	void (*service)(FIBER*, ACL_VSTREAM*, void*), void *ctx, int name, ...)
{
	va_list ap;

	__argc = argc;
	__argv = argv;

	/* Set up call-back info. */
	__service      = service;
	__service_ctx  = ctx;

	va_start(ap, name);
	va_copy(__ap_dest, ap);
	va_end(ap);

	fiber_create(fiber_main, NULL, STACK_SIZE);
	fiber_schedule();
}