#include "vigs_comm.h"
#include "vigs_log.h"

/*
 * Protocol command handlers go here.
 * @{
 */

static vigsp_status vigs_comm_dispatch_init(struct vigs_comm *comm,
                                            struct vigsp_cmd_init_request *request,
                                            struct vigsp_cmd_init_response *response)
{
    response->server_version = VIGS_PROTOCOL_VERSION;

    if (request->client_version != VIGS_PROTOCOL_VERSION) {
        VIGS_LOG_CRITICAL("protocol version mismatch, expected %u, actual %u",
                          VIGS_PROTOCOL_VERSION,
                          request->client_version);
        return vigsp_status_success;
    }

    VIGS_LOG_TRACE("client_version = %d", request->client_version);

    if (comm->comm_ops->init(comm->user_data)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_reset(struct vigs_comm *comm,
                                             void *response)
{
    VIGS_LOG_TRACE("enter");

    comm->comm_ops->reset(comm->user_data);

    return vigsp_status_success;
}

static vigsp_status vigs_comm_dispatch_exit(struct vigs_comm *comm,
                                            void *response)
{
    VIGS_LOG_TRACE("enter");

    comm->comm_ops->exit(comm->user_data);

    return vigsp_status_success;
}

static void vigs_comm_dispatch_create_surface(struct vigs_comm *comm,
                                              struct vigsp_cmd_create_surface_request *request)
{
    switch (request->format) {
    case vigsp_surface_bgrx8888:
    case vigsp_surface_bgra8888:
        break;
    default:
        VIGS_LOG_CRITICAL("bad surface format = %d", request->format);
        return;
    }

    VIGS_LOG_TRACE("%ux%u, strd = %u, fmt = %d, id = %u",
                   request->width,
                   request->height,
                   request->stride,
                   request->format,
                   request->id);

    comm->comm_ops->create_surface(comm->user_data,
                                   request->width,
                                   request->height,
                                   request->stride,
                                   request->format,
                                   request->id);
}

static void vigs_comm_dispatch_destroy_surface(struct vigs_comm *comm,
                                               struct vigsp_cmd_destroy_surface_request *request)
{
    VIGS_LOG_TRACE("id = %u", request->id);

    comm->comm_ops->destroy_surface(comm->user_data, request->id);
}

static void vigs_comm_dispatch_set_root_surface(struct vigs_comm *comm,
                                                struct vigsp_cmd_set_root_surface_request *request)
{
    VIGS_LOG_TRACE("id = %u, offset = %u",
                   request->id,
                   request->offset);

    comm->comm_ops->set_root_surface(comm->user_data,
                                     request->id,
                                     request->offset);
}

static void vigs_comm_dispatch_update_vram(struct vigs_comm *comm,
                                           struct vigsp_cmd_update_vram_request *request)
{
    if (request->sfc_id == 0) {
        VIGS_LOG_TRACE("skipped");
        return;
    } else {
        VIGS_LOG_TRACE("sfc = %u(off = %u)",
                       request->sfc_id,
                       request->offset);
    }

    comm->comm_ops->update_vram(comm->user_data,
                                request->sfc_id,
                                request->offset);
}

static void vigs_comm_dispatch_update_gpu(struct vigs_comm *comm,
                                          struct vigsp_cmd_update_gpu_request *request)
{
    if (request->sfc_id == 0) {
        VIGS_LOG_TRACE("skipped");
        return;
    } else {
        VIGS_LOG_TRACE("sfc = %u(off = %u)",
                       request->sfc_id,
                       request->offset);
    }

    comm->comm_ops->update_gpu(comm->user_data,
                               request->sfc_id,
                               request->offset,
                               &request->entries[0],
                               request->num_entries);
}

static void vigs_comm_dispatch_copy(struct vigs_comm *comm,
                                    struct vigsp_cmd_copy_request *request)
{
    VIGS_LOG_TRACE("src = %u, dst = %u",
                   request->src_id,
                   request->dst_id);

    comm->comm_ops->copy(comm->user_data,
                         request->src_id,
                         request->dst_id,
                         &request->entries[0],
                         request->num_entries);
}

static void vigs_comm_dispatch_solid_fill(struct vigs_comm *comm,
                                          struct vigsp_cmd_solid_fill_request *request)
{
    VIGS_LOG_TRACE("sfc = %u, color = 0x%X",
                   request->sfc_id,
                   request->color);

    comm->comm_ops->solid_fill(comm->user_data,
                               request->sfc_id,
                               request->color,
                               &request->entries[0],
                               request->num_entries);
}

/*
 * @}
 */

#define VIGS_DISPATCH_ENTRY(cmd, func, has_request, has_response) \
    [cmd] = { func, has_request, has_response }

struct vigs_dispatch_entry
{
    void *func;
    bool has_request;
    bool has_response;
};

static const struct vigs_dispatch_entry vigs_dispatch_table[] =
{
    VIGS_DISPATCH_ENTRY(vigsp_cmd_init,
                        vigs_comm_dispatch_init, true, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_reset,
                        vigs_comm_dispatch_reset, false, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_exit,
                        vigs_comm_dispatch_exit, false, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_create_surface,
                        vigs_comm_dispatch_create_surface, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_destroy_surface,
                        vigs_comm_dispatch_destroy_surface, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_set_root_surface,
                        vigs_comm_dispatch_set_root_surface, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_update_vram,
                        vigs_comm_dispatch_update_vram, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_update_gpu,
                        vigs_comm_dispatch_update_gpu, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_copy,
                        vigs_comm_dispatch_copy, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_solid_fill,
                        vigs_comm_dispatch_solid_fill, true, false)
};

struct vigs_comm *vigs_comm_create(uint8_t *ram_ptr,
                                   struct vigs_comm_ops *comm_ops,
                                   void *user_data)
{
    struct vigs_comm *comm;

    comm = g_malloc0(sizeof(*comm));

    comm->ram_ptr = ram_ptr;
    comm->comm_ops = comm_ops;
    comm->user_data = user_data;

    return comm;
}

void vigs_comm_destroy(struct vigs_comm *comm)
{
    g_free(comm);
}

void vigs_comm_dispatch(struct vigs_comm *comm,
                        uint32_t ram_offset)
{
    struct vigsp_cmd_batch_header *batch_header =
        (struct vigsp_cmd_batch_header*)(comm->ram_ptr + ram_offset);
    struct vigsp_cmd_request_header *request_header =
        (struct vigsp_cmd_request_header*)(batch_header + 1);
    struct vigsp_cmd_response_header *response_header;
    vigsp_u32 i;
    vigsp_status status = vigsp_status_success;

    VIGS_LOG_TRACE("batch_start");

    comm->comm_ops->batch_start(comm->user_data);

    for (i = 0; i < batch_header->num_requests; ++i) {
        response_header =
            (struct vigsp_cmd_response_header*)((uint8_t*)(request_header + 1) +
            request_header->size);

        if (status == vigsp_status_success) {
            if (request_header->cmd >= ARRAY_SIZE(vigs_dispatch_table)) {
                VIGS_LOG_CRITICAL("bad command = %d", request_header->cmd);
                status = vigsp_status_bad_call;
            } else {
                const struct vigs_dispatch_entry *dispatch_entry =
                    &vigs_dispatch_table[request_header->cmd];

                if (dispatch_entry->has_response && (i != (batch_header->num_requests - 1))) {
                    VIGS_LOG_CRITICAL("only last request in a batch is allowed to have response, bad command = %d",
                                      request_header->cmd);
                    status = vigsp_status_bad_call;
                } else {
                    if (dispatch_entry->has_request && dispatch_entry->has_response) {
                        vigsp_status (*func)(struct vigs_comm*, void*, void*) =
                            dispatch_entry->func;
                        status = func(comm, request_header + 1, response_header + 1);
                    } else if (dispatch_entry->has_request) {
                        void (*func)(struct vigs_comm*, void*) =
                            dispatch_entry->func;
                        func(comm, request_header + 1);
                    } else if (dispatch_entry->has_response) {
                        vigsp_status (*func)(struct vigs_comm*, void*) =
                            dispatch_entry->func;
                        status = func(comm, response_header + 1);
                    } else {
                        void (*func)(struct vigs_comm*) =
                            dispatch_entry->func;
                        func(comm);
                    }
                }
            }
        }

        request_header = (struct vigsp_cmd_request_header*)response_header;
    }

    response_header = (struct vigsp_cmd_response_header*)request_header;

    response_header->status = status;

    VIGS_LOG_TRACE("batch_end");

    comm->comm_ops->batch_end(comm->user_data);
}
