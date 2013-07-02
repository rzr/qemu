#ifndef _QEMU_HANDLE_GEN_H
#define _QEMU_HANDLE_GEN_H

#include "yagl_types.h"

void yagl_handle_gen_init(void);

void yagl_handle_gen_reset(void);

void yagl_handle_gen_cleanup(void);

yagl_host_handle yagl_handle_gen(void);

#endif
