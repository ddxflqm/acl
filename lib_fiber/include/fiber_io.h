#ifndef FIBER_IO_INCLUDE_H
#define FIBER_IO_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

void fiber_io_hook(void);
acl_int64 fiber_delay(acl_int64 n);

#ifdef __cplusplus
}
#endif

#endif
