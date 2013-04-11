#ifndef _QEMU_VIGS_UTILS_H
#define _QEMU_VIGS_UTILS_H

#include "vigs_types.h"
#include "vigs_protocol.h"

/*
 * Returns format's bytes-per-pixel.
 */
uint32_t vigs_format_bpp(vigsp_surface_format format);

#endif
