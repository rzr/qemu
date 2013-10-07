#include "yagl_gles_api_ts.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_vector.h"

void yagl_gles_api_ts_init(struct yagl_gles_api_ts *gles_api_ts,
                           struct yagl_gles_driver *driver,
                           struct yagl_gles_api_ps *ps)
{
    gles_api_ts->driver = driver;
    gles_api_ts->ps = ps;
}

void yagl_gles_api_ts_cleanup(struct yagl_gles_api_ts *gles_api_ts)
{
    uint32_t i;

    for (i = 0; i < gles_api_ts->num_arrays; ++i) {
        yagl_vector_cleanup(&gles_api_ts->arrays[i]);
    }
    g_free(gles_api_ts->arrays);
}
