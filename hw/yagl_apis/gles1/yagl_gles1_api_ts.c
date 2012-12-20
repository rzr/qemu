#include "yagl_gles1_api_ts.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_gles1_api_ts_init(YaglGles1ApiTs *gles1_api_ts,
                            struct yagl_gles1_driver *driver)
{
    gles1_api_ts->driver = driver;
}

void yagl_gles1_api_ts_cleanup(YaglGles1ApiTs *gles1_api_ts)
{
}
