/*
 * QEMU HAXM support
 *
 * Copyright (c) 2011 Intel Corporation
 *  Written by:
 *  Jiang Yunhong<yunhong.jiang@intel.com>
 *  Xin Xiaohui<xiaohui.xin@intel.com>
 *  Zhang Xiantao<xiantao.zhang@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 *
 */

/* Interface with HAX kernel module */

#ifndef _HAX_INTERFACE_H
#define _HAX_INTERFACE_H

/* fx_layout has 3 formats table 3-56, 512bytes */
struct fx_layout {
    uint16_t    fcw;
    uint16_t    fsw;
    uint8       ftw;
    uint8       res1;
    uint16_t    fop;
    union {
        struct {
            uint32      fip;
            uint16_t    fcs;
            uint16_t    res2;
        };
        uint64  fpu_ip;
    };
    union {
        struct {
            uint32      fdp;
            uint16_t    fds;
            uint16_t    res3;
        };
        uint64 fpu_dp;
    };
    uint32      mxcsr;
    uint32      mxcsr_mask;
    uint8       st_mm[8][16];
    uint8       mmx_1[8][16];
    uint8       mmx_2[8][16];
    uint8       pad[96];
};

struct vmx_msr {
    uint64 entry;
    uint64 value;
};

/*
 * Fixed array is not good, but it makes Mac support a bit easier by avoiding
 * memory map or copyin staff.
 */
#define HAX_MAX_MSR_ARRAY 0x20
struct hax_msr_data
{
    uint16_t nr_msr;
    uint16_t done;
    uint16_t pad[2];
    struct vmx_msr entries[HAX_MAX_MSR_ARRAY];
};

union interruptibility_state_t {
    uint32 raw;
    struct {
        uint32 sti_blocking   : 1;
        uint32 movss_blocking : 1;
        uint32 smi_blocking   : 1;
        uint32 nmi_blocking   : 1;
        uint32 reserved       : 28;
    };
    uint64_t pad;
};

typedef union interruptibility_state_t interruptibility_state_t;

// Segment descriptor
struct segment_desc_t {
    uint16_t selector;
    uint16_t _dummy;
    uint32 limit;
    uint64 base;
    union {
        struct {
            uint32 type             : 4;
            uint32 desc             : 1;
            uint32 dpl              : 2;
            uint32 present          : 1;
            uint32                  : 4;
            uint32 available        : 1;
            uint32 long_mode        : 1;
            uint32 operand_size     : 1;
            uint32 granularity      : 1;
            uint32 null             : 1;
            uint32                  : 15;
        };
        uint32 ar;
    };
    uint32 ipad;
};

typedef struct segment_desc_t segment_desc_t;

struct vcpu_state_t
{
    union {
        uint64 _regs[16];
        struct {
            union {
                struct {
                    uint8 _al,
                          _ah;
                };
                uint16_t    _ax;
                uint32    _eax;
                uint64    _rax;
            };
            union {
                struct {
                    uint8 _cl,
                          _ch;
                };
                uint16_t    _cx;
                uint32    _ecx;
                uint64    _rcx;
            };
            union {
                struct {
                    uint8 _dl,
                          _dh;
                };
                uint16_t    _dx;
                uint32    _edx;
                uint64    _rdx;
            };
            union {
                struct {
                    uint8 _bl,
                          _bh;
                };
                uint16_t    _bx;
                uint32    _ebx;
                uint64    _rbx;
            };
            union {
                uint16_t    _sp;
                uint32    _esp;
                uint64    _rsp;
            };
            union {
                uint16_t    _bp;
                uint32    _ebp;
                uint64    _rbp;
            };
            union {
                uint16_t    _si;
                uint32    _esi;
                uint64    _rsi;
            };
            union {
                uint16_t    _di;
                uint32    _edi;
                uint64    _rdi;
            };

            uint64 _r8;
            uint64 _r9;
            uint64 _r10;
            uint64 _r11;
            uint64 _r12;
            uint64 _r13;
            uint64 _r14;
            uint64 _r15;
        };
    };

    union {
        uint32 _eip;
        uint64 _rip;
    };

    union {
        uint32 _eflags;
        uint64 _rflags;
    };

    segment_desc_t _cs;
    segment_desc_t _ss;
    segment_desc_t _ds;
    segment_desc_t _es;
    segment_desc_t _fs;
    segment_desc_t _gs;
    segment_desc_t _ldt;
    segment_desc_t _tr;

    segment_desc_t _gdt;
    segment_desc_t _idt;

    uint64 _cr0;
    uint64 _cr2;
    uint64 _cr3;
    uint64 _cr4;

    uint64 _dr0;
    uint64 _dr1;
    uint64 _dr2;
    uint64 _dr3;
    uint64 _dr6;
    uint64 _dr7;
    uint64 _pde;

    uint32 _efer;

    uint32 _sysenter_cs;
    uint64 _sysenter_eip;
    uint64 _sysenter_esp;

    uint32 _activity_state;
    uint32 pad;
    interruptibility_state_t _interruptibility_state;
};


/* Seems there are some difference between Xiaohui/Yunhong's implementation */

enum exit_status {
    /* IO port request */
    HAX_EXIT_IO = 1,
    /* MMIO instruction emulation */
    HAX_EXIT_MMIO,
    /* QEMU emulation mode request, currently means guest enter non-PG mode */
    HAX_EXIT_REAL,
    /*
     * Interrupt window open, qemu can inject interrupt now
     * Also used when signal pending since at that time qemu usually need
     * check interrupt
     */
    HAX_EXIT_INTERRUPT,
    /* Unknown vmexit, mostly trigger reboot */
    HAX_EXIT_UNKNOWN_VMEXIT,
    /* HALT from guest */
    HAX_EXIT_HLT,
    /* Reboot request, like because of tripple fault in guest */
    HAX_EXIT_STATECHANGE,
    /* the vcpu is now only paused when destroy, so simply return to hax */
    HAX_EXIT_PAUSED,
};

/* The interface definition:
   1. vcpu_run execute will return 0 on success, otherwise mean failed
   2. exit_status return the exit reason, as stated in enum exit_status
   3. exit_reason is the vmx exit reason (WHY do we need this? )
   4. exit_flag, who knows what's it for!
 */
struct hax_tunnel
{
    uint32_t _exit_reason;
    uint32_t _exit_flag;
    uint32_t _exit_status;
    uint32_t user_event_pending;
    int ready_for_interrupt_injection;
    int request_interrupt_window;
    union {
        struct {
            /* 0: read, 1: write */
#define HAX_EXIT_IO_IN  1
#define HAX_EXIT_IO_OUT 0
            uint8_t _direction;
            uint8_t _df;
            uint16_t _size;
            uint16_t _port;
            uint16_t _count;
            uint8_t _flags;
            uint8_t _pad0;
            uint16_t _pad1;
            uint32_t _pad2;
            uint64_t _vaddr;
        } pio;
        struct {
            uint64_t gla;
        } mmio;
        struct {
        } state;
    };
};

struct hax_module_version
{
    uint32_t compat_version;
    uint32_t cur_version;
};

/* The mac specfic interface to qemu, mostly is ioctl related */
struct hax_tunnel_info
{
    uint64_t va;
    uint64_t io_va;
    uint16_t size;
    uint16_t pad[3];
};

struct hax_alloc_ram_info
{
    uint32_t size;
    uint32_t pad;
    uint64_t va;
};
#define HAX_RAM_INFO_ROM 0x1
struct hax_set_ram_info
{
    uint64_t pa_start;
    uint32_t size;
    uint8_t flags;
    uint8_t pad[3];
    uint64_t va;
};

#define HAX_CAP_STATUS_WORKING  0x1
#define HAX_CAP_STATUS_NOTWORKING  0x0
#define HAX_CAP_WORKSTATUS_MASK 0x1

#define HAX_CAP_FAILREASON_VT   0x1
#define HAX_CAP_FAILREASON_NX   0x2

#define HAX_CAP_MEMQUOTA	0x2

struct hax_capabilityinfo
{
    /* bit 0: 1 - working
     *        0 - not working, possibly because NT/NX disabled
     * bit 1: 1 - memory limitation working
     *        0 - no memory limitation
     */
    uint16_t wstatus;
    /* valid when not working
     * bit 0: VT not enabeld
     * bit 1: NX not enabled*/
    uint16_t winfo;
    uint32_t pad;
    uint64_t mem_quota;
};

#endif
