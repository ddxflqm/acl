#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __max_loop = 1000;
static int __max_fiber = 1000;
static int __left_fiber = 1000;
static struct timeval __begin, __end;

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static void fiber_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  i;

	for (i = 0; i < __max_loop; i++)
		fiber_yield();

	--__left_fiber;
	if (__left_fiber == 0) {
		double spent;
		long long count;

		gettimeofday(&__end, NULL);
		count = __max_fiber * __max_loop;
		spent = stamp_sub(&__end, &__begin);
		printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
			__max_fiber, count, spent,
			(count * 1000) / (spent > 0 ? spent : 1));
	}
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
			__left_fiber = __max_fiber;
			break;
		default:
			break;
		}
	}

	gettimeofday(&__begin, NULL);

	for (i = 0; i < __max_fiber; i++)
		fiber_create(fiber_main, NULL, 32768);

	fiber_schedule();

	return 0;
}
