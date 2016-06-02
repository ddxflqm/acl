#ifndef FIBER_INCLUDE_H
#define FIBER_INCLUDE_H

#include "event.h"

/* in fiber_io.c */
void fiber_io_check(void);
void fiber_wait_read(int fd);
void fiber_wait_write(int fd);
void fiber_io_dec(void);
void fiber_io_inc(void);
EVENT *fiber_io_event(void);

/* in fiber_net.c */
void fiber_net_hook(void);

#endif
