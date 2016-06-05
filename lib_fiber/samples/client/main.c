#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __max_loop = 10000;
static int __max_fibers = 100;

static void echo_client(ACL_VSTREAM *cstream)
{
	char  buf[8192];
	int   ret, i;
	const char *str = "hello world\r\n";

	for (i = 0; i < __max_loop; i++) {
		if (acl_vstream_writen(cstream, str, strlen(str))
			== ACL_VSTREAM_EOF)
		{
			printf("write error: %s\r\n", acl_last_serror());
			break;
		}

		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s\r\n", acl_last_serror());
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);

	}

	acl_vstream_close(cstream);
}

static void fiber_connect(FIBER *fiber acl_unused, void *ctx)
{
	const char *addr = (const char *) ctx;
	ACL_VSTREAM *cstream = acl_vstream_connect(addr, ACL_BLOCKING,
			0, 0, 4096);
	if (cstream == NULL)
		printf("connect %s error %s\r\n", addr, acl_last_serror());
	else
		echo_client(cstream);

	printf("max_fibers: %d\r\n", __max_fibers);
	if (--__max_fibers == 0)
		fiber_io_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -c max_fibers\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;
	char  addr[256];
       
	snprintf(addr, sizeof(addr), "%s", "0.0.0.0:9002");

	while ((ch = getopt(argc, argv, "hc:n:s:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fibers = atoi(optarg);
			break;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		default:
			break;
		}
	}

	for (i = 0; i < __max_fibers; i++)
		fiber_create(fiber_connect, addr, 32768);

	printf("call fiber_schedule\r\n");

	fiber_schedule();

	return 0;
}
