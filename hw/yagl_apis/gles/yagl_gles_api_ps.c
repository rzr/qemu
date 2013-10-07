#include <GL/gl.h>
#include "yagl_gles_api_ps.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_avl.h"

struct yagl_location
{
    uint32_t location;

    GLint actual_location;
};

static int yagl_location_compare(const void *avl_a,
                                 const void *avl_b,
                                 void *avl_param)
{
    const struct yagl_location *a = avl_a;
    const struct yagl_location *b = avl_b;

    if (a->location < b->location) {
        return -1;
    } else if (a->location > b->location) {
        return 1;
    } else {
        return 0;
    }
}

static void yagl_location_destroy(void *avl_item, void *avl_param)
{
    g_free(avl_item);
}

void yagl_gles_api_ps_init(struct yagl_gles_api_ps *gles_api_ps,
                           struct yagl_gles_driver *driver)
{
    gles_api_ps->driver = driver;
    gles_api_ps->locations = yagl_avl_create(&yagl_location_compare,
                                             NULL,
                                             NULL);
    assert(gles_api_ps->locations);
}

void yagl_gles_api_ps_cleanup(struct yagl_gles_api_ps *gles_api_ps)
{
    yagl_avl_destroy(gles_api_ps->locations, &yagl_location_destroy);
}

void yagl_gles_api_ps_add_location(struct yagl_gles_api_ps *gles_api_ps,
                                   uint32_t location,
                                   GLint actual_location)
{
    struct yagl_location *item = g_malloc0(sizeof(struct yagl_location));

    item->location = location;
    item->actual_location = actual_location;

    yagl_avl_assert_insert(gles_api_ps->locations, item);
}

GLint yagl_gles_api_ps_translate_location(struct yagl_gles_api_ps *gles_api_ps,
                                          GLboolean tl,
                                          uint32_t location)
{
    struct yagl_location *item;
    struct yagl_location dummy;

    if (!tl) {
        return location;
    }

    dummy.location = location;

    item = yagl_avl_find(gles_api_ps->locations, &dummy);

    if (item) {
        return item->actual_location;
    } else {
        assert(0);
        return -1;
    }
}

void yagl_gles_api_ps_remove_location(struct yagl_gles_api_ps *gles_api_ps,
                                      uint32_t location)
{
    void *item;
    struct yagl_location dummy;

    dummy.location = location;

    item = yagl_avl_assert_delete(gles_api_ps->locations, &dummy);

    yagl_location_destroy(item, gles_api_ps->locations->avl_param);
}
