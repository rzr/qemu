#include "yagl_egl_native_config.h"
#include "yagl_process.h"

void yagl_egl_native_config_init(struct yagl_egl_native_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->surface_type = EGL_WINDOW_BIT;
    cfg->caveat = EGL_DONT_CARE;
    cfg->config_id = EGL_DONT_CARE;
    cfg->max_swap_interval = EGL_DONT_CARE;
    cfg->min_swap_interval = EGL_DONT_CARE;
    cfg->native_visual_type = EGL_DONT_CARE;
    cfg->transparent_type = EGL_NONE;
    cfg->trans_red_val = EGL_DONT_CARE;
    cfg->trans_green_val = EGL_DONT_CARE;
    cfg->trans_blue_val = EGL_DONT_CARE;
}
