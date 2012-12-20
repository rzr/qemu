#ifndef YAGL_GLES1_API_TS_H_
#define YAGL_GLES1_API_TS_H_

#include "yagl_types.h"

struct yagl_gles1_driver;

typedef struct YaglGles1ApiTs
{
    struct yagl_gles1_driver *driver;
} YaglGles1ApiTs;

/*
 * Does NOT take ownership of anything.
 */
void yagl_gles1_api_ts_init(YaglGles1ApiTs *gles1_api_ts,
                            struct yagl_gles1_driver *driver);

void yagl_gles1_api_ts_cleanup(YaglGles1ApiTs *gles1_api_ts);

#endif /* YAGL_GLES1_API_TS_H_ */
