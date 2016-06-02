#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fiber.h"

static void fiber_sleep(void *ctx)
{
	int   n = *((int*) ctx);
	time_t last, now;

	printf("begin sleep %d\r\n", n);
	time(&last);
	n = (int) sleep(n);
	time(&now);

	printf("wakup, n: %d, sleep: %ld\r\n", n, (long) (now - last));

	fiber_io_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n seconds\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 1;

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			break;
		}
	}

	printf("n: %d\r\n", n);
	fiber_create(fiber_sleep, &n, 32768);

	fiber_schedule();

	return 0;
}
