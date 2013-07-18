#include "vigs_utils.h"
#include "vigs_log.h"

uint32_t vigs_format_bpp(vigsp_surface_format format)
{
    switch (format) {
    case vigsp_surface_bgrx8888: return 4;
    case vigsp_surface_bgra8888: return 4;
    default:
        assert(false);
        VIGS_LOG_CRITICAL("unknown format: %d", format);
        exit(1);
        return 0;
    }
}
