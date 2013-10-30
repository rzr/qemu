#include <GL/gl.h>
#include "yagl_gles_api_ts.h"
#include "yagl_gles_driver.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_gles_api_ts_init(struct yagl_gles_api_ts *gles_api_ts,
                           struct yagl_gles_driver *driver,
                           struct yagl_gles_api_ps *ps)
{
    gles_api_ts->driver = driver;
    gles_api_ts->ps = ps;
}

void yagl_gles_api_ts_cleanup(struct yagl_gles_api_ts *gles_api_ts)
{
    if (gles_api_ts->num_arrays > 0) {
        yagl_ensure_ctx(0);
        gles_api_ts->driver->DeleteBuffers(gles_api_ts->num_arrays,
                                           gles_api_ts->arrays);
        yagl_unensure_ctx(0);
    }

    g_free(gles_api_ts->arrays);
}
