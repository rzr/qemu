#include "yagl_egl_offscreen_image.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_client_context.h"
#include "yagl_client_image.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

YAGL_DECLARE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static bool yagl_egl_offscreen_image_update(struct yagl_eglb_image *image,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t bpp,
                                            target_ulong pixels)
{
    struct yagl_eglb_context *ctx =
        (egl_offscreen_ts->ctx ? &egl_offscreen_ts->ctx->base : NULL);

    if (!ctx) {
        return true;
    }

    if (!image->glegl_image) {
        image->glegl_image = ctx->client_ctx->create_image(ctx->client_ctx);
        if (!image->glegl_image) {
            return true;
        }
    }

    return image->glegl_image->update(image->glegl_image, width, height, bpp, pixels);
}

static void yagl_egl_offscreen_image_destroy(struct yagl_eglb_image *image)
{
    struct yagl_egl_offscreen_image *oimage =
        (struct yagl_egl_offscreen_image*)image;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_image_destroy, NULL);

    yagl_eglb_image_cleanup(image);

    g_free(oimage);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_image
    *yagl_egl_offscreen_image_create(struct yagl_egl_offscreen_display *dpy,
                                     yagl_winsys_id buffer)
{
    struct yagl_egl_offscreen_image *image;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_image_create,
                        "dpy = %p", dpy);

    image = g_malloc0(sizeof(*image));

    yagl_eglb_image_init(&image->base, buffer, &dpy->base);

    image->base.update_offscreen = &yagl_egl_offscreen_image_update;
    image->base.destroy = &yagl_egl_offscreen_image_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return image;
}
