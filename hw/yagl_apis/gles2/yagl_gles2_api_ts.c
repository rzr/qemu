#include "yagl_gles2_api_ts.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_gles2_api_ts_init(struct yagl_gles2_api_ts *gles2_api_ts,
                            struct yagl_gles2_driver *driver)
{
    gles2_api_ts->driver = driver;
}

void yagl_gles2_api_ts_cleanup(struct yagl_gles2_api_ts *gles2_api_ts)
{
}
