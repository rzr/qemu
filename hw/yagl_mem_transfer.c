#include "yagl_mem_transfer.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_log.h"
#include "cpu-all.h"

struct yagl_mem_transfer
    *yagl_mem_transfer_create(struct yagl_thread_state *ts)
{
    struct yagl_mem_transfer *mt;

    mt = g_malloc0(sizeof(*mt));

    mt->ts = ts;

    return mt;
}

void yagl_mem_transfer_destroy(struct yagl_mem_transfer *mt)
{
    g_free(mt->pages);
    g_free(mt);
}

bool yagl_mem_prepare(struct yagl_mem_transfer *mt,
                      target_ulong va,
                      int len)
{
    bool res = true;
    int l;
    target_phys_addr_t page_pa;
    target_ulong page_va;

    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_prepare, "va = 0x%X, len = %d", (uint32_t)va, len);

    assert(mt->ts->current_env);

    if (len >= 0) {
        int max_pages = ((len + TARGET_PAGE_SIZE - 1) / TARGET_PAGE_SIZE) + 1;

        if (max_pages > mt->max_pages) {
            g_free(mt->pages);
            mt->pages = g_malloc(sizeof(*mt->pages) * max_pages);
        }

        mt->max_pages = max_pages;
    }

    mt->va = va;
    mt->offset = (va & ~TARGET_PAGE_MASK);
    mt->len = len;
    mt->num_pages = 0;

    if (va) {
        while (len > 0) {
            page_va = va & TARGET_PAGE_MASK;
            page_pa = cpu_get_phys_page_debug(mt->ts->current_env, page_va);

            if (page_pa == -1) {
                YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)page_va);
                res = false;
                break;
            }

            l = (page_va + TARGET_PAGE_SIZE) - va;
            if (l > len) {
                l = len;
            }

            len -= l;
            va += l;

            assert(mt->num_pages < mt->max_pages);

            mt->pages[mt->num_pages++] = page_pa;
        }
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return res;
}

static void yagl_mem_put_internal(struct yagl_mem_transfer *mt, const void *data)
{
    int len = mt->len, offset = mt->offset, i = 0;

    if (!mt->va) {
        assert(false);
        YAGL_LOG_CRITICAL("mt->va is 0");
        return;
    }

    while (len > 0) {
        int l = MIN(len, (TARGET_PAGE_SIZE - offset));

        assert(i < mt->num_pages);

        cpu_physical_memory_write_rom(mt->pages[i++] + offset, data, l);

        offset = 0;

        data += l;
        len -= l;
    }
}

void yagl_mem_put(struct yagl_mem_transfer *mt, const void *data)
{
    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_put, "va = 0X%X, data = %p", (uint32_t)mt->va, data);

    yagl_mem_put_internal(mt, data);

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_mem_put_uint8(struct yagl_mem_transfer *mt, uint8_t value)
{
    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_put_uint8, "va = 0x%X, value = 0x%X", (uint32_t)mt->va, (uint32_t)value);

    yagl_mem_put_internal(mt, &value);

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_mem_put_uint16(struct yagl_mem_transfer *mt, uint16_t value)
{
    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_put_uint16, "va = 0x%X, value = 0x%X", (uint32_t)mt->va, (uint32_t)value);

    yagl_mem_put_internal(mt, &value);

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_mem_put_uint32(struct yagl_mem_transfer *mt, uint32_t value)
{
    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_put_uint32, "va = 0x%X, value = 0x%X", (uint32_t)mt->va, value);

    yagl_mem_put_internal(mt, &value);

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_mem_put_float(struct yagl_mem_transfer *mt, float value)
{
    YAGL_LOG_FUNC_ENTER_TS(mt->ts, yagl_mem_put_float, "va = 0x%X, value = %f", (uint32_t)mt->va, value);

    yagl_mem_put_internal(mt, &value);

    YAGL_LOG_FUNC_EXIT(NULL);
}
