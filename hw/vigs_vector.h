#ifndef _QEMU_VIGS_VECTOR_H
#define _QEMU_VIGS_VECTOR_H

#include "vigs_types.h"

struct vigs_vector
{
    void *data;
    uint32_t size;
    uint32_t capacity;
};

void vigs_vector_init(struct vigs_vector *v,
                      uint32_t initial_capacity);

void vigs_vector_cleanup(struct vigs_vector *v);

uint32_t vigs_vector_size(struct vigs_vector *v);

uint32_t vigs_vector_capacity(struct vigs_vector *v);

void vigs_vector_resize(struct vigs_vector *v, uint32_t new_size);

/*
 * Returns pointer to appended data.
 */
void *vigs_vector_append(struct vigs_vector *v, uint32_t append_size);

void *vigs_vector_data(struct vigs_vector *v);

#endif
