#ifndef _QEMU_VIGS_ID_GEN_H
#define _QEMU_VIGS_ID_GEN_H

#include "vigs_types.h"

void vigs_id_gen_init(void);

void vigs_id_gen_reset(void);

void vigs_id_gen_cleanup(void);

vigsp_surface_id vigs_id_gen(void);

#endif
