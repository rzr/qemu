#include <windows.h>
#include <GL/wglext.h>
#include <wingdi.h>
#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_thread.h"

struct yagl_egl_driver *yagl_egl_wgl_create(void)
{
    return NULL;
}

void *yagl_egl_wgl_procaddr_get(struct yagl_dyn_lib *dyn_lib,
                                       const char *sym)
{
    void *proc = NULL;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_egl_wgl_get_procaddr, NULL);

    YAGL_LOG_FUNC_EXIT("%p", proc);
    return proc;
}
