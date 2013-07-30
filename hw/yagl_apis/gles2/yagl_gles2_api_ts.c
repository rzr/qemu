#include "yagl_gles2_api_ts.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_gles2_api_ts_init(struct yagl_gles2_api_ts *gles2_api_ts,
                            struct yagl_thread_state *ts,
                            struct yagl_gles2_driver_ps *driver_ps)
{
    gles2_api_ts->driver_ps = driver_ps;
    gles2_api_ts->ts = ts;
    gles2_api_ts->egl_iface = ts->ps->egl_iface;
}

void yagl_gles2_api_ts_cleanup(struct yagl_gles2_api_ts *gles2_api_ts)
{
}
