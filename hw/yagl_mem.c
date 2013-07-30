#include "yagl_mem.h"
#include "yagl_log.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_vector.h"

bool yagl_mem_get_uint8(struct yagl_thread_state *ts, target_ulong va, uint8_t* value)
{
    int ret;

    assert(ts->current_env);

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_uint8, "va = 0x%X", (uint32_t)va);

    ret = cpu_memory_rw_debug(ts->current_env, va, value, sizeof(*value), 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va);

        YAGL_LOG_FUNC_EXIT(NULL);
    } else {
        YAGL_LOG_FUNC_EXIT("0x%X", (uint32_t)*value);
    }

    return ret != -1;
}

bool yagl_mem_get_uint16(struct yagl_thread_state *ts, target_ulong va, uint16_t* value)
{
    int ret;

    assert(ts->current_env);

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_uint16, "va = 0x%X", (uint32_t)va);

    ret = cpu_memory_rw_debug(ts->current_env, va, (uint8_t*)value, sizeof(*value), 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va);

        YAGL_LOG_FUNC_EXIT(NULL);
    } else {
        YAGL_LOG_FUNC_EXIT("0x%X", (uint32_t)*value);
    }

    return ret != -1;
}

bool yagl_mem_get_uint32(struct yagl_thread_state *ts, target_ulong va, uint32_t* value)
{
    int ret;

    assert(ts->current_env);

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_uint32, "va = 0x%X", (uint32_t)va);

    ret = cpu_memory_rw_debug(ts->current_env, va, (uint8_t*)value, sizeof(*value), 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va);

        YAGL_LOG_FUNC_EXIT(NULL);
    } else {
        YAGL_LOG_FUNC_EXIT("0x%X", *value);
    }

    return ret != -1;
}

bool yagl_mem_get_float(struct yagl_thread_state *ts, target_ulong va, float* value)
{
    int ret;

    assert(ts->current_env);

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_float, "va = 0x%X", (uint32_t)va);

    ret = cpu_memory_rw_debug(ts->current_env, va, (uint8_t*)value, sizeof(*value), 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va);

        YAGL_LOG_FUNC_EXIT(NULL);
    } else {
        YAGL_LOG_FUNC_EXIT("%f", *value);
    }

    return ret != -1;
}

bool yagl_mem_get(struct yagl_thread_state *ts, target_ulong va, uint32_t len, void* data)
{
    int ret;

    assert(ts->current_env);

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get, "va = 0x%X, len = %u", (uint32_t)va, len);

    ret = cpu_memory_rw_debug(ts->current_env, va, data, len, 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va, len);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return ret != -1;
}

bool yagl_mem_get_array(struct yagl_thread_state *ts,
                        target_ulong va,
                        target_ulong el_size,
                        yagl_mem_array_cb cb,
                        void *user_data)
{
    uint8_t buff[TARGET_PAGE_SIZE];
    target_ulong rem = 0;
    bool res = true;

    assert(ts->current_env);
    assert(el_size <= sizeof(buff));

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_mem_get_array, "va = 0x%X, el_size = %u",
                           (uint32_t)va, (uint32_t)el_size);

    while (true) {
        target_ulong i;
        target_ulong len = TARGET_PAGE_SIZE - (va & (TARGET_PAGE_SIZE - 1));
        len = MIN(len, (sizeof(buff) - rem));

        if (cpu_memory_rw_debug(ts->current_env,
                                va,
                                &buff[rem],
                                len,
                                0) == -1) {
            YAGL_LOG_WARN("page fault at 0x%X:%u", (uint32_t)va, (uint32_t)len);
            res = false;
            break;
        }

        rem += len;
        va += len;

        if (rem < el_size) {
            YAGL_LOG_WARN("array element crosses page boundaries ?");
            continue;
        }

        assert(rem >= el_size);

        len = (rem / el_size);

        for (i = 0; i < len; ++i) {
            if (!cb(&buff[0] + (el_size * i), user_data)) {
                goto out;
            }
        }

        rem %= el_size;

        if (rem != 0) {
            YAGL_LOG_WARN("array element crosses page boundaries ?");
            memmove(&buff[0], &buff[len * el_size], rem);
        }
    }

out:
    YAGL_LOG_FUNC_EXIT(NULL);

    return res;
}

static bool yagl_mem_get_string_cb(const void *el, void *user_data)
{
    const char *c = el;
    struct yagl_vector *v = user_data;

    yagl_vector_push_back(v, c);

    return (*c != 0);
}

char *yagl_mem_get_string(struct yagl_thread_state *ts, target_ulong va)
{
    struct yagl_vector v;

    yagl_vector_init(&v, sizeof(char), 0);

    if (yagl_mem_get_array(ts, va, sizeof(char),
                           &yagl_mem_get_string_cb,
                           &v)) {
        return yagl_vector_detach(&v);
    } else {
        yagl_vector_cleanup(&v);
        return NULL;
    }
}
