#include "yagl_mem_egl.h"
#include "yagl_vector.h"
#include "yagl_log.h"
#include "yagl_thread.h"
#include "yagl_process.h"

EGLint *yagl_mem_get_attrib_list(struct yagl_thread_state *ts,
                                 target_ulong va)
{
    struct yagl_vector v;
    int i = 0;

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_attrib_list, "va = 0x%X", (uint32_t)va);

    yagl_vector_init(&v, sizeof(EGLint), 0);

    while (true) {
        EGLint tmp;

        if (!yagl_mem_get_EGLint(ts, va + (i * sizeof(EGLint)), &tmp)) {
            goto fail;
        }

        yagl_vector_push_back(&v, &tmp);

        ++i;

        if (tmp == EGL_NONE) {
            break;
        }

        if (!yagl_mem_get_EGLint(ts, va + (i * sizeof(EGLint)), &tmp)) {
            goto fail;
        }

        yagl_vector_push_back(&v, &tmp);

        ++i;
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return yagl_vector_detach(&v);

fail:
    yagl_vector_cleanup(&v);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
