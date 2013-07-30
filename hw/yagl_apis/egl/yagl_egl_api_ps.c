#include "yagl_egl_api_ps.h"
#include "yagl_egl_display.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_interface.h"
#include "yagl_process.h"

void yagl_egl_api_ps_init(struct yagl_egl_api_ps *egl_api_ps,
                          struct yagl_egl_driver_ps *driver_ps,
                          struct yagl_egl_interface *egl_iface)
{
    egl_api_ps->driver_ps = driver_ps;
    egl_api_ps->egl_iface = egl_iface;

    yagl_process_register_egl_interface(egl_api_ps->base.ps, egl_api_ps->egl_iface);

    qemu_mutex_init(&egl_api_ps->mutex);

    QLIST_INIT(&egl_api_ps->displays);
}

void yagl_egl_api_ps_fini(struct yagl_egl_api_ps *egl_api_ps)
{
    struct yagl_egl_display *dpy, *next;

    QLIST_FOREACH_SAFE(dpy, &egl_api_ps->displays, entry, next) {
        QLIST_REMOVE(dpy, entry);
        yagl_egl_display_destroy(dpy);
    }

    assert(QLIST_EMPTY(&egl_api_ps->displays));
}

void yagl_egl_api_ps_cleanup(struct yagl_egl_api_ps *egl_api_ps)
{
    assert(QLIST_EMPTY(&egl_api_ps->displays));

    qemu_mutex_destroy(&egl_api_ps->mutex);

    yagl_process_unregister_egl_interface(egl_api_ps->base.ps);

    yagl_egl_interface_cleanup(egl_api_ps->egl_iface);
    g_free(egl_api_ps->egl_iface);
    egl_api_ps->egl_iface = NULL;

    egl_api_ps->driver_ps->destroy(egl_api_ps->driver_ps);
    egl_api_ps->driver_ps = NULL;
}

struct yagl_egl_display *yagl_egl_api_ps_display_get(struct yagl_egl_api_ps *egl_api_ps,
                                                     yagl_host_handle handle)
{
    struct yagl_egl_display *dpy;

    qemu_mutex_lock(&egl_api_ps->mutex);

    QLIST_FOREACH(dpy, &egl_api_ps->displays, entry) {
        if (dpy->handle == handle) {
            qemu_mutex_unlock(&egl_api_ps->mutex);

            return dpy;
        }
    }

    qemu_mutex_unlock(&egl_api_ps->mutex);

    return NULL;
}

struct yagl_egl_display *yagl_egl_api_ps_display_add(struct yagl_egl_api_ps *egl_api_ps,
                                                     target_ulong display_id)
{
    struct yagl_egl_display *dpy;

    qemu_mutex_lock(&egl_api_ps->mutex);

    QLIST_FOREACH(dpy, &egl_api_ps->displays, entry) {
        if (dpy->display_id == display_id) {
            qemu_mutex_unlock(&egl_api_ps->mutex);

            return dpy;
        }
    }

    dpy = yagl_egl_display_create(egl_api_ps->driver_ps, display_id);

    if (!dpy) {
        qemu_mutex_unlock(&egl_api_ps->mutex);

        return NULL;
    }

    QLIST_INSERT_HEAD(&egl_api_ps->displays, dpy, entry);

    qemu_mutex_unlock(&egl_api_ps->mutex);

    return dpy;
}
