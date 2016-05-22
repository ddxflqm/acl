#include "lib_acl.h"
#include <time.h>

static int  __max = 10000000;
static char __dummy[256];

static void *thread_producer(void *arg)
{
	ACL_YPIPE *ypipe = (ACL_YPIPE*) arg;
	int   i;

	for (i = 0; i < __max; i++) {
		acl_ypipe_write(ypipe, __dummy);
		acl_ypipe_flush(ypipe);
	}

	return NULL;
}

static void *thread_consumer(void *arg)
{
	ACL_YPIPE *ypipe = (ACL_YPIPE*) arg;
	int   i;

	for (i = 0; i < __max; i++) {
		char *ptr = (char*) acl_ypipe_read(ypipe);
		if (ptr == NULL) {
			//printf("ptr NULL, i: %d\r\n", i);
		}
	}

	printf("i: %d\r\n", i);
	return NULL;
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1, t2;
	ACL_YPIPE *ypipe = acl_ypipe_new();

	memset(__dummy, 'x', sizeof(__dummy));
	__dummy[sizeof(__dummy) - 1] = 0;

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&t2, &attr, thread_consumer, ypipe);
	acl_pthread_create(&t1, &attr, thread_producer, ypipe);
	acl_pthread_join(t2, NULL);
	acl_pthread_join(t1, NULL);

	acl_ypipe_free(ypipe, NULL);
	printf("over\r\n");

	return 0;
}
