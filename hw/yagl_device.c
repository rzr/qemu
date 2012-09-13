#include "yagl_server.h"
#include "yagl_log.h"
#include "yagl_handle_gen.h"
#include "yagl_marshal.h"
#include "yagl_stats.h"
#include "cpu-all.h"
#include "hw.h"
#include "pci.h"

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
                                target_phys_addr_t buff_pa)
{
    yagl_pid target_pid;
    yagl_tid target_tid;
    target_phys_addr_t buff_len = YAGL_MARSHAL_SIZE;
    uint8_t *buff = NULL, *tmp = NULL;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_device_operate,
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
    YAGL_LOG_FUNC_ENTER_NPT(yagl_device_trigger, "%d", user_index);

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

static uint64_t yagl_device_read(void *opaque, target_phys_addr_t offset,
                                 unsigned size)
{
    return 0;
}

static void yagl_device_write(void *opaque, target_phys_addr_t offset,
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

    yagl_log_init();

    YAGL_LOG_FUNC_ENTER_NPT(yagl_device_init, NULL);

    memory_region_init_io(&s->iomem,
                          &yagl_device_ops,
                          s,
                          TYPE_YAGL_DEVICE,
                          YAGL_MEM_SIZE);

    yagl_handle_gen_init();

    yagl_stats_init();

    s->ss = yagl_server_state_create();

    if (!s->ss) {
        yagl_stats_cleanup();

        yagl_handle_gen_cleanup();

        YAGL_LOG_FUNC_EXIT(NULL);

        yagl_log_cleanup();

        return -1;
    }

    s->in_buff = g_malloc(YAGL_MARSHAL_MAX_RESPONSE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->iomem);

    YAGL_LOG_FUNC_EXIT(NULL);

    return 0;
}

static void yagl_device_reset(DeviceState *d)
{
    YaGLState *s = container_of(d, YaGLState, dev.qdev);
    int i;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_device_reset, NULL);

    yagl_server_reset(s->ss);

    yagl_handle_gen_reset();

    for (i = 0; i < YAGL_MAX_USERS; ++i) {
        memset(&s->users[i], 0, sizeof(s->users[i]));
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static int yagl_device_exit(PCIDevice *dev)
{
    YaGLState *s = DO_UPCAST(YaGLState, dev, dev);

    YAGL_LOG_FUNC_ENTER_NPT(yagl_device_exit, NULL);

    memory_region_destroy(&s->iomem);

    g_free(s->in_buff);
    s->in_buff = NULL;

    yagl_server_state_destroy(s->ss);

    yagl_stats_cleanup();

    yagl_handle_gen_cleanup();

    YAGL_LOG_FUNC_EXIT(NULL);

    yagl_log_cleanup();

    return 0;
}

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
