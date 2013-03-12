#include "yagl_egl_config.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_display.h"
#include "yagl_resource.h"
#include "yagl_process.h"

static EGLint yagl_egl_config_get_renderable_type(struct yagl_process_state *ps)
{
    EGLint renderable_type = 0;

    if (ps->client_ifaces[yagl_client_api_ogl]) {
        renderable_type |= EGL_OPENGL_BIT;
    }

    if (ps->client_ifaces[yagl_client_api_gles1]) {
        renderable_type |= EGL_OPENGL_ES_BIT;
    }

    if (ps->client_ifaces[yagl_client_api_gles2]) {
        renderable_type |= EGL_OPENGL_ES2_BIT;
    }

    if (ps->client_ifaces[yagl_client_api_ovg]) {
        renderable_type |= EGL_OPENVG_BIT;
    }

    return renderable_type;
}

static void yagl_egl_config_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_config *cfg = (struct yagl_egl_config*)ref;

    cfg->dpy->driver_ps->config_cleanup(cfg->dpy->driver_ps,
                                        cfg->dpy->native_dpy,
                                        &cfg->native);

    yagl_resource_cleanup(&cfg->res);

    g_free(cfg);
}

static bool yagl_egl_config_is_less(struct yagl_egl_native_config *lhs_cfg,
                                    struct yagl_egl_native_config *rhs_cfg)
{
    /*
     * 1. EGL_NONE < EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG
     *    Conformant first.
     */
    if (lhs_cfg->conformant != rhs_cfg->conformant) {
        return lhs_cfg->conformant != 0;
    }

    /*
     * 1. EGL_NONE < EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG
     *    EGL_NONE first.
     */
    if (lhs_cfg->caveat != rhs_cfg->caveat) {
        return lhs_cfg->caveat < rhs_cfg->caveat;
    }

    /*
     * 2, 3. skip
     */

    /*
     * 4. Smaller EGL_BUFFER_SIZE.
     */
    if (lhs_cfg->buffer_size != rhs_cfg->buffer_size) {
        return lhs_cfg->buffer_size < rhs_cfg->buffer_size;
    }

    /*
     * 5. Smaller EGL_SAMPLE_BUFFERS.
     */
    if (lhs_cfg->sample_buffers_num != rhs_cfg->sample_buffers_num) {
        return lhs_cfg->sample_buffers_num <
               rhs_cfg->sample_buffers_num;
    }

    /*
     * 6. Smaller EGL_SAMPLES.
     */
    if (lhs_cfg->samples_per_pixel != rhs_cfg->samples_per_pixel) {
        return lhs_cfg->samples_per_pixel < rhs_cfg->samples_per_pixel;
    }

    /*
     * 7. Smaller EGL_DEPTH_SIZE.
     */
    if (lhs_cfg->depth_size != rhs_cfg->depth_size) {
        return lhs_cfg->depth_size < rhs_cfg->depth_size;
    }

    /*
     * 8. Smaller EGL_STENCIL_SIZE.
     */
    if (lhs_cfg->stencil_size != rhs_cfg->stencil_size) {
        return lhs_cfg->stencil_size < rhs_cfg->stencil_size;
    }

    /*
     * 9. skip
     */

    /*
     * 10. EGL_NATIVE_VISUAL_TYPE
     */

    if (lhs_cfg->native_visual_type != rhs_cfg->native_visual_type) {
        return lhs_cfg->native_visual_type < rhs_cfg->native_visual_type;
    }

    /*
     * 11. Smaller EGL_CONFIG_ID. Last rule.
     */

    return lhs_cfg->config_id < rhs_cfg->config_id;
}

static int yagl_egl_config_sort_func(const void *lhs, const void *rhs)
{
    struct yagl_egl_config *lhs_cfg = *(struct yagl_egl_config**)lhs;
    struct yagl_egl_config *rhs_cfg = *(struct yagl_egl_config**)rhs;

    if (yagl_egl_config_is_less(&lhs_cfg->native, &rhs_cfg->native)) {
        return -1;
    } else {
        return yagl_egl_config_is_less(&rhs_cfg->native, &lhs_cfg->native) ? 1 : 0;
    }
}

/*
 * 'native_cfg' is byte-copied into allocated config.
 */
static struct yagl_egl_config
    *yagl_egl_config_create(struct yagl_egl_display *dpy,
                            const struct yagl_egl_native_config *native_cfg)
{
    struct yagl_egl_config *cfg = g_malloc0(sizeof(struct yagl_egl_config));

    yagl_resource_init(&cfg->res, &yagl_egl_config_destroy);
    cfg->dpy = dpy;
    cfg->native = *native_cfg;

    cfg->native.surface_type = EGL_PBUFFER_BIT |
                               EGL_PIXMAP_BIT |
                               EGL_WINDOW_BIT |
                               EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

    cfg->native.native_renderable = EGL_TRUE;

    cfg->native.renderable_type =
        yagl_egl_config_get_renderable_type(dpy->driver_ps->ps);

    cfg->native.conformant =
        (((cfg->native.red_size + cfg->native.green_size + cfg->native.blue_size + cfg->native.alpha_size) > 0) &&
         (cfg->native.caveat != EGL_NON_CONFORMANT_CONFIG)) ? cfg->native.renderable_type : 0;

    cfg->native.sample_buffers_num = (cfg->native.samples_per_pixel > 0) ? 1 : 0;

    return cfg;
}

struct yagl_egl_config
    **yagl_egl_config_enum(struct yagl_egl_display *dpy,
                           int* num_configs )
{
    struct yagl_egl_native_config *native_configs;
    int num_native_configs, i;
    struct yagl_egl_config **configs;

    native_configs = dpy->driver_ps->config_enum(dpy->driver_ps, dpy->native_dpy, &num_native_configs);

    if (num_native_configs <= 0) {
        return NULL;
    }

    configs = g_malloc0(num_native_configs * sizeof(*configs));

    *num_configs = 0;

    for (i = 0; i < num_native_configs; ++i) {
        /*
         * We only accept RGBA8888, that's to be sure we can handle all kinds
         * of formats.
         */
        if ((native_configs[i].buffer_size != 32) ||
            (native_configs[i].red_size != 8) ||
            (native_configs[i].green_size != 8) ||
            (native_configs[i].blue_size != 8) ||
            (native_configs[i].alpha_size != 8)) {
            dpy->driver_ps->config_cleanup(dpy->driver_ps,
                                           dpy->native_dpy,
                                           &native_configs[i]);
            continue;
        }

        configs[*num_configs] = yagl_egl_config_create(dpy, &native_configs[i]);

        ++*num_configs;
    }

    g_free(native_configs);

    return configs;
}

void yagl_egl_config_sort(struct yagl_egl_config **cfgs, int num_configs)
{
    qsort(cfgs, num_configs, sizeof(*cfgs), &yagl_egl_config_sort_func);
}

#define YAGL_CHECK_ATTRIB(attrib, op) \
    if ((dummy->attrib != EGL_DONT_CARE) && (dummy->attrib op cfg->native.attrib)) { \
        return false; \
    }

#define YAGL_CHECK_ATTRIB_CAST(attrib, op) \
    if (((EGLint)(dummy->attrib) != EGL_DONT_CARE) && (dummy->attrib op cfg->native.attrib)) { \
        return false; \
    }

bool yagl_egl_config_is_chosen_by(const struct yagl_egl_config *cfg,
                                  const struct yagl_egl_native_config *dummy)
{
    /*
     * AtLeast.
     */

    YAGL_CHECK_ATTRIB(red_size, >);
    YAGL_CHECK_ATTRIB(green_size, >);
    YAGL_CHECK_ATTRIB(blue_size, >);
    YAGL_CHECK_ATTRIB(alpha_size, >);
    YAGL_CHECK_ATTRIB(buffer_size, >);
    YAGL_CHECK_ATTRIB(depth_size, >);
    YAGL_CHECK_ATTRIB(stencil_size, >);
    YAGL_CHECK_ATTRIB(samples_per_pixel, >);
    YAGL_CHECK_ATTRIB(sample_buffers_num, >);

    /*
     * Exact.
     */

    YAGL_CHECK_ATTRIB(frame_buffer_level, !=);
    YAGL_CHECK_ATTRIB(config_id, !=);
    YAGL_CHECK_ATTRIB(native_visual_type, !=);
    YAGL_CHECK_ATTRIB(max_swap_interval, !=);
    YAGL_CHECK_ATTRIB(min_swap_interval, !=);
    YAGL_CHECK_ATTRIB(trans_red_val, !=);
    YAGL_CHECK_ATTRIB(trans_green_val, !=);
    YAGL_CHECK_ATTRIB(trans_blue_val, !=);
    /*
     * Exact with cast.
     */
    YAGL_CHECK_ATTRIB_CAST(caveat, !=);
    YAGL_CHECK_ATTRIB_CAST(native_renderable, !=);
    YAGL_CHECK_ATTRIB_CAST(transparent_type, !=);

    /*
     * Mask.
     */

    if (dummy->surface_type != EGL_DONT_CARE &&
       ((dummy->surface_type & cfg->native.surface_type) != dummy->surface_type)) {
        return false;
    }

    if (dummy->conformant != (EGLenum)EGL_DONT_CARE &&
       ((dummy->conformant & cfg->native.conformant) != dummy->conformant)) {
        return false;
    }

    if (dummy->renderable_type != EGL_DONT_CARE &&
       ((dummy->renderable_type & cfg->native.renderable_type) != dummy->renderable_type)) {
        return false;
    }

    return true;
}
bool yagl_egl_config_get_attrib(const struct yagl_egl_config *cfg,
                                EGLint attrib,
                                EGLint *value)
{
    switch (attrib) {
    case EGL_BUFFER_SIZE:
        *value = cfg->native.buffer_size;
        break;
    case EGL_RED_SIZE:
        *value = cfg->native.red_size;
        break;
    case EGL_GREEN_SIZE:
        *value = cfg->native.green_size;
        break;
    case EGL_BLUE_SIZE:
        *value = cfg->native.blue_size;
        break;
    case EGL_ALPHA_SIZE:
        *value = cfg->native.alpha_size;
        break;
    case EGL_BIND_TO_TEXTURE_RGB:
        *value = EGL_FALSE;
        break;
    case EGL_BIND_TO_TEXTURE_RGBA:
        *value = EGL_FALSE;
        break;
    case EGL_CONFIG_CAVEAT:
        *value = cfg->native.caveat;
        break;
    case EGL_CONFIG_ID:
        *value = cfg->native.config_id;
        break;
    case EGL_DEPTH_SIZE:
        *value = cfg->native.depth_size;
        break;
    case EGL_LEVEL:
        *value = cfg->native.frame_buffer_level;
        break;
    case EGL_MAX_PBUFFER_WIDTH:
        *value = cfg->native.max_pbuffer_width;
        break;
    case EGL_MAX_PBUFFER_HEIGHT:
        *value = cfg->native.max_pbuffer_height;
        break;
    case EGL_MAX_PBUFFER_PIXELS:
        *value = cfg->native.max_pbuffer_size;
        break;
    case EGL_MAX_SWAP_INTERVAL:
        *value = cfg->native.max_swap_interval;
        break;
    case EGL_MIN_SWAP_INTERVAL:
        *value = cfg->native.min_swap_interval;
        break;
    case EGL_NATIVE_RENDERABLE:
        *value = cfg->native.native_renderable;
        break;
    case EGL_NATIVE_VISUAL_ID:
        *value = cfg->native.native_visual_id;
        break;
    case EGL_NATIVE_VISUAL_TYPE:
        *value = cfg->native.native_visual_type;
        break;
    case EGL_RENDERABLE_TYPE:
        *value = cfg->native.renderable_type;
        break;
    case EGL_SAMPLE_BUFFERS:
        *value = cfg->native.sample_buffers_num;
        break;
    case EGL_SAMPLES:
        *value = cfg->native.samples_per_pixel;
        break;
    case EGL_STENCIL_SIZE:
        *value = cfg->native.stencil_size;
        break;
    case EGL_SURFACE_TYPE:
        *value = cfg->native.surface_type;
        break;
    case EGL_TRANSPARENT_TYPE:
        *value = cfg->native.transparent_type;
        break;
    case EGL_TRANSPARENT_RED_VALUE:
        *value = cfg->native.trans_red_val;
        break;
    case EGL_TRANSPARENT_GREEN_VALUE:
        *value = cfg->native.trans_green_val;
        break;
    case EGL_TRANSPARENT_BLUE_VALUE:
        *value = cfg->native.trans_blue_val;
        break;
    case EGL_CONFORMANT:
        *value = cfg->native.conformant;
        break;
    case EGL_COLOR_BUFFER_TYPE:
        *value = EGL_RGB_BUFFER;
        break;
    default:
        return false;
    }
    return true;
}

void yagl_egl_config_acquire(struct yagl_egl_config *cfg)
{
    if (cfg) {
        yagl_resource_acquire(&cfg->res);
    }
}

void yagl_egl_config_release(struct yagl_egl_config *cfg)
{
    if (cfg) {
        yagl_resource_release(&cfg->res);
    }
}
