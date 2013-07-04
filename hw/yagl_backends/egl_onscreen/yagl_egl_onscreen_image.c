#include "yagl_egl_onscreen_image.h"
#include "yagl_egl_onscreen_display.h"
#include "yagl_egl_onscreen.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_ref.h"
#include "yagl_client_interface.h"
#include <GL/gl.h>
#include "winsys_gl.h"

struct yagl_egl_onscreen_image_data
{
    struct yagl_ref ref;

    struct winsys_gl_surface *ws_sfc;
};

static void yagl_egl_onscreen_image_data_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_onscreen_image_data *image_data =
        (struct yagl_egl_onscreen_image_data*)ref;

    image_data->ws_sfc->base.release(&image_data->ws_sfc->base);

    yagl_ref_cleanup(ref);

    g_free(image_data);
}

static void yagl_egl_onscreen_image_destroy(struct yagl_eglb_image *image)
{
    struct yagl_egl_onscreen_image *oimage =
        (struct yagl_egl_onscreen_image*)image;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_image_destroy, NULL);

    yagl_eglb_image_cleanup(image);

    g_free(oimage);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_onscreen_image
    *yagl_egl_onscreen_image_create(struct yagl_egl_onscreen_display *dpy,
                                    yagl_winsys_id buffer)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_client_interface *client_iface = NULL;
    struct yagl_egl_onscreen_image_data *image_data = NULL;
    struct yagl_client_image *glegl_image = NULL;
    struct yagl_egl_onscreen_image *image = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_image_create,
                        "dpy = %p, buffer = %u", dpy, buffer);

    client_iface = cur_ts->ps->client_ifaces[yagl_client_api_gles2];

    if (!client_iface) {
        YAGL_LOG_ERROR("No GLESv2 API, can't create image");
        goto out;
    }

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, buffer);

    if (!ws_sfc) {
        goto out;
    }

    image_data = g_malloc0(sizeof(*image_data));

    yagl_ref_init(&image_data->ref, &yagl_egl_onscreen_image_data_destroy);
    ws_sfc->base.acquire(&ws_sfc->base);
    image_data->ws_sfc = ws_sfc;

    glegl_image = client_iface->create_image(client_iface,
                                             ws_sfc->get_texture(ws_sfc),
                                             &image_data->ref);

    yagl_ref_release(&image_data->ref);

    if (!glegl_image) {
        goto out;
    }

    image = g_malloc0(sizeof(*image));

    yagl_eglb_image_init(&image->base, buffer, &dpy->base);

    image->base.glegl_image = glegl_image;
    image->base.destroy = &yagl_egl_onscreen_image_destroy;

out:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return image;
}
