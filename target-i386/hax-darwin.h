#ifndef __HAX_UNIX_H
#define __HAX_UNIX_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdarg.h>

#define HAX_INVALID_FD  (-1)
static inline int hax_invalid_fd(hax_fd fd)
{
    return fd <= 0;
}

static inline void hax_mod_close(struct hax_state *hax)
{
    close(hax->fd);
}

static inline void hax_close_fd(hax_fd fd)
{
    close(fd);
}

/* HAX model level ioctl */
#define HAX_IOCTL_VERSION _IOWR(0, 0x20, struct hax_module_version)
#define HAX_IOCTL_CREATE_VM _IOWR(0, 0x21, int)
#define HAX_IOCTL_DESTROY_VM _IOW(0, 0x22, int)
#define HAX_IOCTL_CAPABILITY _IOR(0, 0x23, struct hax_capabilityinfo)

#define HAX_VM_IOCTL_VCPU_CREATE    _IOR(0, 0x80, int)
#define HAX_VM_IOCTL_ALLOC_RAM _IOWR(0, 0x81, struct hax_alloc_ram_info)
#define HAX_VM_IOCTL_SET_RAM _IOWR(0, 0x82, struct hax_set_ram_info)
#define HAX_VM_IOCTL_VCPU_DESTROY    _IOR(0, 0x83, int)

#define HAX_VCPU_IOCTL_RUN  _IO(0, 0xc0)
#define HAX_VCPU_IOCTL_SET_MSRS _IOWR(0, 0xc1, struct hax_msr_data)
#define HAX_VCPU_IOCTL_GET_MSRS _IOWR(0, 0xc2, struct hax_msr_data)

#define HAX_VCPU_IOCTL_SET_FPU  _IOW(0, 0xc3, struct fx_layout)
#define HAX_VCPU_IOCTL_GET_FPU  _IOR(0, 0xc4, struct fx_layout)

#define HAX_VCPU_IOCTL_SETUP_TUNNEL _IOWR(0, 0xc5, struct hax_tunnel_info)
#define HAX_VCPU_IOCTL_INTERRUPT _IOWR(0, 0xc6, uint32_t)
#define HAX_VCPU_SET_REGS       _IOWR(0, 0xc7, struct vcpu_state_t)
#define HAX_VCPU_GET_REGS       _IOWR(0, 0xc8, struct vcpu_state_t)

#endif /* __HAX_UNIX_H */
