/* header to be included in non-HAX-specific code */
#ifndef _HAX_H
#define _HAX_H

#include "config-host.h"
#include "qemu-common.h"
//#include "cpu.h"
#include "kvm.h"
#include "hw/hw.h"
#include "bitops.h"
#include "memory.h"

#define dprint	printf
#ifdef CONFIG_HAX_BACKEND
int hax_enabled(void);
void hax_disable(int disable);
int hax_pre_init(ram_addr_t ram_size);
int hax_accel_init(void);
int hax_sync_vcpus(void);
#ifdef CONFIG_HAX
int hax_init_vcpu(CPUArchState *env);
int hax_vcpu_exec(CPUArchState *env);
void hax_vcpu_sync_state(CPUArchState *env, int modified);
//extern void hax_cpu_synchronize_state(CPUArchState *env);
//extern void hax_cpu_synchronize_post_reset(CPUArchState *env);
//extern void hax_cpu_synchronize_post_init(CPUArchState *env);
int hax_populate_ram(uint64_t va, uint32_t size);
int hax_set_phys_mem(MemoryRegionSection *section);
int hax_vcpu_emulation_mode(CPUArchState *env);
int hax_stop_emulation(CPUArchState *env);
int hax_stop_translate(CPUArchState *env);
int hax_arch_get_registers(CPUArchState *env);
int hax_vcpu_destroy(CPUArchState *env);
void hax_raise_event(CPUArchState *env);
int need_handle_intr_request(CPUArchState *env);
int hax_handle_io(CPUArchState *env, uint32_t df, uint16_t port, int direction,
		 int size, int count, void *buffer);
void qemu_notify_hax_event(void);
void hax_reset_vcpu_state(void *opaque);
#include "target-i386/hax-interface.h"
#include "target-i386/hax-i386.h"
#endif
#else
#define hax_enabled()            (0)
#define hax_sync_vcpus()
#define hax_accel_init()         (0)
#define hax_pre_init(x)
#endif

#endif
