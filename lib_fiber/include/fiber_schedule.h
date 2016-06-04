#ifndef FIBER_SCHEDULE_INCLUDE_H
#define FIBER_SCHEDULE_INCLUDE_H

#include <ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FIBER_STATUS_READY,
	FIBER_STATUS_RUNNING,
	FIBER_STATUS_EXITING,
} fiber_status_t;

typedef struct FIBER FIBER;

struct FIBER {
	ACL_RING me;
	size_t id;
	size_t slot;
	acl_int64 when;
	int sys;
	fiber_status_t status;
	ucontext_t uctx;
	void (*fn)(FIBER *, void *);
	void *arg;
	char *stack;
	size_t size;
	char  buf[1];
};

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size);
int    fiber_id(const FIBER *fiber);
void   fiber_free(FIBER *fiber);
void   fiber_ready(FIBER *fiber);
void   fiber_exit(int exit_code);
FIBER *fiber_running(void);
int    fiber_yield(void);
void   fiber_system(void);
void   fiber_switch(void);
void   fiber_schedule(void);
void   fiber_init(void);
void   fiber_count_inc(void);
void   fiber_count_dec(void);

#ifdef __cplusplus
}
#endif

#endif
