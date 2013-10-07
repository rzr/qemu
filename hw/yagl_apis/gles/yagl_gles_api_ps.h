#ifndef _QEMU_YAGL_GLES_API_PS_H
#define _QEMU_YAGL_GLES_API_PS_H

#include "yagl_api.h"

struct yagl_gles_driver;
struct yagl_avl_table;

struct yagl_gles_api_ps
{
    struct yagl_api_ps base;

    struct yagl_gles_driver *driver;

    struct yagl_avl_table *locations;
};

void yagl_gles_api_ps_init(struct yagl_gles_api_ps *gles_api_ps,
                           struct yagl_gles_driver *driver);

void yagl_gles_api_ps_cleanup(struct yagl_gles_api_ps *gles_api_ps);

void yagl_gles_api_ps_add_location(struct yagl_gles_api_ps *gles_api_ps,
                                   uint32_t location,
                                   GLint actual_location);

GLint yagl_gles_api_ps_translate_location(struct yagl_gles_api_ps *gles_api_ps,
                                          GLboolean tl,
                                          uint32_t location);

void yagl_gles_api_ps_remove_location(struct yagl_gles_api_ps *gles_api_ps,
                                      uint32_t location);

#endif
