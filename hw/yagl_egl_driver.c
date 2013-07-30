#include "yagl_egl_driver.h"

void yagl_egl_driver_ps_init(struct yagl_egl_driver_ps *driver_ps,
                             struct yagl_egl_driver *driver,
                             struct yagl_process_state *ps)
{
    driver_ps->driver = driver;
    driver_ps->ps = ps;
}

void yagl_egl_driver_ps_cleanup(struct yagl_egl_driver_ps *driver_ps)
{
}

void yagl_egl_driver_init(struct yagl_egl_driver *driver)
{
}

void yagl_egl_driver_cleanup(struct yagl_egl_driver *driver)
{
}
