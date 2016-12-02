#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static ACL_FIBER *__fiber_wait1 = NULL;
static ACL_FIBER *__fiber_wait2 = NULL;
static ACL_FIBER *__fiber_sleep = NULL;
static ACL_FIBER *__fiber_sleep2 = NULL;

static void fiber_wait(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) ctx;
	int left;

	printf("fiber-%d begin to sem_wait\r\n", acl_fiber_self());
	left = acl_fiber_sem_wait(sem);
	printf("fiber-%d sem_wait ok, left: %d\r\n", acl_fiber_self(), left);
	if (acl_fiber_killed(fiber))
		printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
}

static void fiber_sleep(ACL_FIBER *fiber, void *ctx acl_unused)
{
	while (1) {
		printf("fiber-%d begin sleep\r\n", acl_fiber_self());
		acl_fiber_sleep(2);
		printf("fiber-%d wakeup\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_sleep2(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	while (1)
	{
		printf("-----fiber-%d, %p sleep ---\r\n",
			acl_fiber_self(), fiber);
		sleep(1);
		printf("-----fiber-%d wakup ---\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_killer(ACL_FIBER *fiber, void *ctx acl_unused)
{
	acl_fiber_sleep(1);

	printf("---kill(killer-%d, %p) fiber_wait1: fiber-%d---\r\n",
		acl_fiber_self(), fiber, acl_fiber_id(__fiber_wait1));

	acl_fiber_kill(__fiber_wait1);

	printf("---kill fiber_wait2: fiber-%d---\r\n",
		acl_fiber_id(__fiber_wait2));
	acl_fiber_kill(__fiber_wait2);

	acl_fiber_sleep(1);
	printf("---kill fiber_sleep: fiber-%d---\r\n",
		acl_fiber_id(__fiber_sleep));
	acl_fiber_kill(__fiber_sleep);

	//acl_fiber_kill(__fiber_sleep2);

	printf("=====all fiber are killed, %d, %p=======\r\n",
		acl_fiber_self(), fiber);
	//acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	ACL_FIBER_SEM *sem;

	acl_msg_stdout_enable(1);

	while ((ch = getopt(argc, argv, "hn:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	sem = acl_fiber_sem_create(0);

	__fiber_wait1 = acl_fiber_create(fiber_wait, sem, 32000);
	__fiber_wait2 = acl_fiber_create(fiber_wait, sem, 32000);
	__fiber_sleep = acl_fiber_create(fiber_sleep, sem, 32000);

	__fiber_sleep2 = acl_fiber_create(fiber_sleep2, NULL, 32000);
	acl_fiber_create(fiber_killer, NULL, 32000);

	printf("----%s-%d----\r\n", __FUNCTION__, __LINE__);
	acl_fiber_schedule();
	printf("----%s-%d----\r\n", __FUNCTION__, __LINE__);
	acl_fiber_sem_free(sem);
	printf("----%s-%d----\r\n", __FUNCTION__, __LINE__);

	return 0;
}
