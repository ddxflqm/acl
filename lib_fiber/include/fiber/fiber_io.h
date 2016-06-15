#ifndef FIBER_IO_INCLUDE_H
#define FIBER_IO_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

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
