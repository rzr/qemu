#ifndef _QEMU_YAGL_EGL_NATIVE_CONFIG_H
#define _QEMU_YAGL_EGL_NATIVE_CONFIG_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_process_state;

struct yagl_egl_native_config
{
    /*
     * Filled by the driver.
     */

    EGLint red_size;
    EGLint green_size;
    EGLint blue_size;
    EGLint alpha_size;
    EGLint buffer_size;
    EGLenum caveat;
    EGLint config_id;
    EGLint frame_buffer_level;
    EGLint depth_size;
    EGLint max_pbuffer_width;
    EGLint max_pbuffer_height;
    EGLint max_pbuffer_size;
    EGLint max_swap_interval;
    EGLint min_swap_interval;
    EGLint native_visual_id;
    EGLint native_visual_type;
    EGLint samples_per_pixel;
    EGLint stencil_size;
    EGLenum transparent_type;
    EGLint trans_red_val;
    EGLint trans_green_val;
    EGLint trans_blue_val;

    void *driver_data;

    /*
     * Filled automatically by 'yagl_egl_config_xxx'.
     */

    EGLint surface_type;
    EGLBoolean native_renderable;
    EGLint renderable_type;
    EGLenum conformant;
    EGLint sample_buffers_num;
};

/*
 * Initialize to default values.
 */
void yagl_egl_native_config_init(struct yagl_egl_native_config *cfg);

#endif
