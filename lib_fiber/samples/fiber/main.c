#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fiber.h"

static int __max_loop = 1000;
static int __max_fiber = 1000;

static void fiber_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  i;

	for (i = 0; i < __max_loop; i++)
		fiber_yield();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max_loop -m max_fiber\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;

	while ((ch = getopt(argc, argv, "hn:m:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 'm':
			__max_fiber = atoi(optarg);
			break;
		default:
			break;
		}
	}

	fiber_init();

	for (i = 0; i < __max_fiber; i++)
		fiber_create(fiber_main, NULL, 32768);

	fiber_schedule();

	return 0;
}
