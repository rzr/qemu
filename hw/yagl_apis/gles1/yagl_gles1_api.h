#ifndef YAGL_GLES1_API_H_
#define YAGL_GLES1_API_H_

#include "yagl_api.h"

struct yagl_gles1_driver;

typedef struct YaglGles1API
{
    struct yagl_api base;

    struct yagl_gles1_driver *driver;
} YaglGles1API;

/*
 * Takes ownership of 'driver'
 */
struct yagl_api *yagl_gles1_api_create(struct yagl_gles1_driver *driver);


#endif /* YAGL_GLES1_API_H_ */
