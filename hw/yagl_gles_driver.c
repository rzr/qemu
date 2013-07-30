#include <GL/gl.h>
#include "yagl_gles_driver.h"

void yagl_gles_driver_ps_init(struct yagl_gles_driver_ps *driver_ps,
                              struct yagl_process_state *ps)
{
    driver_ps->ps = ps;
}

void yagl_gles_driver_ps_cleanup(struct yagl_gles_driver_ps *driver_ps)
{
}
