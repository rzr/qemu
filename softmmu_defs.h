/*
 *  Software MMU support
 *
 * Declare helpers used by TCG for qemu_ld/st ops.
 *
 * Used by softmmu_exec.h, TCG targets and exec-all.h.
 *
 */
#ifndef SOFTMMU_DEFS_H
#define SOFTMMU_DEFS_H

#ifndef CONFIG_TCG_PASS_AREG0
uint8_t __ldb_mmu(target_ulong addr, int mmu_idx);
void __stb_mmu(target_ulong addr, uint8_t val, int mmu_idx);
uint16_t __ldw_mmu(target_ulong addr, int mmu_idx);
void __stw_mmu(target_ulong addr, uint16_t val, int mmu_idx);
uint32_t __ldl_mmu(target_ulong addr, int mmu_idx);
void __stl_mmu(target_ulong addr, uint32_t val, int mmu_idx);
uint64_t __ldq_mmu(target_ulong addr, int mmu_idx);
void __stq_mmu(target_ulong addr, uint64_t val, int mmu_idx);

#if defined(CONFIG_QEMU_LDST_OPTIMIZATION) && defined(CONFIG_SOFTMMU)
/* Extended versions of MMU helpers for qemu_ld/st optimization.
   They get return address arguments because the caller PCs are not where helpers return to. */
uint8_t __ext_ldb_mmu(target_ulong addr, int mmu_idx, uintptr_t ra);
void __ext_stb_mmu(target_ulong addr, uint8_t val, int mmu_idx, uintptr_t ra);
uint16_t __ext_ldw_mmu(target_ulong addr, int mmu_idx, uintptr_t ra);
void __ext_stw_mmu(target_ulong addr, uint16_t val, int mmu_idx, uintptr_t ra);
uint32_t __ext_ldl_mmu(target_ulong addr, int mmu_idx, uintptr_t ra);
void __ext_stl_mmu(target_ulong addr, uint32_t val, int mmu_idx, uintptr_t ra);
uint64_t __ext_ldq_mmu(target_ulong addr, int mmu_idx, uintptr_t ra);
void __ext_stq_mmu(target_ulong addr, uint64_t val, int mmu_idx, uintptr_t ra);
#endif  /* CONFIG_QEMU_LDST_OPTIMIZATION && CONFIG_SOFTMMU */

uint8_t __ldb_cmmu(target_ulong addr, int mmu_idx);
void __stb_cmmu(target_ulong addr, uint8_t val, int mmu_idx);
uint16_t __ldw_cmmu(target_ulong addr, int mmu_idx);
void __stw_cmmu(target_ulong addr, uint16_t val, int mmu_idx);
uint32_t __ldl_cmmu(target_ulong addr, int mmu_idx);
void __stl_cmmu(target_ulong addr, uint32_t val, int mmu_idx);
uint64_t __ldq_cmmu(target_ulong addr, int mmu_idx);
void __stq_cmmu(target_ulong addr, uint64_t val, int mmu_idx);
#else
uint8_t helper_ldb_mmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stb_mmu(CPUArchState *env, target_ulong addr, uint8_t val,
                    int mmu_idx);
uint16_t helper_ldw_mmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stw_mmu(CPUArchState *env, target_ulong addr, uint16_t val,
                    int mmu_idx);
uint32_t helper_ldl_mmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stl_mmu(CPUArchState *env, target_ulong addr, uint32_t val,
                    int mmu_idx);
uint64_t helper_ldq_mmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stq_mmu(CPUArchState *env, target_ulong addr, uint64_t val,
                    int mmu_idx);

uint8_t helper_ldb_cmmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stb_cmmu(CPUArchState *env, target_ulong addr, uint8_t val,
int mmu_idx);
uint16_t helper_ldw_cmmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stw_cmmu(CPUArchState *env, target_ulong addr, uint16_t val,
                     int mmu_idx);
uint32_t helper_ldl_cmmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stl_cmmu(CPUArchState *env, target_ulong addr, uint32_t val,
                     int mmu_idx);
uint64_t helper_ldq_cmmu(CPUArchState *env, target_ulong addr, int mmu_idx);
void helper_stq_cmmu(CPUArchState *env, target_ulong addr, uint64_t val,
                     int mmu_idx);
#endif

#endif
