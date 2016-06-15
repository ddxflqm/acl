#ifndef LIB_FIBER_INCLUDE_H
#define LIB_FIBER_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FIBER FIBER;

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size);
int fiber_id(const FIBER *fiber);
void fiber_set_errno(FIBER *fiber, int errnum);
int fiber_errno(FIBER *fiber);
int fiber_status(const FIBER *fiber);
int fiber_yield(void);
void fiber_ready(FIBER *fiber);
void fiber_switch(void);
void fiber_schedule(void);

void fiber_io_stop(void);
unsigned int fiber_delay(unsigned int milliseconds);
unsigned int fiber_sleep(unsigned int seconds);
FIBER *fiber_create_timer(unsigned int milliseconds,
	void (*fn)(FIBER *, void *), void *ctx);
void fiber_reset_timer(FIBER *timer, unsigned int milliseconds);

void fiber_set_dns(const char* ip, int port);

#ifdef __cplusplus
}
#endif

#endif
