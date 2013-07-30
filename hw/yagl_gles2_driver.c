#include <GLES2/gl2.h>
#include "yagl_gles2_driver.h"

void yagl_gles2_driver_ps_init(struct yagl_gles2_driver_ps *driver_ps,
                               struct yagl_gles2_driver *driver,
                               struct yagl_gles_driver_ps *common)
{
    driver_ps->driver = driver;
    driver_ps->common = common;
}

void yagl_gles2_driver_ps_cleanup(struct yagl_gles2_driver_ps *driver_ps)
{
}

void yagl_gles2_driver_init(struct yagl_gles2_driver *driver)
{
}

void yagl_gles2_driver_cleanup(struct yagl_gles2_driver *driver)
{
}
