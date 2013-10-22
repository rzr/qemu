#include <GL/gl.h>
#include "yagl_gles_driver.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_egl_interface.h"

void yagl_gles_driver_init(struct yagl_gles_driver *driver,
                           yagl_gl_version gl_version)
{
    driver->gl_version = gl_version;
}

void yagl_gles_driver_cleanup(struct yagl_gles_driver *driver)
{
}

void yagl_ensure_ctx(void)
{
    assert(cur_ts);
    cur_ts->ps->egl_iface->ensure_ctx(cur_ts->ps->egl_iface);
}

void yagl_unensure_ctx(void)
{
    assert(cur_ts);
    cur_ts->ps->egl_iface->unensure_ctx(cur_ts->ps->egl_iface);
}
