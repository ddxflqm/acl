#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __rw_timeout = 0;

static void echo_client(FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	char  buf[8192];
	int   ret;

	cstream->rw_timeout = __rw_timeout;

#define	SOCK ACL_VSTREAM_SOCK

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error, fd: %d\r\n", SOCK(cstream));
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error, fd: %d\r\n", SOCK(cstream));
			break;
		}
	}

	acl_vstream_close(cstream);
}

static void fiber_accept(FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		printf("accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(cstream));
		fiber_create(echo_client, cstream, 32768);
	}

	acl_vstream_close(sstream);
}

static void fiber_sleep_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		sleep(1);
		time(&now);
		printf("wakeup, cost %ld seconds\r\n", (long) now - last);
	}
}

static void fiber_sleep2_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		sleep(3);
		time(&now);
		printf(">>>wakeup, cost %ld seconds<<<\r\n", (long) now - last);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr -r rw_timeout -S [if sleep]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int   ch, enable_sleep = 0;

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:S")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'S':
			enable_sleep = 1;
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	acl_non_blocking(ACL_VSTREAM_SOCK(sstream), ACL_NON_BLOCKING);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	fiber_create(fiber_accept, sstream, 32768);

	if (enable_sleep) {
		fiber_create(fiber_sleep_main, NULL, 32768);
		fiber_create(fiber_sleep2_main, NULL, 32768);
	}

	printf("call fiber_schedule\r\n");
	fiber_schedule();

	return 0;
}
