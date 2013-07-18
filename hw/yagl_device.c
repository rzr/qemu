#include "yagl_server.h"
#include "yagl_log.h"
#include "yagl_handle_gen.h"
#include "yagl_marshal.h"
#include "yagl_stats.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_egl_driver.h"
#include "yagl_drivers/gles1_ogl/yagl_gles1_ogl.h"
#include "yagl_drivers/gles2_ogl/yagl_gles2_ogl.h"
#include "yagl_drivers/gles1_onscreen/yagl_gles1_onscreen.h"
#include "yagl_drivers/gles2_onscreen/yagl_gles2_onscreen.h"
#include "yagl_backends/egl_offscreen/yagl_egl_offscreen.h"
#include "yagl_backends/egl_onscreen/yagl_egl_onscreen.h"
#include "exec/cpu-all.h"
#include "hw/hw.h"
#include "hw/pci/pci.h"
#include <GL/gl.h>
#include "winsys.h"
#include "yagl_gles1_driver.h"
#include "yagl_gles2_driver.h"

#define PCI_VENDOR_ID_YAGL 0x19B1
#define PCI_DEVICE_ID_YAGL 0x1010

#define YAGL_REG_BUFFPTR 0
#define YAGL_REG_TRIGGER 4
#define YAGL_REGS_SIZE   8

#define YAGL_MEM_SIZE 0x1000

#define YAGL_MAX_USERS (YAGL_MEM_SIZE / YAGL_REGS_SIZE)

struct yagl_user
{
    uint8_t *buff;
    yagl_pid process_id;
    yagl_tid thread_id;
};

typedef struct YaGLState
{
    PCIDevice dev;
    void *display;
    struct winsys_interface *wsi;
    MemoryRegion iomem;
    struct yagl_server_state *ss;
    struct yagl_user users[YAGL_MAX_USERS];

    /*
     * YAGL_MARSHAL_MAX_RESPONSE byte buffer to hold the response.
     */
    uint8_t *in_buff;
} YaGLState;

#define TYPE_YAGL_DEVICE "yagl"

static void yagl_device_operate(YaGLState *s,
                                int user_index,
                                hwaddr buff_pa)
{
    yagl_pid target_pid;
    yagl_tid target_tid;
    hwaddr buff_len = YAGL_MARSHAL_SIZE;
    uint8_t *buff = NULL, *tmp = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_device_operate,
                        "user_index = %d, buff_ptr = 0x%X",
                        user_index,
                        (uint32_t)buff_pa);

    if (buff_pa && s->users[user_index].buff) {
        YAGL_LOG_CRITICAL("user %d is already activated", user_index);
        goto out;
    }

    if (!buff_pa && !s->users[user_index].buff) {
        YAGL_LOG_CRITICAL("user %d is not activated", user_index);
        goto out;
    }

    if (buff_pa) {
        /*
         * Activate user.
         */

        buff = cpu_physical_memory_map(buff_pa, &buff_len, false);

        if (!buff || (buff_len != YAGL_MARSHAL_SIZE)) {
            YAGL_LOG_CRITICAL("cpu_physical_memory_map(read) failed for user %d, buff_ptr = 0x%X",
                              user_index,
                              (uint32_t)buff_pa);
            goto out;
        }

        tmp = buff;

        yagl_marshal_skip(&tmp);

        target_pid = yagl_marshal_get_pid(&tmp);
        target_tid = yagl_marshal_get_tid(&tmp);

        YAGL_LOG_TRACE("pid = %u, tid = %u", target_pid, target_tid);

        if (!target_pid || !target_tid) {
            YAGL_LOG_CRITICAL(
                "target-host protocol error, zero pid or tid from target");
            goto out;
        }

        if (yagl_server_dispatch_init(s->ss,
                                      target_pid,
                                      target_tid,
                                      buff,
                                      s->in_buff)) {

            memcpy(buff, s->in_buff, YAGL_MARSHAL_MAX_RESPONSE);

            s->users[user_index].buff = buff;
            s->users[user_index].process_id = target_pid;
            s->users[user_index].thread_id = target_tid;

            buff = NULL;

            YAGL_LOG_INFO("user %d activated", user_index);
        }
    } else {
        /*
         * Deactivate user.
         */

        yagl_server_dispatch_exit(s->ss,
                                  s->users[user_index].process_id,
                                  s->users[user_index].thread_id);

        cpu_physical_memory_unmap(s->users[user_index].buff,
                                  YAGL_MARSHAL_SIZE,
                                  0,
                                  YAGL_MARSHAL_SIZE);

        memset(&s->users[user_index], 0, sizeof(s->users[user_index]));

        YAGL_LOG_INFO("user %d deactivated", user_index);
    }

out:
    if (buff) {
        cpu_physical_memory_unmap(buff,
                                  YAGL_MARSHAL_SIZE,
                                  0,
                                  YAGL_MARSHAL_SIZE);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_device_trigger(YaGLState *s, int user_index)
{
    YAGL_LOG_FUNC_ENTER(yagl_device_trigger, "%d", user_index);

    if (!s->users[user_index].buff) {
        YAGL_LOG_CRITICAL("user %d not activated", user_index);
        goto out;
    }

    yagl_server_dispatch(s->ss,
                         s->users[user_index].process_id,
                         s->users[user_index].thread_id,
                         s->users[user_index].buff,
                         s->in_buff);

    memcpy(s->users[user_index].buff, s->in_buff, YAGL_MARSHAL_MAX_RESPONSE);

out:
    YAGL_LOG_FUNC_EXIT(NULL);
}

static uint64_t yagl_device_read(void *opaque, hwaddr offset,
                                 unsigned size)
{
    return 0;
}

static void yagl_device_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    YaGLState *s = (YaGLState*)opaque;
    int user_index = (offset / YAGL_REGS_SIZE);
    offset -= user_index * YAGL_REGS_SIZE;

    assert(user_index < YAGL_MAX_USERS);

    if (user_index >= YAGL_MAX_USERS) {
        YAGL_LOG_CRITICAL("bad user index = %d", user_index);
        return;
    }

    switch (offset) {
    case YAGL_REG_BUFFPTR:
        yagl_device_operate(s, user_index, value);
        break;
    case YAGL_REG_TRIGGER:
        yagl_device_trigger(s, user_index);
        break;
    default:
        YAGL_LOG_CRITICAL("user %d, bad offset = %d", user_index, offset);
        break;
    }
}

static const MemoryRegionOps yagl_device_ops =
{
    .read = yagl_device_read,
    .write = yagl_device_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int yagl_device_init(PCIDevice *dev)
{
    YaGLState *s = DO_UPCAST(YaGLState, dev, dev);
    struct yagl_egl_driver *egl_driver = NULL;
    struct yagl_egl_backend *egl_backend = NULL;
    struct yagl_gles1_driver *gles1_driver = NULL;
    struct yagl_gles2_driver *gles2_driver = NULL;

    yagl_log_init();

    YAGL_LOG_FUNC_ENTER(yagl_device_init, NULL);

    memory_region_init_io(&s->iomem,
                          &yagl_device_ops,
                          s,
                          TYPE_YAGL_DEVICE,
                          YAGL_MEM_SIZE);

    yagl_handle_gen_init();

    yagl_stats_init();

    egl_driver = yagl_egl_driver_create(s->display);

    if (!egl_driver) {
        goto fail;
    }

    gles1_driver = yagl_gles1_ogl_create(egl_driver->dyn_lib);

    if (!gles1_driver) {
        goto fail;
    }

    gles2_driver = yagl_gles2_ogl_create(egl_driver->dyn_lib);

    if (!gles2_driver) {
        goto fail;
    }

    if (s->wsi) {
        egl_backend = yagl_egl_onscreen_create(s->wsi,
                                               egl_driver,
                                               &gles2_driver->base);
        gles1_driver = yagl_gles1_onscreen_create(gles1_driver);
        gles2_driver = yagl_gles2_onscreen_create(gles2_driver);
    } else {
        egl_backend = yagl_egl_offscreen_create(egl_driver);
    }

    if (!egl_backend) {
        goto fail;
    }

    /*
     * Now owned by EGL backend.
     */
    egl_driver = NULL;

    s->ss = yagl_server_state_create(egl_backend, gles1_driver, gles2_driver);

    /*
     * Owned/destroyed by server state.
     */
    egl_backend = NULL;
    gles1_driver = NULL;
    gles2_driver = NULL;

    if (!s->ss) {
        goto fail;
    }

    s->in_buff = g_malloc(YAGL_MARSHAL_MAX_RESPONSE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->iomem);

    YAGL_LOG_FUNC_EXIT(NULL);

    return 0;

fail:
    if (egl_backend) {
        egl_backend->destroy(egl_backend);
    }

    if (gles1_driver) {
        gles1_driver->destroy(gles1_driver);
    }

    if (gles2_driver) {
        gles2_driver->destroy(gles2_driver);
    }

    if (egl_driver) {
        egl_driver->destroy(egl_driver);
    }

    yagl_stats_cleanup();

    yagl_handle_gen_cleanup();

    YAGL_LOG_FUNC_EXIT(NULL);

    yagl_log_cleanup();

    return -1;
}

static void yagl_device_reset(DeviceState *d)
{
    YaGLState *s = container_of(d, YaGLState, dev.qdev);
    int i;

    YAGL_LOG_FUNC_ENTER(yagl_device_reset, NULL);

    yagl_server_reset(s->ss);

    yagl_handle_gen_reset();

    for (i = 0; i < YAGL_MAX_USERS; ++i) {
        memset(&s->users[i], 0, sizeof(s->users[i]));
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_device_exit(PCIDevice *dev)
{
    YaGLState *s = DO_UPCAST(YaGLState, dev, dev);

    YAGL_LOG_FUNC_ENTER(yagl_device_exit, NULL);

    memory_region_destroy(&s->iomem);

    g_free(s->in_buff);
    s->in_buff = NULL;

    yagl_server_state_destroy(s->ss);

    yagl_stats_cleanup();

    yagl_handle_gen_cleanup();

    YAGL_LOG_FUNC_EXIT(NULL);

    yagl_log_cleanup();
}

static Property yagl_properties[] = {
    {
        .name   = "display",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(YaGLState, display),
    },
    {
        .name   = "winsys_gl_interface",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(YaGLState, wsi),
    },
    DEFINE_PROP_END_OF_LIST(),
};

static void yagl_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = yagl_device_init;
    k->exit = yagl_device_exit;
    k->vendor_id = PCI_VENDOR_ID_YAGL;
    k->device_id = PCI_DEVICE_ID_YAGL;
    k->class_id = PCI_CLASS_OTHERS;
    dc->reset = yagl_device_reset;
    dc->props = yagl_properties;
    dc->desc = "YaGL device";
}

static TypeInfo yagl_device_info =
{
    .name          = TYPE_YAGL_DEVICE,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(YaGLState),
    .class_init    = yagl_class_init,
};

static void yagl_register_types(void)
{
    type_register_static(&yagl_device_info);
}

type_init(yagl_register_types)
