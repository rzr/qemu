#include "yagl_compiled_transfer.h"
#include "yagl_vector.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_log.h"
#include "cpu-all.h"

#define YAGL_TARGET_PAGE_VA(addr) ((addr) & ~(TARGET_PAGE_SIZE - 1))

#define YAGL_TARGET_PAGE_OFFSET(addr) ((addr) & (TARGET_PAGE_SIZE - 1))

static target_phys_addr_t yagl_pa(struct yagl_thread_state *ts,
                                  target_ulong va)
{
    target_phys_addr_t ret =
        cpu_get_phys_page_debug(ts->current_env, va);

    if (ret == -1) {
        return 0;
    } else {
        return ret;
    }
}

struct yagl_compiled_transfer
    *yagl_compiled_transfer_create(struct yagl_thread_state *ts,
                                   target_ulong va,
                                   uint32_t len,
                                   bool is_write)
{
    struct yagl_compiled_transfer *ct;
    struct yagl_vector v;
    target_ulong last_page_va = YAGL_TARGET_PAGE_VA(va + len - 1);
    target_ulong cur_va = va;
    int i, num_sections;

    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_compiled_transfer_init,
                           "va = 0x%X, len = 0x%X, is_write = %u",
                           (uint32_t)va,
                           len,
                           (uint32_t)is_write);

    yagl_vector_init(&v, sizeof(struct yagl_compiled_transfer_section), 0);

    while (len) {
        target_ulong start_page_va = YAGL_TARGET_PAGE_VA(cur_va);
        target_phys_addr_t start_page_pa = yagl_pa(ts, start_page_va);
        target_ulong end_page_va;
        struct yagl_compiled_transfer_section section;

        if (!start_page_pa) {
            YAGL_LOG_ERROR("yagl_pa of va 0x%X failed", (uint32_t)start_page_va);
            goto fail;
        }

        end_page_va = start_page_va;

        while (end_page_va < last_page_va) {
            target_ulong next_page_va = end_page_va + TARGET_PAGE_SIZE;
            target_phys_addr_t next_page_pa = yagl_pa(ts, next_page_va);

            if (!next_page_pa) {
                YAGL_LOG_ERROR("yagl_pa of va 0x%X failed", (uint32_t)next_page_va);
                goto fail;
            }

            /*
             * If the target pages are not linearly spaced, stop.
             */

            if ((next_page_pa < start_page_pa) ||
                ((next_page_pa - start_page_pa) >
                 (next_page_va - start_page_va))) {
                break;
            }

            end_page_va = next_page_va;
        }

        section.map_len = end_page_va + TARGET_PAGE_SIZE - start_page_va;
        section.map_base = cpu_physical_memory_map(start_page_pa, &section.map_len, 0);

        if (!section.map_base || !section.map_len) {
            YAGL_LOG_ERROR("cpu_physical_memory_map(0x%X, %u) failed",
                           (uint32_t)start_page_pa,
                           (uint32_t)section.map_len);
            goto fail;
        }

        section.len = end_page_va + TARGET_PAGE_SIZE - cur_va;

        if (section.len > len) {
            section.len = len;
        }

        section.base = (char*)section.map_base + YAGL_TARGET_PAGE_OFFSET(cur_va);

        yagl_vector_push_back(&v, &section);

        len -= section.len;
        cur_va += section.len;
    }

    ct = g_malloc0(sizeof(*ct));

    ct->num_sections = yagl_vector_size(&v);
    ct->sections = yagl_vector_detach(&v);
    ct->is_write = is_write;

    YAGL_LOG_FUNC_EXIT("num_sections = %d", ct->num_sections);

    return ct;

fail:
    num_sections = yagl_vector_size(&v);

    for (i = 0; i < num_sections; ++i) {
        struct yagl_compiled_transfer_section *section =
            (struct yagl_compiled_transfer_section*)
                ((char*)yagl_vector_data(&v) +
                 (i * sizeof(struct yagl_compiled_transfer_section)));

        cpu_physical_memory_unmap(section->map_base,
                                  section->map_len,
                                  0,
                                  section->map_len);
    }

    yagl_vector_cleanup(&v);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void yagl_compiled_transfer_destroy(struct yagl_compiled_transfer *ct)
{
    int i;

    for (i = 0; i < ct->num_sections; ++i) {
        cpu_physical_memory_unmap(ct->sections[i].map_base,
                                  ct->sections[i].map_len,
                                  0,
                                  ct->sections[i].map_len);
    }

    g_free(ct->sections);

    ct->sections = NULL;
    ct->num_sections = 0;
    ct->is_write = false;

    g_free(ct);
}

void yagl_compiled_transfer_exec(struct yagl_compiled_transfer *ct, void* data)
{
    int i;

    for (i = 0; i < ct->num_sections; ++i) {
        void *base = ct->sections[i].base;
        uint32_t len = ct->sections[i].len;

        if (ct->is_write) {
            memcpy(base, data, len);
        } else {
            memcpy(data, base, len);
        }

        data = (char*)data + len;
    }
}
