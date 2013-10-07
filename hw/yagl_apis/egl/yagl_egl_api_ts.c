#include "yagl_egl_api_ts.h"
#include "yagl_egl_api_ps.h"
#include "yagl_egl_context.h"
#include "yagl_eglb_context.h"
#include "yagl_egl_backend.h"

void yagl_egl_api_ts_init(struct yagl_egl_api_ts *egl_api_ts,
                          struct yagl_egl_api_ps *api_ps)
{
    egl_api_ts->api_ps = api_ps;
    egl_api_ts->backend = api_ps->backend;
    egl_api_ts->context = NULL;

    yagl_egl_api_ts_reset(egl_api_ts);
}

void yagl_egl_api_ts_cleanup(struct yagl_egl_api_ts *egl_api_ts)
{
    if (egl_api_ts->context) {
        /*
         * Force release current.
         */
        egl_api_ts->backend->release_current(egl_api_ts->backend, true);

        yagl_egl_context_update_surfaces(egl_api_ts->context, NULL, NULL);
    }

    yagl_egl_api_ts_update_context(egl_api_ts, NULL);
}

void yagl_egl_api_ts_update_context(struct yagl_egl_api_ts *egl_api_ts,
                                    struct yagl_egl_context *ctx)
{
    yagl_egl_context_acquire(ctx);

    yagl_egl_context_release(egl_api_ts->context);

    egl_api_ts->context = ctx;
}

void yagl_egl_api_ts_reset(struct yagl_egl_api_ts *egl_api_ts)
{
    egl_api_ts->api = EGL_OPENGL_ES_API;
}
