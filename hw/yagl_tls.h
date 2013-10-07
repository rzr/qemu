#ifndef _QEMU_YAGL_TLS_H
#define _QEMU_YAGL_TLS_H

#include "yagl_types.h"

#define YAGL_DEFINE_TLS(type, x) type x
#define YAGL_DECLARE_TLS(type, x) extern YAGL_DEFINE_TLS(type, x)

#endif
