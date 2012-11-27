#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"

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
    driver->dyn_lib = NULL;
}

void yagl_egl_driver_cleanup(struct yagl_egl_driver *driver)
{
    if (driver->dyn_lib) {
        yagl_dyn_lib_destroy(driver->dyn_lib);
    }
}
