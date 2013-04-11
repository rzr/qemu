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

static vigsp_status vigs_comm_dispatch_reset(struct vigs_comm *comm)
{
    VIGS_LOG_TRACE("enter");

    if (comm->comm_ops->reset(comm->user_data)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_exit(struct vigs_comm *comm)
{
    VIGS_LOG_TRACE("enter");

    if (comm->comm_ops->exit(comm->user_data)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_create_surface(struct vigs_comm *comm,
                                                      struct vigsp_cmd_create_surface_request *request,
                                                      struct vigsp_cmd_create_surface_response *response)
{
    switch (request->format) {
    case vigsp_surface_bgrx8888:
    case vigsp_surface_bgra8888:
        break;
    default:
        VIGS_LOG_CRITICAL("bad surface format = %d", request->format);
        return vigsp_status_bad_call;
        break;
    }

    VIGS_LOG_TRACE("%ux%u, strd = %u, fmt = %d, off = %d",
                   request->width,
                   request->height,
                   request->stride,
                   request->format,
                   request->vram_offset);

    if (comm->comm_ops->create_surface(comm->user_data,
                                       request->width,
                                       request->height,
                                       request->stride,
                                       request->format,
                                       request->vram_offset,
                                       &response->id)) {
        VIGS_LOG_TRACE("id = %u", response->id);
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_destroy_surface(struct vigs_comm *comm,
                                                       struct vigsp_cmd_destroy_surface_request *request)
{
    VIGS_LOG_TRACE("id = %u", request->id);

    if (comm->comm_ops->destroy_surface(comm->user_data, request->id)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_set_root_surface(struct vigs_comm *comm,
                                                        struct vigsp_cmd_set_root_surface_request *request)
{
    VIGS_LOG_TRACE("id = %u", request->id);

    if (comm->comm_ops->set_root_surface(comm->user_data, request->id)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_copy(struct vigs_comm *comm,
                                            struct vigsp_cmd_copy_request *request)
{
    VIGS_LOG_TRACE("src = %u(off = %d), dst = %u(off = %d)",
                   request->src.id,
                   request->src.vram_offset,
                   request->dst.id,
                   request->dst.vram_offset);

    if (comm->comm_ops->copy(comm->user_data,
                             &request->src,
                             &request->dst,
                             &request->entries[0],
                             request->num_entries)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_solid_fill(struct vigs_comm *comm,
                                                  struct vigsp_cmd_solid_fill_request *request)
{
    VIGS_LOG_TRACE("sfc = %u(off = %d), color = 0x%X",
                   request->sfc.id,
                   request->sfc.vram_offset,
                   request->color);

    if (comm->comm_ops->solid_fill(comm->user_data,
                                   &request->sfc,
                                   request->color,
                                   &request->entries[0],
                                   request->num_entries)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_update_vram(struct vigs_comm *comm,
                                                   struct vigsp_cmd_update_vram_request *request)
{
    if (request->sfc.vram_offset < 0) {
        VIGS_LOG_CRITICAL("vram_offset must be >= 0");
        return vigsp_status_bad_call;
    }

    VIGS_LOG_TRACE("sfc = %u(off = %d)",
                   request->sfc.id,
                   request->sfc.vram_offset);

    if (comm->comm_ops->update_vram(comm->user_data,
                                    &request->sfc)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_put_image(struct vigs_comm *comm,
                                                 struct vigsp_cmd_put_image_request *request,
                                                 struct vigsp_cmd_put_image_response *response)
{
    bool is_pf = false;

    VIGS_LOG_TRACE("sfc = %u(off = %d), src_va = 0x%X, src_strd = %u",
                   request->sfc.id,
                   request->sfc.vram_offset,
                   (uint32_t)request->src_va,
                   request->src_stride);

    if (comm->comm_ops->put_image(comm->user_data,
                                  &request->sfc,
                                  request->src_va,
                                  request->src_stride,
                                  &request->rect,
                                  &is_pf)) {
        response->is_pf = is_pf;
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_get_image(struct vigs_comm *comm,
                                                 struct vigsp_cmd_get_image_request *request,
                                                 struct vigsp_cmd_get_image_response *response)
{
    bool is_pf = false;

    VIGS_LOG_TRACE("sfc = %u, dst_va = 0x%X, dst_strd = %u",
                   request->sfc_id,
                   (uint32_t)request->dst_va,
                   request->dst_stride);

    if (comm->comm_ops->get_image(comm->user_data,
                                  request->sfc_id,
                                  request->dst_va,
                                  request->dst_stride,
                                  &request->rect,
                                  &is_pf)) {
        response->is_pf = is_pf;
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_assign_resource(struct vigs_comm *comm,
                                                       struct vigsp_cmd_assign_resource_request *request)
{
    switch (request->res_type) {
    case vigsp_resource_window:
    case vigsp_resource_pixmap:
        break;
    default:
        VIGS_LOG_CRITICAL("bad resource type = %d", request->res_type);
        return vigsp_status_bad_call;
        break;
    }

    VIGS_LOG_TRACE("res_id = 0x%X, res_type = %d, sfc_id = %u",
                   request->res_id,
                   request->res_type,
                   request->sfc_id);

    if (comm->comm_ops->assign_resource(comm->user_data,
                                        request->res_id,
                                        request->res_type,
                                        request->sfc_id)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
}

static vigsp_status vigs_comm_dispatch_destroy_resource(struct vigs_comm *comm,
                                                        struct vigsp_cmd_destroy_resource_request *request)
{
    VIGS_LOG_TRACE("id = 0x%X", request->id);

    if (comm->comm_ops->destroy_resource(comm->user_data,
                                         request->id)) {
        return vigsp_status_success;
    } else {
        return vigsp_status_exec_error;
    }
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
                        vigs_comm_dispatch_reset, false, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_exit,
                        vigs_comm_dispatch_exit, false, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_create_surface,
                        vigs_comm_dispatch_create_surface, true, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_destroy_surface,
                        vigs_comm_dispatch_destroy_surface, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_set_root_surface,
                        vigs_comm_dispatch_set_root_surface, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_copy,
                        vigs_comm_dispatch_copy, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_solid_fill,
                        vigs_comm_dispatch_solid_fill, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_update_vram,
                        vigs_comm_dispatch_update_vram, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_put_image,
                        vigs_comm_dispatch_put_image, true, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_get_image,
                        vigs_comm_dispatch_get_image, true, true),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_assign_resource,
                        vigs_comm_dispatch_assign_resource, true, false),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_destroy_resource,
                        vigs_comm_dispatch_destroy_resource, true, false),
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
    struct vigsp_cmd_request_header *request_header;
    struct vigsp_cmd_response_header *response_header;
    vigsp_status status;

    request_header = (struct vigsp_cmd_request_header*)(comm->ram_ptr + ram_offset);
    response_header = (struct vigsp_cmd_response_header*)((uint8_t*)(request_header + 1) + request_header->response_offset);

    if (request_header->cmd >= ARRAY_SIZE(vigs_dispatch_table)) {
        VIGS_LOG_CRITICAL("bad command = %d", request_header->cmd);
        status = vigsp_status_bad_call;
    } else {
        const struct vigs_dispatch_entry *dispatch_entry =
            &vigs_dispatch_table[request_header->cmd];

        if (dispatch_entry->has_request && dispatch_entry->has_response) {
            vigsp_status (*func)(struct vigs_comm*, void*, void*) =
                dispatch_entry->func;
            status = func(comm, request_header + 1, response_header + 1);
        } else if (dispatch_entry->has_request) {
            vigsp_status (*func)(struct vigs_comm*, void*) =
                dispatch_entry->func;
            status = func(comm, request_header + 1);
        } else if (dispatch_entry->has_response) {
            vigsp_status (*func)(struct vigs_comm*, void*) =
                dispatch_entry->func;
            status = func(comm, response_header + 1);
        } else {
            vigsp_status (*func)(struct vigs_comm*) =
                dispatch_entry->func;
            status = func(comm);
        }
    }

    response_header->status = status;
}
