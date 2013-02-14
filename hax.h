/* header to be included in non-HAX-specific code */
#ifndef _HAX_H
#define _HAX_H

#include "config-host.h"
#include "qemu-common.h"
//#include "cpu.h"
#include "kvm.h"
#include "hw/hw.h"
#include "bitops.h"

extern int hax_disabled;
#define dprint	printf
#ifdef CONFIG_HAX
int hax_enabled(void);
int hax_pre_init(ram_addr_t ram_size);
int hax_init(void);
int hax_sync_vcpus(void);
#ifdef NEED_CPU_H
int hax_init_vcpu(CPUState *env);
int hax_vcpu_exec(CPUState *env);
void hax_vcpu_sync_state(CPUState *env, int modified);
//extern void hax_cpu_synchronize_state(CPUState *env);
//extern void hax_cpu_synchronize_post_reset(CPUState *env);
//extern void hax_cpu_synchronize_post_init(CPUState *env);
int hax_populate_ram(uint64_t va, uint32_t size);
int hax_set_phys_mem(target_phys_addr_t start_addr,
                     ram_addr_t size, ram_addr_t phys_offset);
int hax_vcpu_emulation_mode(CPUState *env);
int hax_stop_emulation(CPUState *env);
int hax_stop_translate(CPUState *env);
int hax_arch_get_registers(CPUState *env);
int hax_vcpu_destroy(CPUState *env);
void hax_raise_event(CPUState *env);
int need_handle_intr_request(CPUState *env);
int hax_handle_io(CPUState *env, uint32_t df, uint16_t port, int direction,
		 int size, int count, void *buffer);
void qemu_notify_hax_event(void);
#endif
void hax_reset_vcpu_state(void *opaque);

#include "target-i386/hax-interface.h"
#include "target-i386/hax-i386.h"
#else
#define hax_enabled() (0)
#endif

#endif
