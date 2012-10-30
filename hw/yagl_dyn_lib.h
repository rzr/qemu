#ifndef _QEMU_YAGL_DYN_LIB_H
#define _QEMU_YAGL_DYN_LIB_H

#include "yagl_types.h"

struct yagl_dyn_lib;

struct yagl_dyn_lib *yagl_dyn_lib_create(void);

void yagl_dyn_lib_destroy(struct yagl_dyn_lib *dyn_lib);

bool yagl_dyn_lib_load(struct yagl_dyn_lib *dyn_lib, const char *name);

void yagl_dyn_lib_unload(struct yagl_dyn_lib *dyn_lib);

void *yagl_dyn_lib_get_sym(struct yagl_dyn_lib *dyn_lib, const char* sym_name);

const char *yagl_dyn_lib_get_error(struct yagl_dyn_lib *dyn_lib);

#endif
