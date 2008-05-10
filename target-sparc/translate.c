/*
   SPARC translation

   Copyright (C) 2003 Thomas M. Ogrisegg <tom@fnord.at>
   Copyright (C) 2003-2005 Fabrice Bellard

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "cpu.h"
#include "exec-all.h"
#include "disas.h"
#include "helper.h"
#include "tcg-op.h"

#define DEBUG_DISAS

#define DYNAMIC_PC  1 /* dynamic pc value */
#define JUMP_PC     2 /* dynamic pc value which takes only two values
                         according to jump_pc[T2] */

/* global register indexes */
static TCGv cpu_env, cpu_T[2], cpu_regwptr, cpu_cc_src, cpu_cc_src2, cpu_cc_dst;
static TCGv cpu_psr, cpu_fsr, cpu_pc, cpu_npc, cpu_gregs[8];
static TCGv cpu_cond, cpu_src1, cpu_src2, cpu_dst, cpu_addr, cpu_val;
#ifdef TARGET_SPARC64
static TCGv cpu_xcc;
#endif
/* local register indexes (only used inside old micro ops) */
static TCGv cpu_tmp0, cpu_tmp32, cpu_tmp64;

typedef struct DisasContext {
    target_ulong pc;    /* current Program Counter: integer or DYNAMIC_PC */
    target_ulong npc;   /* next PC: integer or DYNAMIC_PC or JUMP_PC */
    target_ulong jump_pc[2]; /* used when JUMP_PC pc value is used */
    int is_br;
    int mem_idx;
    int fpu_enabled;
    struct TranslationBlock *tb;
    uint32_t features;
} DisasContext;

// This function uses non-native bit order
#define GET_FIELD(X, FROM, TO) \
  ((X) >> (31 - (TO)) & ((1 << ((TO) - (FROM) + 1)) - 1))

// This function uses the order in the manuals, i.e. bit 0 is 2^0
#define GET_FIELD_SP(X, FROM, TO) \
    GET_FIELD(X, 31 - (TO), 31 - (FROM))

#define GET_FIELDs(x,a,b) sign_extend (GET_FIELD(x,a,b), (b) - (a) + 1)
#define GET_FIELD_SPs(x,a,b) sign_extend (GET_FIELD_SP(x,a,b), ((b) - (a) + 1))

#ifdef TARGET_SPARC64
#define FFPREG(r) (r)
#define DFPREG(r) (((r & 1) << 5) | (r & 0x1e))
#define QFPREG(r) (((r & 1) << 5) | (r & 0x1c))
#else
#define FFPREG(r) (r)
#define DFPREG(r) (r & 0x1e)
#define QFPREG(r) (r & 0x1c)
#endif

static int sign_extend(int x, int len)
{
    len = 32 - len;
    return (x << len) >> len;
}

#define IS_IMM (insn & (1<<13))

/* floating point registers moves */
static void gen_op_load_fpr_FT0(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, ft0));
}

static void gen_op_load_fpr_FT1(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, ft1));
}

static void gen_op_store_FT0_fpr(unsigned int dst)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, ft0));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst]));
}

static void gen_op_load_fpr_DT0(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt0) + offsetof(CPU_DoubleU, l.upper));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 1]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt0) + offsetof(CPU_DoubleU, l.lower));
}

static void gen_op_load_fpr_DT1(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt1) + offsetof(CPU_DoubleU, l.upper));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 1]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt1) + offsetof(CPU_DoubleU, l.lower));
}

static void gen_op_store_DT0_fpr(unsigned int dst)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt0) + offsetof(CPU_DoubleU, l.upper));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst]));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, dt0) + offsetof(CPU_DoubleU, l.lower));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst + 1]));
}

static void gen_op_load_fpr_QT0(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.upmost));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 1]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.upper));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 2]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.lower));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 3]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.lowest));
}

static void gen_op_load_fpr_QT1(unsigned int src)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt1) + offsetof(CPU_QuadU, l.upmost));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 1]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt1) + offsetof(CPU_QuadU, l.upper));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 2]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt1) + offsetof(CPU_QuadU, l.lower));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[src + 3]));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt1) + offsetof(CPU_QuadU, l.lowest));
}

static void gen_op_store_QT0_fpr(unsigned int dst)
{
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.upmost));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst]));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.upper));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst + 1]));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.lower));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst + 2]));
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, qt0) + offsetof(CPU_QuadU, l.lowest));
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fpr[dst + 3]));
}

/* moves */
#ifdef CONFIG_USER_ONLY
#define supervisor(dc) 0
#ifdef TARGET_SPARC64
#define hypervisor(dc) 0
#endif
#else
#define supervisor(dc) (dc->mem_idx >= 1)
#ifdef TARGET_SPARC64
#define hypervisor(dc) (dc->mem_idx == 2)
#else
#endif
#endif

#ifdef TARGET_ABI32
#define ABI32_MASK(addr) tcg_gen_andi_tl(addr, addr, 0xffffffffULL);
#else
#define ABI32_MASK(addr)
#endif

static inline void gen_movl_reg_TN(int reg, TCGv tn)
{
    if (reg == 0)
        tcg_gen_movi_tl(tn, 0);
    else if (reg < 8)
        tcg_gen_mov_tl(tn, cpu_gregs[reg]);
    else {
        tcg_gen_ld_tl(tn, cpu_regwptr, (reg - 8) * sizeof(target_ulong));
    }
}

static inline void gen_movl_TN_reg(int reg, TCGv tn)
{
    if (reg == 0)
        return;
    else if (reg < 8)
        tcg_gen_mov_tl(cpu_gregs[reg], tn);
    else {
        tcg_gen_st_tl(tn, cpu_regwptr, (reg - 8) * sizeof(target_ulong));
    }
}

static inline void gen_goto_tb(DisasContext *s, int tb_num,
                               target_ulong pc, target_ulong npc)
{
    TranslationBlock *tb;

    tb = s->tb;
    if ((pc & TARGET_PAGE_MASK) == (tb->pc & TARGET_PAGE_MASK) &&
        (npc & TARGET_PAGE_MASK) == (tb->pc & TARGET_PAGE_MASK))  {
        /* jump to same page: we can use a direct jump */
        tcg_gen_goto_tb(tb_num);
        tcg_gen_movi_tl(cpu_pc, pc);
        tcg_gen_movi_tl(cpu_npc, npc);
        tcg_gen_exit_tb((long)tb + tb_num);
    } else {
        /* jump to another page: currently not optimized */
        tcg_gen_movi_tl(cpu_pc, pc);
        tcg_gen_movi_tl(cpu_npc, npc);
        tcg_gen_exit_tb(0);
    }
}

// XXX suboptimal
static inline void gen_mov_reg_N(TCGv reg, TCGv src)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, PSR_NEG_SHIFT);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

static inline void gen_mov_reg_Z(TCGv reg, TCGv src)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, PSR_ZERO_SHIFT);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

static inline void gen_mov_reg_V(TCGv reg, TCGv src)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, PSR_OVF_SHIFT);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

static inline void gen_mov_reg_C(TCGv reg, TCGv src)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, PSR_CARRY_SHIFT);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

static inline void gen_cc_clear_icc(void)
{
    tcg_gen_movi_i32(cpu_psr, 0);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_clear_xcc(void)
{
    tcg_gen_movi_i32(cpu_xcc, 0);
}
#endif

/* old op:
    if (!T0)
        env->psr |= PSR_ZERO;
    if ((int32_t) T0 < 0)
        env->psr |= PSR_NEG;
*/
static inline void gen_cc_NZ_icc(TCGv dst)
{
    TCGv r_temp;
    int l1, l2;

    l1 = gen_new_label();
    l2 = gen_new_label();
    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_andi_tl(r_temp, dst, 0xffffffffULL);
    tcg_gen_brcond_tl(TCG_COND_NE, r_temp, tcg_const_tl(0), l1);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_ZERO);
    gen_set_label(l1);
    tcg_gen_ext_i32_tl(r_temp, dst);
    tcg_gen_brcond_tl(TCG_COND_GE, r_temp, tcg_const_tl(0), l2);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_NEG);
    gen_set_label(l2);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_NZ_xcc(TCGv dst)
{
    int l1, l2;

    l1 = gen_new_label();
    l2 = gen_new_label();
    tcg_gen_brcond_tl(TCG_COND_NE, dst, tcg_const_tl(0), l1);
    tcg_gen_ori_i32(cpu_xcc, cpu_xcc, PSR_ZERO);
    gen_set_label(l1);
    tcg_gen_brcond_tl(TCG_COND_GE, dst, tcg_const_tl(0), l2);
    tcg_gen_ori_i32(cpu_xcc, cpu_xcc, PSR_NEG);
    gen_set_label(l2);
}
#endif

/* old op:
    if (T0 < src1)
        env->psr |= PSR_CARRY;
*/
static inline void gen_cc_C_add_icc(TCGv dst, TCGv src1)
{
    TCGv r_temp;
    int l1;

    l1 = gen_new_label();
    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_andi_tl(r_temp, dst, 0xffffffffULL);
    tcg_gen_brcond_tl(TCG_COND_GEU, dst, src1, l1);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_CARRY);
    gen_set_label(l1);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_C_add_xcc(TCGv dst, TCGv src1)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_brcond_tl(TCG_COND_GEU, dst, src1, l1);
    tcg_gen_ori_i32(cpu_xcc, cpu_xcc, PSR_CARRY);
    gen_set_label(l1);
}
#endif

/* old op:
    if (((src1 ^ T1 ^ -1) & (src1 ^ T0)) & (1 << 31))
        env->psr |= PSR_OVF;
*/
static inline void gen_cc_V_add_icc(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xori_tl(r_temp, r_temp, -1);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1 << 31));
    tcg_gen_shri_tl(r_temp, r_temp, 31 - PSR_OVF_SHIFT);
    tcg_gen_trunc_tl_i32(cpu_tmp32, r_temp);
    tcg_gen_or_i32(cpu_psr, cpu_psr, cpu_tmp32);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_V_add_xcc(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xori_tl(r_temp, r_temp, -1);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1ULL << 63));
    tcg_gen_shri_tl(r_temp, r_temp, 63 - PSR_OVF_SHIFT);
    tcg_gen_trunc_tl_i32(cpu_tmp32, r_temp);
    tcg_gen_or_i32(cpu_xcc, cpu_xcc, cpu_tmp32);
}
#endif

static inline void gen_add_tv(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;
    int l1;

    l1 = gen_new_label();

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xori_tl(r_temp, r_temp, -1);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1 << 31));
    tcg_gen_brcond_tl(TCG_COND_EQ, r_temp, tcg_const_tl(0), l1);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_TOVF));
    gen_set_label(l1);
}

static inline void gen_cc_V_tag(TCGv src1, TCGv src2)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_or_tl(cpu_tmp0, src1, src2);
    tcg_gen_andi_tl(cpu_tmp0, cpu_tmp0, 0x3);
    tcg_gen_brcond_tl(TCG_COND_EQ, cpu_tmp0, tcg_const_tl(0), l1);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_OVF);
    gen_set_label(l1);
}

static inline void gen_tag_tv(TCGv src1, TCGv src2)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_or_tl(cpu_tmp0, src1, src2);
    tcg_gen_andi_tl(cpu_tmp0, cpu_tmp0, 0x3);
    tcg_gen_brcond_tl(TCG_COND_EQ, cpu_tmp0, tcg_const_tl(0), l1);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_TOVF));
    gen_set_label(l1);
}

static inline void gen_op_add_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    tcg_gen_add_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_add_icc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_add_xcc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_addx_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    gen_mov_reg_C(cpu_tmp0, cpu_psr);
    tcg_gen_add_tl(dst, src1, cpu_tmp0);
    gen_cc_clear_icc();
    gen_cc_C_add_icc(dst, cpu_cc_src);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_C_add_xcc(dst, cpu_cc_src);
#endif
    tcg_gen_add_tl(dst, dst, cpu_cc_src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_add_icc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_add_xcc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_tadd_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    tcg_gen_add_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_add_icc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
    gen_cc_V_tag(cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_add_xcc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_tadd_ccTV(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    gen_tag_tv(cpu_cc_src, cpu_cc_src2);
    tcg_gen_add_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_add_tv(dst, cpu_cc_src, cpu_cc_src2);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_add_icc(cpu_cc_dst, cpu_cc_src);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_add_xcc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_add_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

/* old op:
    if (src1 < T1)
        env->psr |= PSR_CARRY;
*/
static inline void gen_cc_C_sub_icc(TCGv src1, TCGv src2)
{
    TCGv r_temp1, r_temp2;
    int l1;

    l1 = gen_new_label();
    r_temp1 = tcg_temp_new(TCG_TYPE_TL);
    r_temp2 = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_andi_tl(r_temp1, src1, 0xffffffffULL);
    tcg_gen_andi_tl(r_temp2, src2, 0xffffffffULL);
    tcg_gen_brcond_tl(TCG_COND_GEU, r_temp1, r_temp2, l1);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_CARRY);
    gen_set_label(l1);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_C_sub_xcc(TCGv src1, TCGv src2)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_brcond_tl(TCG_COND_GEU, src1, src2, l1);
    tcg_gen_ori_i32(cpu_xcc, cpu_xcc, PSR_CARRY);
    gen_set_label(l1);
}
#endif

/* old op:
    if (((src1 ^ T1) & (src1 ^ T0)) & (1 << 31))
        env->psr |= PSR_OVF;
*/
static inline void gen_cc_V_sub_icc(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1 << 31));
    tcg_gen_shri_tl(r_temp, r_temp, 31 - PSR_OVF_SHIFT);
    tcg_gen_trunc_tl_i32(cpu_tmp32, r_temp);
    tcg_gen_or_i32(cpu_psr, cpu_psr, cpu_tmp32);
}

#ifdef TARGET_SPARC64
static inline void gen_cc_V_sub_xcc(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1ULL << 63));
    tcg_gen_shri_tl(r_temp, r_temp, 63 - PSR_OVF_SHIFT);
    tcg_gen_trunc_tl_i32(cpu_tmp32, r_temp);
    tcg_gen_or_i32(cpu_xcc, cpu_xcc, cpu_tmp32);
}
#endif

static inline void gen_sub_tv(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp;
    int l1;

    l1 = gen_new_label();

    r_temp = tcg_temp_new(TCG_TYPE_TL);
    tcg_gen_xor_tl(r_temp, src1, src2);
    tcg_gen_xor_tl(cpu_tmp0, src1, dst);
    tcg_gen_and_tl(r_temp, r_temp, cpu_tmp0);
    tcg_gen_andi_tl(r_temp, r_temp, (1 << 31));
    tcg_gen_brcond_tl(TCG_COND_EQ, r_temp, tcg_const_tl(0), l1);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_TOVF));
    gen_set_label(l1);
}

static inline void gen_op_sub_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    tcg_gen_sub_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_sub_icc(cpu_cc_src, cpu_cc_src2);
    gen_cc_V_sub_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_sub_xcc(cpu_cc_src, cpu_cc_src2);
    gen_cc_V_sub_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_subx_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    gen_mov_reg_C(cpu_tmp0, cpu_psr);
    tcg_gen_sub_tl(dst, src1, cpu_tmp0);
    gen_cc_clear_icc();
    gen_cc_C_sub_icc(dst, cpu_cc_src);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_C_sub_xcc(dst, cpu_cc_src);
#endif
    tcg_gen_sub_tl(dst, dst, cpu_cc_src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_sub_icc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_sub_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_sub_xcc(cpu_cc_dst, cpu_cc_src);
    gen_cc_V_sub_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_tsub_cc(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    tcg_gen_sub_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_sub_icc(cpu_cc_src, cpu_cc_src2);
    gen_cc_V_sub_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
    gen_cc_V_tag(cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_sub_xcc(cpu_cc_src, cpu_cc_src2);
    gen_cc_V_sub_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_tsub_ccTV(TCGv dst, TCGv src1, TCGv src2)
{
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    gen_tag_tv(cpu_cc_src, cpu_cc_src2);
    tcg_gen_sub_tl(dst, src1, src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_sub_tv(dst, cpu_cc_src, cpu_cc_src2);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_C_sub_icc(cpu_cc_src, cpu_cc_src2);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
    gen_cc_C_sub_xcc(cpu_cc_src, cpu_cc_src2);
    gen_cc_V_sub_xcc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
#endif
}

static inline void gen_op_mulscc(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp, r_temp2;
    int l1;

    l1 = gen_new_label();
    r_temp = tcg_temp_new(TCG_TYPE_TL);
    r_temp2 = tcg_temp_new(TCG_TYPE_I32);

    /* old op:
    if (!(env->y & 1))
        T1 = 0;
    */
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_ld32u_tl(r_temp, cpu_env, offsetof(CPUSPARCState, y));
    tcg_gen_trunc_tl_i32(r_temp2, r_temp);
    tcg_gen_andi_i32(r_temp2, r_temp2, 0x1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    tcg_gen_brcond_i32(TCG_COND_NE, r_temp2, tcg_const_i32(0), l1);
    tcg_gen_movi_tl(cpu_cc_src2, 0);
    gen_set_label(l1);

    // b2 = T0 & 1;
    // env->y = (b2 << 31) | (env->y >> 1);
    tcg_gen_trunc_tl_i32(r_temp2, cpu_cc_src);
    tcg_gen_andi_i32(r_temp2, r_temp2, 0x1);
    tcg_gen_shli_i32(r_temp2, r_temp2, 31);
    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, y));
    tcg_gen_shri_i32(cpu_tmp32, cpu_tmp32, 1);
    tcg_gen_or_i32(cpu_tmp32, cpu_tmp32, r_temp2);
    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, y));

    // b1 = N ^ V;
    gen_mov_reg_N(cpu_tmp0, cpu_psr);
    gen_mov_reg_V(r_temp, cpu_psr);
    tcg_gen_xor_tl(cpu_tmp0, cpu_tmp0, r_temp);

    // T0 = (b1 << 31) | (T0 >> 1);
    // src1 = T0;
    tcg_gen_shli_tl(cpu_tmp0, cpu_tmp0, 31);
    tcg_gen_shri_tl(cpu_cc_src, cpu_cc_src, 1);
    tcg_gen_or_tl(cpu_cc_src, cpu_cc_src, cpu_tmp0);

    /* do addition and update flags */
    tcg_gen_add_tl(dst, cpu_cc_src, cpu_cc_src2);
    tcg_gen_mov_tl(cpu_cc_dst, dst);

    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    gen_cc_V_add_icc(cpu_cc_dst, cpu_cc_src, cpu_cc_src2);
    gen_cc_C_add_icc(cpu_cc_dst, cpu_cc_src);
}

static inline void gen_op_umul(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp, r_temp2;

    r_temp = tcg_temp_new(TCG_TYPE_I64);
    r_temp2 = tcg_temp_new(TCG_TYPE_I64);

    tcg_gen_extu_tl_i64(r_temp, src2);
    tcg_gen_extu_tl_i64(r_temp2, src1);
    tcg_gen_mul_i64(r_temp2, r_temp, r_temp2);

    tcg_gen_shri_i64(r_temp, r_temp2, 32);
    tcg_gen_trunc_i64_i32(r_temp, r_temp);
    tcg_gen_st_i32(r_temp, cpu_env, offsetof(CPUSPARCState, y));
#ifdef TARGET_SPARC64
    tcg_gen_mov_i64(dst, r_temp2);
#else
    tcg_gen_trunc_i64_tl(dst, r_temp2);
#endif
}

static inline void gen_op_smul(TCGv dst, TCGv src1, TCGv src2)
{
    TCGv r_temp, r_temp2;

    r_temp = tcg_temp_new(TCG_TYPE_I64);
    r_temp2 = tcg_temp_new(TCG_TYPE_I64);

    tcg_gen_ext_tl_i64(r_temp, src2);
    tcg_gen_ext_tl_i64(r_temp2, src1);
    tcg_gen_mul_i64(r_temp2, r_temp, r_temp2);

    tcg_gen_shri_i64(r_temp, r_temp2, 32);
    tcg_gen_trunc_i64_i32(r_temp, r_temp);
    tcg_gen_st_i32(r_temp, cpu_env, offsetof(CPUSPARCState, y));
#ifdef TARGET_SPARC64
    tcg_gen_mov_i64(dst, r_temp2);
#else
    tcg_gen_trunc_i64_tl(dst, r_temp2);
#endif
}

#ifdef TARGET_SPARC64
static inline void gen_trap_ifdivzero_tl(TCGv divisor)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_brcond_tl(TCG_COND_NE, divisor, tcg_const_tl(0), l1);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_DIV_ZERO));
    gen_set_label(l1);
}

static inline void gen_op_sdivx(TCGv dst, TCGv src1, TCGv src2)
{
    int l1, l2;

    l1 = gen_new_label();
    l2 = gen_new_label();
    tcg_gen_mov_tl(cpu_cc_src, src1);
    tcg_gen_mov_tl(cpu_cc_src2, src2);
    gen_trap_ifdivzero_tl(src2);
    tcg_gen_brcond_tl(TCG_COND_NE, cpu_cc_src, tcg_const_tl(INT64_MIN), l1);
    tcg_gen_brcond_tl(TCG_COND_NE, cpu_cc_src2, tcg_const_tl(-1), l1);
    tcg_gen_movi_i64(dst, INT64_MIN);
    tcg_gen_br(l2);
    gen_set_label(l1);
    tcg_gen_div_i64(dst, cpu_cc_src, cpu_cc_src2);
    gen_set_label(l2);
}
#endif

static inline void gen_op_div_cc(TCGv dst)
{
    int l1;

    tcg_gen_mov_tl(cpu_cc_dst, dst);
    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
    l1 = gen_new_label();
    tcg_gen_ld_tl(cpu_tmp0, cpu_env, offsetof(CPUSPARCState, cc_src2));
    tcg_gen_brcond_tl(TCG_COND_EQ, cpu_tmp0, tcg_const_tl(0), l1);
    tcg_gen_ori_i32(cpu_psr, cpu_psr, PSR_OVF);
    gen_set_label(l1);
}

static inline void gen_op_logic_cc(TCGv dst)
{
    tcg_gen_mov_tl(cpu_cc_dst, dst);

    gen_cc_clear_icc();
    gen_cc_NZ_icc(cpu_cc_dst);
#ifdef TARGET_SPARC64
    gen_cc_clear_xcc();
    gen_cc_NZ_xcc(cpu_cc_dst);
#endif
}

// 1
static inline void gen_op_eval_ba(TCGv dst)
{
    tcg_gen_movi_tl(dst, 1);
}

// Z
static inline void gen_op_eval_be(TCGv dst, TCGv src)
{
    gen_mov_reg_Z(dst, src);
}

// Z | (N ^ V)
static inline void gen_op_eval_ble(TCGv dst, TCGv src)
{
    gen_mov_reg_N(cpu_tmp0, src);
    gen_mov_reg_V(dst, src);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
    gen_mov_reg_Z(cpu_tmp0, src);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
}

// N ^ V
static inline void gen_op_eval_bl(TCGv dst, TCGv src)
{
    gen_mov_reg_V(cpu_tmp0, src);
    gen_mov_reg_N(dst, src);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
}

// C | Z
static inline void gen_op_eval_bleu(TCGv dst, TCGv src)
{
    gen_mov_reg_Z(cpu_tmp0, src);
    gen_mov_reg_C(dst, src);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
}

// C
static inline void gen_op_eval_bcs(TCGv dst, TCGv src)
{
    gen_mov_reg_C(dst, src);
}

// V
static inline void gen_op_eval_bvs(TCGv dst, TCGv src)
{
    gen_mov_reg_V(dst, src);
}

// 0
static inline void gen_op_eval_bn(TCGv dst)
{
    tcg_gen_movi_tl(dst, 0);
}

// N
static inline void gen_op_eval_bneg(TCGv dst, TCGv src)
{
    gen_mov_reg_N(dst, src);
}

// !Z
static inline void gen_op_eval_bne(TCGv dst, TCGv src)
{
    gen_mov_reg_Z(dst, src);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !(Z | (N ^ V))
static inline void gen_op_eval_bg(TCGv dst, TCGv src)
{
    gen_mov_reg_N(cpu_tmp0, src);
    gen_mov_reg_V(dst, src);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
    gen_mov_reg_Z(cpu_tmp0, src);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !(N ^ V)
static inline void gen_op_eval_bge(TCGv dst, TCGv src)
{
    gen_mov_reg_V(cpu_tmp0, src);
    gen_mov_reg_N(dst, src);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !(C | Z)
static inline void gen_op_eval_bgu(TCGv dst, TCGv src)
{
    gen_mov_reg_Z(cpu_tmp0, src);
    gen_mov_reg_C(dst, src);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !C
static inline void gen_op_eval_bcc(TCGv dst, TCGv src)
{
    gen_mov_reg_C(dst, src);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !N
static inline void gen_op_eval_bpos(TCGv dst, TCGv src)
{
    gen_mov_reg_N(dst, src);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !V
static inline void gen_op_eval_bvc(TCGv dst, TCGv src)
{
    gen_mov_reg_V(dst, src);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

/*
  FPSR bit field FCC1 | FCC0:
   0 =
   1 <
   2 >
   3 unordered
*/
static inline void gen_mov_reg_FCC0(TCGv reg, TCGv src,
                                    unsigned int fcc_offset)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, FSR_FCC0_SHIFT + fcc_offset);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

static inline void gen_mov_reg_FCC1(TCGv reg, TCGv src,
                                    unsigned int fcc_offset)
{
    tcg_gen_extu_i32_tl(reg, src);
    tcg_gen_shri_tl(reg, reg, FSR_FCC1_SHIFT + fcc_offset);
    tcg_gen_andi_tl(reg, reg, 0x1);
}

// !0: FCC0 | FCC1
static inline void gen_op_eval_fbne(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
}

// 1 or 2: FCC0 ^ FCC1
static inline void gen_op_eval_fblg(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
}

// 1 or 3: FCC0
static inline void gen_op_eval_fbul(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
}

// 1: FCC0 & !FCC1
static inline void gen_op_eval_fbl(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_xori_tl(cpu_tmp0, cpu_tmp0, 0x1);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
}

// 2 or 3: FCC1
static inline void gen_op_eval_fbug(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC1(dst, src, fcc_offset);
}

// 2: !FCC0 & FCC1
static inline void gen_op_eval_fbg(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    tcg_gen_xori_tl(dst, dst, 0x1);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
}

// 3: FCC0 & FCC1
static inline void gen_op_eval_fbu(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
}

// 0: !(FCC0 | FCC1)
static inline void gen_op_eval_fbe(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_or_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// 0 or 3: !(FCC0 ^ FCC1)
static inline void gen_op_eval_fbue(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_xor_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// 0 or 2: !FCC0
static inline void gen_op_eval_fbge(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !1: !(FCC0 & !FCC1)
static inline void gen_op_eval_fbuge(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_xori_tl(cpu_tmp0, cpu_tmp0, 0x1);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// 0 or 1: !FCC1
static inline void gen_op_eval_fble(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC1(dst, src, fcc_offset);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !2: !(!FCC0 & FCC1)
static inline void gen_op_eval_fbule(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    tcg_gen_xori_tl(dst, dst, 0x1);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

// !3: !(FCC0 & FCC1)
static inline void gen_op_eval_fbo(TCGv dst, TCGv src,
                                    unsigned int fcc_offset)
{
    gen_mov_reg_FCC0(dst, src, fcc_offset);
    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);
    tcg_gen_and_tl(dst, dst, cpu_tmp0);
    tcg_gen_xori_tl(dst, dst, 0x1);
}

static inline void gen_branch2(DisasContext *dc, target_ulong pc1,
                               target_ulong pc2, TCGv r_cond)
{
    int l1;

    l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_EQ, r_cond, tcg_const_tl(0), l1);

    gen_goto_tb(dc, 0, pc1, pc1 + 4);

    gen_set_label(l1);
    gen_goto_tb(dc, 1, pc2, pc2 + 4);
}

static inline void gen_branch_a(DisasContext *dc, target_ulong pc1,
                                target_ulong pc2, TCGv r_cond)
{
    int l1;

    l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_EQ, r_cond, tcg_const_tl(0), l1);

    gen_goto_tb(dc, 0, pc2, pc1);

    gen_set_label(l1);
    gen_goto_tb(dc, 1, pc2 + 4, pc2 + 8);
}

static inline void gen_generic_branch(target_ulong npc1, target_ulong npc2,
                                      TCGv r_cond)
{
    int l1, l2;

    l1 = gen_new_label();
    l2 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_EQ, r_cond, tcg_const_tl(0), l1);

    tcg_gen_movi_tl(cpu_npc, npc1);
    tcg_gen_br(l2);

    gen_set_label(l1);
    tcg_gen_movi_tl(cpu_npc, npc2);
    gen_set_label(l2);
}

/* call this function before using the condition register as it may
   have been set for a jump */
static inline void flush_cond(DisasContext *dc, TCGv cond)
{
    if (dc->npc == JUMP_PC) {
        gen_generic_branch(dc->jump_pc[0], dc->jump_pc[1], cond);
        dc->npc = DYNAMIC_PC;
    }
}

static inline void save_npc(DisasContext *dc, TCGv cond)
{
    if (dc->npc == JUMP_PC) {
        gen_generic_branch(dc->jump_pc[0], dc->jump_pc[1], cond);
        dc->npc = DYNAMIC_PC;
    } else if (dc->npc != DYNAMIC_PC) {
        tcg_gen_movi_tl(cpu_npc, dc->npc);
    }
}

static inline void save_state(DisasContext *dc, TCGv cond)
{
    tcg_gen_movi_tl(cpu_pc, dc->pc);
    save_npc(dc, cond);
}

static inline void gen_mov_pc_npc(DisasContext *dc, TCGv cond)
{
    if (dc->npc == JUMP_PC) {
        gen_generic_branch(dc->jump_pc[0], dc->jump_pc[1], cond);
        tcg_gen_mov_tl(cpu_pc, cpu_npc);
        dc->pc = DYNAMIC_PC;
    } else if (dc->npc == DYNAMIC_PC) {
        tcg_gen_mov_tl(cpu_pc, cpu_npc);
        dc->pc = DYNAMIC_PC;
    } else {
        dc->pc = dc->npc;
    }
}

static inline void gen_op_next_insn(void)
{
    tcg_gen_mov_tl(cpu_pc, cpu_npc);
    tcg_gen_addi_tl(cpu_npc, cpu_npc, 4);
}

static inline void gen_cond(TCGv r_dst, unsigned int cc, unsigned int cond)
{
    TCGv r_src;

#ifdef TARGET_SPARC64
    if (cc)
        r_src = cpu_xcc;
    else
        r_src = cpu_psr;
#else
    r_src = cpu_psr;
#endif
    switch (cond) {
    case 0x0:
        gen_op_eval_bn(r_dst);
        break;
    case 0x1:
        gen_op_eval_be(r_dst, r_src);
        break;
    case 0x2:
        gen_op_eval_ble(r_dst, r_src);
        break;
    case 0x3:
        gen_op_eval_bl(r_dst, r_src);
        break;
    case 0x4:
        gen_op_eval_bleu(r_dst, r_src);
        break;
    case 0x5:
        gen_op_eval_bcs(r_dst, r_src);
        break;
    case 0x6:
        gen_op_eval_bneg(r_dst, r_src);
        break;
    case 0x7:
        gen_op_eval_bvs(r_dst, r_src);
        break;
    case 0x8:
        gen_op_eval_ba(r_dst);
        break;
    case 0x9:
        gen_op_eval_bne(r_dst, r_src);
        break;
    case 0xa:
        gen_op_eval_bg(r_dst, r_src);
        break;
    case 0xb:
        gen_op_eval_bge(r_dst, r_src);
        break;
    case 0xc:
        gen_op_eval_bgu(r_dst, r_src);
        break;
    case 0xd:
        gen_op_eval_bcc(r_dst, r_src);
        break;
    case 0xe:
        gen_op_eval_bpos(r_dst, r_src);
        break;
    case 0xf:
        gen_op_eval_bvc(r_dst, r_src);
        break;
    }
}

static inline void gen_fcond(TCGv r_dst, unsigned int cc, unsigned int cond)
{
    unsigned int offset;

    switch (cc) {
    default:
    case 0x0:
        offset = 0;
        break;
    case 0x1:
        offset = 32 - 10;
        break;
    case 0x2:
        offset = 34 - 10;
        break;
    case 0x3:
        offset = 36 - 10;
        break;
    }

    switch (cond) {
    case 0x0:
        gen_op_eval_bn(r_dst);
        break;
    case 0x1:
        gen_op_eval_fbne(r_dst, cpu_fsr, offset);
        break;
    case 0x2:
        gen_op_eval_fblg(r_dst, cpu_fsr, offset);
        break;
    case 0x3:
        gen_op_eval_fbul(r_dst, cpu_fsr, offset);
        break;
    case 0x4:
        gen_op_eval_fbl(r_dst, cpu_fsr, offset);
        break;
    case 0x5:
        gen_op_eval_fbug(r_dst, cpu_fsr, offset);
        break;
    case 0x6:
        gen_op_eval_fbg(r_dst, cpu_fsr, offset);
        break;
    case 0x7:
        gen_op_eval_fbu(r_dst, cpu_fsr, offset);
        break;
    case 0x8:
        gen_op_eval_ba(r_dst);
        break;
    case 0x9:
        gen_op_eval_fbe(r_dst, cpu_fsr, offset);
        break;
    case 0xa:
        gen_op_eval_fbue(r_dst, cpu_fsr, offset);
        break;
    case 0xb:
        gen_op_eval_fbge(r_dst, cpu_fsr, offset);
        break;
    case 0xc:
        gen_op_eval_fbuge(r_dst, cpu_fsr, offset);
        break;
    case 0xd:
        gen_op_eval_fble(r_dst, cpu_fsr, offset);
        break;
    case 0xe:
        gen_op_eval_fbule(r_dst, cpu_fsr, offset);
        break;
    case 0xf:
        gen_op_eval_fbo(r_dst, cpu_fsr, offset);
        break;
    }
}

#ifdef TARGET_SPARC64
// Inverted logic
static const int gen_tcg_cond_reg[8] = {
    -1,
    TCG_COND_NE,
    TCG_COND_GT,
    TCG_COND_GE,
    -1,
    TCG_COND_EQ,
    TCG_COND_LE,
    TCG_COND_LT,
};

static inline void gen_cond_reg(TCGv r_dst, int cond, TCGv r_src)
{
    int l1;

    l1 = gen_new_label();
    tcg_gen_movi_tl(r_dst, 0);
    tcg_gen_brcond_tl(gen_tcg_cond_reg[cond], r_src, tcg_const_tl(0), l1);
    tcg_gen_movi_tl(r_dst, 1);
    gen_set_label(l1);
}
#endif

/* XXX: potentially incorrect if dynamic npc */
static void do_branch(DisasContext *dc, int32_t offset, uint32_t insn, int cc,
                      TCGv r_cond)
{
    unsigned int cond = GET_FIELD(insn, 3, 6), a = (insn & (1 << 29));
    target_ulong target = dc->pc + offset;

    if (cond == 0x0) {
        /* unconditional not taken */
        if (a) {
            dc->pc = dc->npc + 4;
            dc->npc = dc->pc + 4;
        } else {
            dc->pc = dc->npc;
            dc->npc = dc->pc + 4;
        }
    } else if (cond == 0x8) {
        /* unconditional taken */
        if (a) {
            dc->pc = target;
            dc->npc = dc->pc + 4;
        } else {
            dc->pc = dc->npc;
            dc->npc = target;
        }
    } else {
        flush_cond(dc, r_cond);
        gen_cond(r_cond, cc, cond);
        if (a) {
            gen_branch_a(dc, target, dc->npc, r_cond);
            dc->is_br = 1;
        } else {
            dc->pc = dc->npc;
            dc->jump_pc[0] = target;
            dc->jump_pc[1] = dc->npc + 4;
            dc->npc = JUMP_PC;
        }
    }
}

/* XXX: potentially incorrect if dynamic npc */
static void do_fbranch(DisasContext *dc, int32_t offset, uint32_t insn, int cc,
                      TCGv r_cond)
{
    unsigned int cond = GET_FIELD(insn, 3, 6), a = (insn & (1 << 29));
    target_ulong target = dc->pc + offset;

    if (cond == 0x0) {
        /* unconditional not taken */
        if (a) {
            dc->pc = dc->npc + 4;
            dc->npc = dc->pc + 4;
        } else {
            dc->pc = dc->npc;
            dc->npc = dc->pc + 4;
        }
    } else if (cond == 0x8) {
        /* unconditional taken */
        if (a) {
            dc->pc = target;
            dc->npc = dc->pc + 4;
        } else {
            dc->pc = dc->npc;
            dc->npc = target;
        }
    } else {
        flush_cond(dc, r_cond);
        gen_fcond(r_cond, cc, cond);
        if (a) {
            gen_branch_a(dc, target, dc->npc, r_cond);
            dc->is_br = 1;
        } else {
            dc->pc = dc->npc;
            dc->jump_pc[0] = target;
            dc->jump_pc[1] = dc->npc + 4;
            dc->npc = JUMP_PC;
        }
    }
}

#ifdef TARGET_SPARC64
/* XXX: potentially incorrect if dynamic npc */
static void do_branch_reg(DisasContext *dc, int32_t offset, uint32_t insn,
                          TCGv r_cond, TCGv r_reg)
{
    unsigned int cond = GET_FIELD_SP(insn, 25, 27), a = (insn & (1 << 29));
    target_ulong target = dc->pc + offset;

    flush_cond(dc, r_cond);
    gen_cond_reg(r_cond, cond, r_reg);
    if (a) {
        gen_branch_a(dc, target, dc->npc, r_cond);
        dc->is_br = 1;
    } else {
        dc->pc = dc->npc;
        dc->jump_pc[0] = target;
        dc->jump_pc[1] = dc->npc + 4;
        dc->npc = JUMP_PC;
    }
}

static GenOpFunc * const gen_fcmps[4] = {
    helper_fcmps,
    helper_fcmps_fcc1,
    helper_fcmps_fcc2,
    helper_fcmps_fcc3,
};

static GenOpFunc * const gen_fcmpd[4] = {
    helper_fcmpd,
    helper_fcmpd_fcc1,
    helper_fcmpd_fcc2,
    helper_fcmpd_fcc3,
};

static GenOpFunc * const gen_fcmpq[4] = {
    helper_fcmpq,
    helper_fcmpq_fcc1,
    helper_fcmpq_fcc2,
    helper_fcmpq_fcc3,
};

static GenOpFunc * const gen_fcmpes[4] = {
    helper_fcmpes,
    helper_fcmpes_fcc1,
    helper_fcmpes_fcc2,
    helper_fcmpes_fcc3,
};

static GenOpFunc * const gen_fcmped[4] = {
    helper_fcmped,
    helper_fcmped_fcc1,
    helper_fcmped_fcc2,
    helper_fcmped_fcc3,
};

static GenOpFunc * const gen_fcmpeq[4] = {
    helper_fcmpeq,
    helper_fcmpeq_fcc1,
    helper_fcmpeq_fcc2,
    helper_fcmpeq_fcc3,
};

static inline void gen_op_fcmps(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmps[fccno]);
}

static inline void gen_op_fcmpd(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmpd[fccno]);
}

static inline void gen_op_fcmpq(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmpq[fccno]);
}

static inline void gen_op_fcmpes(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmpes[fccno]);
}

static inline void gen_op_fcmped(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmped[fccno]);
}

static inline void gen_op_fcmpeq(int fccno)
{
    tcg_gen_helper_0_0(gen_fcmpeq[fccno]);
}

#else

static inline void gen_op_fcmps(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmps);
}

static inline void gen_op_fcmpd(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmpd);
}

static inline void gen_op_fcmpq(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmpq);
}

static inline void gen_op_fcmpes(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmpes);
}

static inline void gen_op_fcmped(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmped);
}

static inline void gen_op_fcmpeq(int fccno)
{
    tcg_gen_helper_0_0(helper_fcmpeq);
}
#endif

static inline void gen_op_fpexception_im(int fsr_flags)
{
    tcg_gen_andi_tl(cpu_fsr, cpu_fsr, ~FSR_FTT_MASK);
    tcg_gen_ori_tl(cpu_fsr, cpu_fsr, fsr_flags);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_FP_EXCP));
}

static int gen_trap_ifnofpu(DisasContext *dc, TCGv r_cond)
{
#if !defined(CONFIG_USER_ONLY)
    if (!dc->fpu_enabled) {
        save_state(dc, r_cond);
        tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_NFPU_INSN));
        dc->is_br = 1;
        return 1;
    }
#endif
    return 0;
}

static inline void gen_op_clear_ieee_excp_and_FTT(void)
{
    tcg_gen_andi_tl(cpu_fsr, cpu_fsr, ~(FSR_FTT_MASK | FSR_CEXC_MASK));
}

static inline void gen_clear_float_exceptions(void)
{
    tcg_gen_helper_0_0(helper_clear_float_exceptions);
}

/* asi moves */
#ifdef TARGET_SPARC64
static inline TCGv gen_get_asi(int insn, TCGv r_addr)
{
    int asi, offset;
    TCGv r_asi;

    if (IS_IMM) {
        r_asi = tcg_temp_new(TCG_TYPE_I32);
        offset = GET_FIELD(insn, 25, 31);
        tcg_gen_addi_tl(r_addr, r_addr, offset);
        tcg_gen_ld_i32(r_asi, cpu_env, offsetof(CPUSPARCState, asi));
    } else {
        asi = GET_FIELD(insn, 19, 26);
        r_asi = tcg_const_i32(asi);
    }
    return r_asi;
}

static inline void gen_ld_asi(TCGv dst, TCGv addr, int insn, int size, int sign)
{
    TCGv r_asi;

    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_1_4(helper_ld_asi, dst, addr, r_asi,
                       tcg_const_i32(size), tcg_const_i32(sign));
}

static inline void gen_st_asi(TCGv src, TCGv addr, int insn, int size)
{
    TCGv r_asi;

    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_0_4(helper_st_asi, addr, src, r_asi, tcg_const_i32(size));
}

static inline void gen_ldf_asi(TCGv addr, int insn, int size, int rd)
{
    TCGv r_asi;

    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_0_4(helper_ldf_asi, addr, r_asi, tcg_const_i32(size),
                       tcg_const_i32(rd));
}

static inline void gen_stf_asi(TCGv addr, int insn, int size, int rd)
{
    TCGv r_asi;

    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_0_4(helper_stf_asi, addr, r_asi, tcg_const_i32(size),
                       tcg_const_i32(rd));
}

static inline void gen_swap_asi(TCGv dst, TCGv addr, int insn)
{
    TCGv r_temp, r_asi;

    r_temp = tcg_temp_new(TCG_TYPE_I32);
    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_1_4(helper_ld_asi, r_temp, addr, r_asi,
                       tcg_const_i32(4), tcg_const_i32(0));
    tcg_gen_helper_0_4(helper_st_asi, addr, dst, r_asi,
                       tcg_const_i32(4));
    tcg_gen_extu_i32_tl(dst, r_temp);
}

static inline void gen_ldda_asi(TCGv lo, TCGv hi, TCGv addr, int insn)
{
    TCGv r_asi;

    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_1_4(helper_ld_asi, cpu_tmp64, addr, r_asi,
                       tcg_const_i32(8), tcg_const_i32(0));
    tcg_gen_andi_i64(lo, cpu_tmp64, 0xffffffffULL);
    tcg_gen_shri_i64(cpu_tmp64, cpu_tmp64, 32);
    tcg_gen_andi_i64(hi, cpu_tmp64, 0xffffffffULL);
}

static inline void gen_stda_asi(TCGv hi, TCGv addr, int insn, int rd)
{
    TCGv r_temp, r_asi;

    r_temp = tcg_temp_new(TCG_TYPE_I32);
    gen_movl_reg_TN(rd + 1, r_temp);
    tcg_gen_helper_1_2(helper_pack64, cpu_tmp64, hi,
                       r_temp);
    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_0_4(helper_st_asi, addr, cpu_tmp64, r_asi,
                       tcg_const_i32(8));
}

static inline void gen_cas_asi(TCGv dst, TCGv addr, TCGv val2, int insn, int rd)
{
    TCGv r_val1, r_asi;

    r_val1 = tcg_temp_new(TCG_TYPE_I32);
    gen_movl_reg_TN(rd, r_val1);
    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_1_4(helper_cas_asi, dst, addr, r_val1, val2, r_asi);
}

static inline void gen_casx_asi(TCGv dst, TCGv addr, TCGv val2, int insn, int rd)
{
    TCGv r_asi;

    gen_movl_reg_TN(rd, cpu_tmp64);
    r_asi = gen_get_asi(insn, addr);
    tcg_gen_helper_1_4(helper_casx_asi, dst, addr, cpu_tmp64, val2, r_asi);
}

#elif !defined(CONFIG_USER_ONLY)

static inline void gen_ld_asi(TCGv dst, TCGv addr, int insn, int size, int sign)
{
    int asi;

    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_1_4(helper_ld_asi, cpu_tmp64, addr, tcg_const_i32(asi),
                       tcg_const_i32(size), tcg_const_i32(sign));
    tcg_gen_trunc_i64_tl(dst, cpu_tmp64);
}

static inline void gen_st_asi(TCGv src, TCGv addr, int insn, int size)
{
    int asi;

    tcg_gen_extu_tl_i64(cpu_tmp64, src);
    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_0_4(helper_st_asi, addr, cpu_tmp64, tcg_const_i32(asi),
                       tcg_const_i32(size));
}

static inline void gen_swap_asi(TCGv dst, TCGv addr, int insn)
{
    int asi;
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_I32);
    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_1_4(helper_ld_asi, r_temp, addr, tcg_const_i32(asi),
                       tcg_const_i32(4), tcg_const_i32(0));
    tcg_gen_helper_0_4(helper_st_asi, addr, dst, tcg_const_i32(asi),
                       tcg_const_i32(4));
    tcg_gen_extu_i32_tl(dst, r_temp);
}

static inline void gen_ldda_asi(TCGv lo, TCGv hi, TCGv addr, int insn)
{
    int asi;

    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_1_4(helper_ld_asi, cpu_tmp64, addr, tcg_const_i32(asi),
                       tcg_const_i32(8), tcg_const_i32(0));
    tcg_gen_trunc_i64_tl(lo, cpu_tmp64);
    tcg_gen_shri_i64(cpu_tmp64, cpu_tmp64, 32);
    tcg_gen_trunc_i64_tl(hi, cpu_tmp64);
}

static inline void gen_stda_asi(TCGv hi, TCGv addr, int insn, int rd)
{
    int asi;
    TCGv r_temp;

    r_temp = tcg_temp_new(TCG_TYPE_I32);
    gen_movl_reg_TN(rd + 1, r_temp);
    tcg_gen_helper_1_2(helper_pack64, cpu_tmp64, hi, r_temp);
    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_0_4(helper_st_asi, addr, cpu_tmp64, tcg_const_i32(asi),
                       tcg_const_i32(8));
}
#endif

#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)
static inline void gen_ldstub_asi(TCGv dst, TCGv addr, int insn)
{
    int asi;

    gen_ld_asi(dst, addr, insn, 1, 0);

    asi = GET_FIELD(insn, 19, 26);
    tcg_gen_helper_0_4(helper_st_asi, addr, tcg_const_i64(0xffULL),
                       tcg_const_i32(asi), tcg_const_i32(1));
}
#endif

static inline TCGv get_src1(unsigned int insn, TCGv def)
{
    TCGv r_rs1 = def;
    unsigned int rs1;

    rs1 = GET_FIELD(insn, 13, 17);
    if (rs1 == 0)
        //r_rs1 = tcg_const_tl(0);
        tcg_gen_movi_tl(def, 0);
    else if (rs1 < 8)
        //r_rs1 = cpu_gregs[rs1];
        tcg_gen_mov_tl(def, cpu_gregs[rs1]);
    else
        tcg_gen_ld_tl(def, cpu_regwptr, (rs1 - 8) * sizeof(target_ulong));
    return r_rs1;
}

static inline TCGv get_src2(unsigned int insn, TCGv def)
{
    TCGv r_rs2 = def;
    unsigned int rs2;

    if (IS_IMM) { /* immediate */
        rs2 = GET_FIELDs(insn, 19, 31);
        r_rs2 = tcg_const_tl((int)rs2);
    } else { /* register */
        rs2 = GET_FIELD(insn, 27, 31);
        if (rs2 == 0)
            r_rs2 = tcg_const_tl(0);
        else if (rs2 < 8)
            r_rs2 = cpu_gregs[rs2];
        else
            tcg_gen_ld_tl(def, cpu_regwptr, (rs2 - 8) * sizeof(target_ulong));
    }
    return r_rs2;
}

#define CHECK_IU_FEATURE(dc, FEATURE)                      \
    if (!((dc)->features & CPU_FEATURE_ ## FEATURE))       \
        goto illegal_insn;
#define CHECK_FPU_FEATURE(dc, FEATURE)                     \
    if (!((dc)->features & CPU_FEATURE_ ## FEATURE))       \
        goto nfpu_insn;

/* before an instruction, dc->pc must be static */
static void disas_sparc_insn(DisasContext * dc)
{
    unsigned int insn, opc, rs1, rs2, rd;

    insn = ldl_code(dc->pc);
    opc = GET_FIELD(insn, 0, 1);

    rd = GET_FIELD(insn, 2, 6);

    cpu_dst = cpu_T[0];
    cpu_src1 = cpu_T[0]; // const
    cpu_src2 = cpu_T[1]; // const

    // loads and stores
    cpu_addr = cpu_T[0];
    cpu_val = cpu_T[1];

    switch (opc) {
    case 0:                     /* branches/sethi */
        {
            unsigned int xop = GET_FIELD(insn, 7, 9);
            int32_t target;
            switch (xop) {
#ifdef TARGET_SPARC64
            case 0x1:           /* V9 BPcc */
                {
                    int cc;

                    target = GET_FIELD_SP(insn, 0, 18);
                    target = sign_extend(target, 18);
                    target <<= 2;
                    cc = GET_FIELD_SP(insn, 20, 21);
                    if (cc == 0)
                        do_branch(dc, target, insn, 0, cpu_cond);
                    else if (cc == 2)
                        do_branch(dc, target, insn, 1, cpu_cond);
                    else
                        goto illegal_insn;
                    goto jmp_insn;
                }
            case 0x3:           /* V9 BPr */
                {
                    target = GET_FIELD_SP(insn, 0, 13) |
                        (GET_FIELD_SP(insn, 20, 21) << 14);
                    target = sign_extend(target, 16);
                    target <<= 2;
                    cpu_src1 = get_src1(insn, cpu_src1);
                    do_branch_reg(dc, target, insn, cpu_cond, cpu_src1);
                    goto jmp_insn;
                }
            case 0x5:           /* V9 FBPcc */
                {
                    int cc = GET_FIELD_SP(insn, 20, 21);
                    if (gen_trap_ifnofpu(dc, cpu_cond))
                        goto jmp_insn;
                    target = GET_FIELD_SP(insn, 0, 18);
                    target = sign_extend(target, 19);
                    target <<= 2;
                    do_fbranch(dc, target, insn, cc, cpu_cond);
                    goto jmp_insn;
                }
#else
            case 0x7:           /* CBN+x */
                {
                    goto ncp_insn;
                }
#endif
            case 0x2:           /* BN+x */
                {
                    target = GET_FIELD(insn, 10, 31);
                    target = sign_extend(target, 22);
                    target <<= 2;
                    do_branch(dc, target, insn, 0, cpu_cond);
                    goto jmp_insn;
                }
            case 0x6:           /* FBN+x */
                {
                    if (gen_trap_ifnofpu(dc, cpu_cond))
                        goto jmp_insn;
                    target = GET_FIELD(insn, 10, 31);
                    target = sign_extend(target, 22);
                    target <<= 2;
                    do_fbranch(dc, target, insn, 0, cpu_cond);
                    goto jmp_insn;
                }
            case 0x4:           /* SETHI */
                if (rd) { // nop
                    uint32_t value = GET_FIELD(insn, 10, 31);
                    gen_movl_TN_reg(rd, tcg_const_tl(value << 10));
                }
                break;
            case 0x0:           /* UNIMPL */
            default:
                goto illegal_insn;
            }
            break;
        }
        break;
    case 1:
        /*CALL*/ {
            target_long target = GET_FIELDs(insn, 2, 31) << 2;

            gen_movl_TN_reg(15, tcg_const_tl(dc->pc));
            target += dc->pc;
            gen_mov_pc_npc(dc, cpu_cond);
            dc->npc = target;
        }
        goto jmp_insn;
    case 2:                     /* FPU & Logical Operations */
        {
            unsigned int xop = GET_FIELD(insn, 7, 12);
            if (xop == 0x3a) {  /* generate trap */
                int cond;

                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {
                    rs2 = GET_FIELD(insn, 25, 31);
                    tcg_gen_addi_tl(cpu_dst, cpu_src1, rs2);
                } else {
                    rs2 = GET_FIELD(insn, 27, 31);
                    if (rs2 != 0) {
                        gen_movl_reg_TN(rs2, cpu_src2);
                        tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);
                    } else
                        tcg_gen_mov_tl(cpu_dst, cpu_src1);
                }
                cond = GET_FIELD(insn, 3, 6);
                if (cond == 0x8) {
                    save_state(dc, cpu_cond);
                    tcg_gen_helper_0_1(helper_trap, cpu_dst);
                } else if (cond != 0) {
                    TCGv r_cond = tcg_temp_new(TCG_TYPE_TL);
#ifdef TARGET_SPARC64
                    /* V9 icc/xcc */
                    int cc = GET_FIELD_SP(insn, 11, 12);

                    save_state(dc, cpu_cond);
                    if (cc == 0)
                        gen_cond(r_cond, 0, cond);
                    else if (cc == 2)
                        gen_cond(r_cond, 1, cond);
                    else
                        goto illegal_insn;
#else
                    save_state(dc, cpu_cond);
                    gen_cond(r_cond, 0, cond);
#endif
                    tcg_gen_helper_0_2(helper_trapcc, cpu_dst, r_cond);
                }
                gen_op_next_insn();
                tcg_gen_exit_tb(0);
                dc->is_br = 1;
                goto jmp_insn;
            } else if (xop == 0x28) {
                rs1 = GET_FIELD(insn, 13, 17);
                switch(rs1) {
                case 0: /* rdy */
#ifndef TARGET_SPARC64
                case 0x01 ... 0x0e: /* undefined in the SPARCv8
                                       manual, rdy on the microSPARC
                                       II */
                case 0x0f:          /* stbar in the SPARCv8 manual,
                                       rdy on the microSPARC II */
                case 0x10 ... 0x1f: /* implementation-dependent in the
                                       SPARCv8 manual, rdy on the
                                       microSPARC II */
#endif
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, y));
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
#ifdef TARGET_SPARC64
                case 0x2: /* V9 rdccr */
                    tcg_gen_helper_1_0(helper_rdccr, cpu_dst);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x3: /* V9 rdasi */
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, asi));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x4: /* V9 rdtick */
                    {
                        TCGv r_tickptr;

                        r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                       offsetof(CPUState, tick));
                        tcg_gen_helper_1_1(helper_tick_get_count, cpu_dst,
                                           r_tickptr);
                        gen_movl_TN_reg(rd, cpu_dst);
                    }
                    break;
                case 0x5: /* V9 rdpc */
                    gen_movl_TN_reg(rd, tcg_const_tl(dc->pc));
                    break;
                case 0x6: /* V9 rdfprs */
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fprs));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0xf: /* V9 membar */
                    break; /* no effect */
                case 0x13: /* Graphics Status */
                    if (gen_trap_ifnofpu(dc, cpu_cond))
                        goto jmp_insn;
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, gsr));
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x17: /* Tick compare */
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, tick_cmpr));
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x18: /* System tick */
                    {
                        TCGv r_tickptr;

                        r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                       offsetof(CPUState, stick));
                        tcg_gen_helper_1_1(helper_tick_get_count, cpu_dst,
                                           r_tickptr);
                        gen_movl_TN_reg(rd, cpu_dst);
                    }
                    break;
                case 0x19: /* System tick compare */
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, stick_cmpr));
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x10: /* Performance Control */
                case 0x11: /* Performance Instrumentation Counter */
                case 0x12: /* Dispatch Control */
                case 0x14: /* Softint set, WO */
                case 0x15: /* Softint clear, WO */
                case 0x16: /* Softint write */
#endif
                default:
                    goto illegal_insn;
                }
#if !defined(CONFIG_USER_ONLY)
            } else if (xop == 0x29) { /* rdpsr / UA2005 rdhpr */
#ifndef TARGET_SPARC64
                if (!supervisor(dc))
                    goto priv_insn;
                tcg_gen_helper_1_0(helper_rdpsr, cpu_dst);
#else
                if (!hypervisor(dc))
                    goto priv_insn;
                rs1 = GET_FIELD(insn, 13, 17);
                switch (rs1) {
                case 0: // hpstate
                    // gen_op_rdhpstate();
                    break;
                case 1: // htstate
                    // gen_op_rdhtstate();
                    break;
                case 3: // hintp
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, hintp));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 5: // htba
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, htba));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 6: // hver
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, hver));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 31: // hstick_cmpr
                    tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                    tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, hstick_cmpr));
                    break;
                default:
                    goto illegal_insn;
                }
#endif
                gen_movl_TN_reg(rd, cpu_dst);
                break;
            } else if (xop == 0x2a) { /* rdwim / V9 rdpr */
                if (!supervisor(dc))
                    goto priv_insn;
#ifdef TARGET_SPARC64
                rs1 = GET_FIELD(insn, 13, 17);
                switch (rs1) {
                case 0: // tpc
                    {
                        TCGv r_tsptr;

                        r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                       offsetof(CPUState, tsptr));
                        tcg_gen_ld_tl(cpu_dst, r_tsptr,
                                      offsetof(trap_state, tpc));
                    }
                    break;
                case 1: // tnpc
                    {
                        TCGv r_tsptr;

                        r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                       offsetof(CPUState, tsptr));
                        tcg_gen_ld_tl(cpu_dst, r_tsptr,
                                      offsetof(trap_state, tnpc));
                    }
                    break;
                case 2: // tstate
                    {
                        TCGv r_tsptr;

                        r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                       offsetof(CPUState, tsptr));
                        tcg_gen_ld_tl(cpu_dst, r_tsptr,
                                      offsetof(trap_state, tstate));
                    }
                    break;
                case 3: // tt
                    {
                        TCGv r_tsptr;

                        r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                       offsetof(CPUState, tsptr));
                        tcg_gen_ld_i32(cpu_dst, r_tsptr,
                                       offsetof(trap_state, tt));
                    }
                    break;
                case 4: // tick
                    {
                        TCGv r_tickptr;

                        r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                        tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                       offsetof(CPUState, tick));
                        tcg_gen_helper_1_1(helper_tick_get_count, cpu_dst,
                                           r_tickptr);
                        gen_movl_TN_reg(rd, cpu_dst);
                    }
                    break;
                case 5: // tba
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, tbr));
                    break;
                case 6: // pstate
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, pstate));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 7: // tl
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, tl));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 8: // pil
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, psrpil));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 9: // cwp
                    tcg_gen_helper_1_0(helper_rdcwp, cpu_dst);
                    break;
                case 10: // cansave
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, cansave));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 11: // canrestore
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, canrestore));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 12: // cleanwin
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, cleanwin));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 13: // otherwin
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, otherwin));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 14: // wstate
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, wstate));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 16: // UA2005 gl
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, gl));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 26: // UA2005 strand status
                    if (!hypervisor(dc))
                        goto priv_insn;
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, ssr));
                    tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
                    break;
                case 31: // ver
                    tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, version));
                    break;
                case 15: // fq
                default:
                    goto illegal_insn;
                }
#else
                tcg_gen_ld_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, wim));
                tcg_gen_ext_i32_tl(cpu_dst, cpu_tmp32);
#endif
                gen_movl_TN_reg(rd, cpu_dst);
                break;
            } else if (xop == 0x2b) { /* rdtbr / V9 flushw */
#ifdef TARGET_SPARC64
                tcg_gen_helper_0_0(helper_flushw);
#else
                if (!supervisor(dc))
                    goto priv_insn;
                tcg_gen_ld_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, tbr));
                gen_movl_TN_reg(rd, cpu_dst);
#endif
                break;
#endif
            } else if (xop == 0x34) {   /* FPU Operations */
                if (gen_trap_ifnofpu(dc, cpu_cond))
                    goto jmp_insn;
                gen_op_clear_ieee_excp_and_FTT();
                rs1 = GET_FIELD(insn, 13, 17);
                rs2 = GET_FIELD(insn, 27, 31);
                xop = GET_FIELD(insn, 18, 26);
                switch (xop) {
                    case 0x1: /* fmovs */
                        gen_op_load_fpr_FT0(rs2);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x5: /* fnegs */
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fnegs);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x9: /* fabss */
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fabss);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x29: /* fsqrts */
                        CHECK_FPU_FEATURE(dc, FSQRT);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsqrts);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x2a: /* fsqrtd */
                        CHECK_FPU_FEATURE(dc, FSQRT);
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsqrtd);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x2b: /* fsqrtq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsqrtq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x41:
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fadds);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x42:
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_faddd);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x43: /* faddq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_faddq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x45:
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsubs);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x46:
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsubd);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x47: /* fsubq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsubq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x49: /* fmuls */
                        CHECK_FPU_FEATURE(dc, FMUL);
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fmuls);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x4a: /* fmuld */
                        CHECK_FPU_FEATURE(dc, FMUL);
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fmuld);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x4b: /* fmulq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        CHECK_FPU_FEATURE(dc, FMUL);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fmulq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x4d:
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdivs);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x4e:
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdivd);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x4f: /* fdivq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdivq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x69:
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fsmuld);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x6e: /* fdmulq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdmulq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0xc4:
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fitos);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0xc6:
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdtos);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0xc7: /* fqtos */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fqtos);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0xc8:
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fitod);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0xc9:
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fstod);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0xcb: /* fqtod */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fqtod);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0xcc: /* fitoq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fitoq);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0xcd: /* fstoq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_FT1(rs2);
                        tcg_gen_helper_0_0(helper_fstoq);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0xce: /* fdtoq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        tcg_gen_helper_0_0(helper_fdtoq);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0xd1:
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fstoi);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0xd2:
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdtoi);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0xd3: /* fqtoi */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fqtoi);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
#ifdef TARGET_SPARC64
                    case 0x2: /* V9 fmovd */
                        gen_op_load_fpr_DT0(DFPREG(rs2));
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x3: /* V9 fmovq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs2));
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x6: /* V9 fnegd */
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        tcg_gen_helper_0_0(helper_fnegd);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x7: /* V9 fnegq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        tcg_gen_helper_0_0(helper_fnegq);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0xa: /* V9 fabsd */
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        tcg_gen_helper_0_0(helper_fabsd);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0xb: /* V9 fabsq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        tcg_gen_helper_0_0(helper_fabsq);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
                    case 0x81: /* V9 fstox */
                        gen_op_load_fpr_FT1(rs2);
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fstox);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x82: /* V9 fdtox */
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fdtox);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x83: /* V9 fqtox */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fqtox);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x84: /* V9 fxtos */
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fxtos);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_FT0_fpr(rd);
                        break;
                    case 0x88: /* V9 fxtod */
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fxtod);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_DT0_fpr(DFPREG(rd));
                        break;
                    case 0x8c: /* V9 fxtoq */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_clear_float_exceptions();
                        tcg_gen_helper_0_0(helper_fxtoq);
                        tcg_gen_helper_0_0(helper_check_ieee_exceptions);
                        gen_op_store_QT0_fpr(QFPREG(rd));
                        break;
#endif
                    default:
                        goto illegal_insn;
                }
            } else if (xop == 0x35) {   /* FPU Operations */
#ifdef TARGET_SPARC64
                int cond;
#endif
                if (gen_trap_ifnofpu(dc, cpu_cond))
                    goto jmp_insn;
                gen_op_clear_ieee_excp_and_FTT();
                rs1 = GET_FIELD(insn, 13, 17);
                rs2 = GET_FIELD(insn, 27, 31);
                xop = GET_FIELD(insn, 18, 26);
#ifdef TARGET_SPARC64
                if ((xop & 0x11f) == 0x005) { // V9 fmovsr
                    int l1;

                    l1 = gen_new_label();
                    cond = GET_FIELD_SP(insn, 14, 17);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    tcg_gen_brcond_tl(gen_tcg_cond_reg[cond], cpu_src1,
                                      tcg_const_tl(0), l1);
                    gen_op_load_fpr_FT0(rs2);
                    gen_op_store_FT0_fpr(rd);
                    gen_set_label(l1);
                    break;
                } else if ((xop & 0x11f) == 0x006) { // V9 fmovdr
                    int l1;

                    l1 = gen_new_label();
                    cond = GET_FIELD_SP(insn, 14, 17);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    tcg_gen_brcond_tl(gen_tcg_cond_reg[cond], cpu_src1,
                                      tcg_const_tl(0), l1);
                    gen_op_load_fpr_DT0(DFPREG(rs2));
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    gen_set_label(l1);
                    break;
                } else if ((xop & 0x11f) == 0x007) { // V9 fmovqr
                    int l1;

                    CHECK_FPU_FEATURE(dc, FLOAT128);
                    l1 = gen_new_label();
                    cond = GET_FIELD_SP(insn, 14, 17);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    tcg_gen_brcond_tl(gen_tcg_cond_reg[cond], cpu_src1,
                                      tcg_const_tl(0), l1);
                    gen_op_load_fpr_QT0(QFPREG(rs2));
                    gen_op_store_QT0_fpr(QFPREG(rd));
                    gen_set_label(l1);
                    break;
                }
#endif
                switch (xop) {
#ifdef TARGET_SPARC64
#define FMOVCC(size_FDQ, fcc)                                           \
                    {                                                   \
                        TCGv r_cond;                                    \
                        int l1;                                         \
                                                                        \
                        l1 = gen_new_label();                           \
                        r_cond = tcg_temp_new(TCG_TYPE_TL);             \
                        cond = GET_FIELD_SP(insn, 14, 17);              \
                        gen_fcond(r_cond, fcc, cond);                   \
                        tcg_gen_brcond_tl(TCG_COND_EQ, r_cond,          \
                                          tcg_const_tl(0), l1);         \
                        glue(glue(gen_op_load_fpr_, size_FDQ), T0)(glue(size_FDQ, FPREG(rs2))); \
                        glue(glue(gen_op_store_, size_FDQ), T0_fpr)(glue(size_FDQ, FPREG(rd))); \
                        gen_set_label(l1);                              \
                    }
                    case 0x001: /* V9 fmovscc %fcc0 */
                        FMOVCC(F, 0);
                        break;
                    case 0x002: /* V9 fmovdcc %fcc0 */
                        FMOVCC(D, 0);
                        break;
                    case 0x003: /* V9 fmovqcc %fcc0 */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 0);
                        break;
                    case 0x041: /* V9 fmovscc %fcc1 */
                        FMOVCC(F, 1);
                        break;
                    case 0x042: /* V9 fmovdcc %fcc1 */
                        FMOVCC(D, 1);
                        break;
                    case 0x043: /* V9 fmovqcc %fcc1 */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 1);
                        break;
                    case 0x081: /* V9 fmovscc %fcc2 */
                        FMOVCC(F, 2);
                        break;
                    case 0x082: /* V9 fmovdcc %fcc2 */
                        FMOVCC(D, 2);
                        break;
                    case 0x083: /* V9 fmovqcc %fcc2 */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 2);
                        break;
                    case 0x0c1: /* V9 fmovscc %fcc3 */
                        FMOVCC(F, 3);
                        break;
                    case 0x0c2: /* V9 fmovdcc %fcc3 */
                        FMOVCC(D, 3);
                        break;
                    case 0x0c3: /* V9 fmovqcc %fcc3 */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 3);
                        break;
#undef FMOVCC
#define FMOVCC(size_FDQ, icc)                                           \
                    {                                                   \
                        TCGv r_cond;                                    \
                        int l1;                                         \
                                                                        \
                        l1 = gen_new_label();                           \
                        r_cond = tcg_temp_new(TCG_TYPE_TL);             \
                        cond = GET_FIELD_SP(insn, 14, 17);              \
                        gen_cond(r_cond, icc, cond);                    \
                        tcg_gen_brcond_tl(TCG_COND_EQ, r_cond,          \
                                          tcg_const_tl(0), l1);         \
                        glue(glue(gen_op_load_fpr_, size_FDQ), T0)(glue(size_FDQ, FPREG(rs2))); \
                        glue(glue(gen_op_store_, size_FDQ), T0_fpr)(glue(size_FDQ, FPREG(rd))); \
                        gen_set_label(l1);                              \
                    }

                    case 0x101: /* V9 fmovscc %icc */
                        FMOVCC(F, 0);
                        break;
                    case 0x102: /* V9 fmovdcc %icc */
                        FMOVCC(D, 0);
                    case 0x103: /* V9 fmovqcc %icc */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 0);
                        break;
                    case 0x181: /* V9 fmovscc %xcc */
                        FMOVCC(F, 1);
                        break;
                    case 0x182: /* V9 fmovdcc %xcc */
                        FMOVCC(D, 1);
                        break;
                    case 0x183: /* V9 fmovqcc %xcc */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        FMOVCC(Q, 1);
                        break;
#undef FMOVCC
#endif
                    case 0x51: /* fcmps, V9 %fcc */
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_op_fcmps(rd & 3);
                        break;
                    case 0x52: /* fcmpd, V9 %fcc */
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_op_fcmpd(rd & 3);
                        break;
                    case 0x53: /* fcmpq, V9 %fcc */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_op_fcmpq(rd & 3);
                        break;
                    case 0x55: /* fcmpes, V9 %fcc */
                        gen_op_load_fpr_FT0(rs1);
                        gen_op_load_fpr_FT1(rs2);
                        gen_op_fcmpes(rd & 3);
                        break;
                    case 0x56: /* fcmped, V9 %fcc */
                        gen_op_load_fpr_DT0(DFPREG(rs1));
                        gen_op_load_fpr_DT1(DFPREG(rs2));
                        gen_op_fcmped(rd & 3);
                        break;
                    case 0x57: /* fcmpeq, V9 %fcc */
                        CHECK_FPU_FEATURE(dc, FLOAT128);
                        gen_op_load_fpr_QT0(QFPREG(rs1));
                        gen_op_load_fpr_QT1(QFPREG(rs2));
                        gen_op_fcmpeq(rd & 3);
                        break;
                    default:
                        goto illegal_insn;
                }
            } else if (xop == 0x2) {
                // clr/mov shortcut

                rs1 = GET_FIELD(insn, 13, 17);
                if (rs1 == 0) {
                    // or %g0, x, y -> mov T0, x; mov y, T0
                    if (IS_IMM) {       /* immediate */
                        rs2 = GET_FIELDs(insn, 19, 31);
                        gen_movl_TN_reg(rd, tcg_const_tl((int)rs2));
                    } else {            /* register */
                        rs2 = GET_FIELD(insn, 27, 31);
                        gen_movl_reg_TN(rs2, cpu_dst);
                        gen_movl_TN_reg(rd, cpu_dst);
                    }
                } else {
                    cpu_src1 = get_src1(insn, cpu_src1);
                    if (IS_IMM) {       /* immediate */
                        rs2 = GET_FIELDs(insn, 19, 31);
                        tcg_gen_ori_tl(cpu_dst, cpu_src1, (int)rs2);
                        gen_movl_TN_reg(rd, cpu_dst);
                    } else {            /* register */
                        // or x, %g0, y -> mov T1, x; mov y, T1
                        rs2 = GET_FIELD(insn, 27, 31);
                        if (rs2 != 0) {
                            gen_movl_reg_TN(rs2, cpu_src2);
                            tcg_gen_or_tl(cpu_dst, cpu_src1, cpu_src2);
                            gen_movl_TN_reg(rd, cpu_dst);
                        } else
                            gen_movl_TN_reg(rd, cpu_src1);
                    }
                }
#ifdef TARGET_SPARC64
            } else if (xop == 0x25) { /* sll, V9 sllx */
                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {   /* immediate */
                    rs2 = GET_FIELDs(insn, 20, 31);
                    if (insn & (1 << 12)) {
                        tcg_gen_shli_i64(cpu_dst, cpu_src1, rs2 & 0x3f);
                    } else {
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_shli_i64(cpu_dst, cpu_dst, rs2 & 0x1f);
                    }
                } else {                /* register */
                    rs2 = GET_FIELD(insn, 27, 31);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    if (insn & (1 << 12)) {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);
                        tcg_gen_shl_i64(cpu_dst, cpu_src1, cpu_tmp0);
                    } else {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_shl_i64(cpu_dst, cpu_dst, cpu_tmp0);
                    }
                }
                gen_movl_TN_reg(rd, cpu_dst);
            } else if (xop == 0x26) { /* srl, V9 srlx */
                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {   /* immediate */
                    rs2 = GET_FIELDs(insn, 20, 31);
                    if (insn & (1 << 12)) {
                        tcg_gen_shri_i64(cpu_dst, cpu_src1, rs2 & 0x3f);
                    } else {
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_shri_i64(cpu_dst, cpu_dst, rs2 & 0x1f);
                    }
                } else {                /* register */
                    rs2 = GET_FIELD(insn, 27, 31);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    if (insn & (1 << 12)) {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);
                        tcg_gen_shr_i64(cpu_dst, cpu_src1, cpu_tmp0);
                    } else {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_shr_i64(cpu_dst, cpu_dst, cpu_tmp0);
                    }
                }
                gen_movl_TN_reg(rd, cpu_dst);
            } else if (xop == 0x27) { /* sra, V9 srax */
                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {   /* immediate */
                    rs2 = GET_FIELDs(insn, 20, 31);
                    if (insn & (1 << 12)) {
                        tcg_gen_sari_i64(cpu_dst, cpu_src1, rs2 & 0x3f);
                    } else {
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_ext_i32_i64(cpu_dst, cpu_dst);
                        tcg_gen_sari_i64(cpu_dst, cpu_dst, rs2 & 0x1f);
                    }
                } else {                /* register */
                    rs2 = GET_FIELD(insn, 27, 31);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    if (insn & (1 << 12)) {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);
                        tcg_gen_sar_i64(cpu_dst, cpu_src1, cpu_tmp0);
                    } else {
                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);
                        tcg_gen_sar_i64(cpu_dst, cpu_dst, cpu_tmp0);
                    }
                }
                gen_movl_TN_reg(rd, cpu_dst);
#endif
            } else if (xop < 0x36) {
                cpu_src1 = get_src1(insn, cpu_src1);
                cpu_src2 = get_src2(insn, cpu_src2);
                if (xop < 0x20) {
                    switch (xop & ~0x10) {
                    case 0x0:
                        if (xop & 0x10)
                            gen_op_add_cc(cpu_dst, cpu_src1, cpu_src2);
                        else
                            tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);
                        break;
                    case 0x1:
                        tcg_gen_and_tl(cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x2:
                        tcg_gen_or_tl(cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x3:
                        tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x4:
                        if (xop & 0x10)
                            gen_op_sub_cc(cpu_dst, cpu_src1, cpu_src2);
                        else
                            tcg_gen_sub_tl(cpu_dst, cpu_src1, cpu_src2);
                        break;
                    case 0x5:
                        tcg_gen_xori_tl(cpu_tmp0, cpu_src2, -1);
                        tcg_gen_and_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x6:
                        tcg_gen_xori_tl(cpu_tmp0, cpu_src2, -1);
                        tcg_gen_or_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x7:
                        tcg_gen_xori_tl(cpu_tmp0, cpu_src2, -1);
                        tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0x8:
                        if (xop & 0x10)
                            gen_op_addx_cc(cpu_dst, cpu_src1, cpu_src2);
                        else {
                            gen_mov_reg_C(cpu_tmp0, cpu_psr);
                            tcg_gen_add_tl(cpu_tmp0, cpu_src2, cpu_tmp0);
                            tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        }
                        break;
#ifdef TARGET_SPARC64
                    case 0x9: /* V9 mulx */
                        tcg_gen_mul_i64(cpu_dst, cpu_src1, cpu_src2);
                        break;
#endif
                    case 0xa:
                        CHECK_IU_FEATURE(dc, MUL);
                        gen_op_umul(cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0xb:
                        CHECK_IU_FEATURE(dc, MUL);
                        gen_op_smul(cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_logic_cc(cpu_dst);
                        break;
                    case 0xc:
                        if (xop & 0x10)
                            gen_op_subx_cc(cpu_dst, cpu_src1, cpu_src2);
                        else {
                            gen_mov_reg_C(cpu_tmp0, cpu_psr);
                            tcg_gen_add_tl(cpu_tmp0, cpu_src2, cpu_tmp0);
                            tcg_gen_sub_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        }
                        break;
#ifdef TARGET_SPARC64
                    case 0xd: /* V9 udivx */
                        gen_trap_ifdivzero_tl(cpu_src2);
                        tcg_gen_divu_i64(cpu_dst, cpu_src1, cpu_src2);
                        break;
#endif
                    case 0xe:
                        CHECK_IU_FEATURE(dc, DIV);
                        tcg_gen_helper_1_2(helper_udiv, cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_div_cc(cpu_dst);
                        break;
                    case 0xf:
                        CHECK_IU_FEATURE(dc, DIV);
                        tcg_gen_helper_1_2(helper_sdiv, cpu_dst, cpu_src1, cpu_src2);
                        if (xop & 0x10)
                            gen_op_div_cc(cpu_dst);
                        break;
                    default:
                        goto illegal_insn;
                    }
                    gen_movl_TN_reg(rd, cpu_dst);
                } else {
                    switch (xop) {
                    case 0x20: /* taddcc */
                        gen_op_tadd_cc(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x21: /* tsubcc */
                        gen_op_tsub_cc(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x22: /* taddcctv */
                        save_state(dc, cpu_cond);
                        gen_op_tadd_ccTV(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x23: /* tsubcctv */
                        save_state(dc, cpu_cond);
                        gen_op_tsub_ccTV(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x24: /* mulscc */
                        gen_op_mulscc(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
#ifndef TARGET_SPARC64
                    case 0x25:  /* sll */
                        tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_shl_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x26:  /* srl */
                        tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_shr_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x27:  /* sra */
                        tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);
                        tcg_gen_sar_tl(cpu_dst, cpu_src1, cpu_tmp0);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
#endif
                    case 0x30:
                        {
                            switch(rd) {
                            case 0: /* wry */
                                tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                                tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, y));
                                break;
#ifndef TARGET_SPARC64
                            case 0x01 ... 0x0f: /* undefined in the
                                                   SPARCv8 manual, nop
                                                   on the microSPARC
                                                   II */
                            case 0x10 ... 0x1f: /* implementation-dependent
                                                   in the SPARCv8
                                                   manual, nop on the
                                                   microSPARC II */
                                break;
#else
                            case 0x2: /* V9 wrccr */
                                tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                                tcg_gen_helper_0_1(helper_wrccr, cpu_dst);
                                break;
                            case 0x3: /* V9 wrasi */
                                tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, asi));
                                break;
                            case 0x6: /* V9 wrfprs */
                                tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, fprs));
                                save_state(dc, cpu_cond);
                                gen_op_next_insn();
                                tcg_gen_exit_tb(0);
                                dc->is_br = 1;
                                break;
                            case 0xf: /* V9 sir, nop if user */
#if !defined(CONFIG_USER_ONLY)
                                if (supervisor(dc))
                                    ; // XXX
#endif
                                break;
                            case 0x13: /* Graphics Status */
                                if (gen_trap_ifnofpu(dc, cpu_cond))
                                    goto jmp_insn;
                                tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                                tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, gsr));
                                break;
                            case 0x17: /* Tick compare */
#if !defined(CONFIG_USER_ONLY)
                                if (!supervisor(dc))
                                    goto illegal_insn;
#endif
                                {
                                    TCGv r_tickptr;

                                    tcg_gen_xor_tl(cpu_dst, cpu_src1,
                                                   cpu_src2);
                                    tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState,
                                                                 tick_cmpr));
                                    r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                                   offsetof(CPUState, tick));
                                    tcg_gen_helper_0_2(helper_tick_set_limit,
                                                       r_tickptr, cpu_dst);
                                }
                                break;
                            case 0x18: /* System tick */
#if !defined(CONFIG_USER_ONLY)
                                if (!supervisor(dc))
                                    goto illegal_insn;
#endif
                                {
                                    TCGv r_tickptr;

                                    tcg_gen_xor_tl(cpu_dst, cpu_src1,
                                                   cpu_src2);
                                    r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                                   offsetof(CPUState, stick));
                                    tcg_gen_helper_0_2(helper_tick_set_count,
                                                       r_tickptr, cpu_dst);
                                }
                                break;
                            case 0x19: /* System tick compare */
#if !defined(CONFIG_USER_ONLY)
                                if (!supervisor(dc))
                                    goto illegal_insn;
#endif
                                {
                                    TCGv r_tickptr;

                                    tcg_gen_xor_tl(cpu_dst, cpu_src1,
                                                   cpu_src2);
                                    tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState,
                                                                 stick_cmpr));
                                    r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                                   offsetof(CPUState, stick));
                                    tcg_gen_helper_0_2(helper_tick_set_limit,
                                                       r_tickptr, cpu_dst);
                                }
                                break;

                            case 0x10: /* Performance Control */
                            case 0x11: /* Performance Instrumentation Counter */
                            case 0x12: /* Dispatch Control */
                            case 0x14: /* Softint set */
                            case 0x15: /* Softint clear */
                            case 0x16: /* Softint write */
#endif
                            default:
                                goto illegal_insn;
                            }
                        }
                        break;
#if !defined(CONFIG_USER_ONLY)
                    case 0x31: /* wrpsr, V9 saved, restored */
                        {
                            if (!supervisor(dc))
                                goto priv_insn;
#ifdef TARGET_SPARC64
                            switch (rd) {
                            case 0:
                                tcg_gen_helper_0_0(helper_saved);
                                break;
                            case 1:
                                tcg_gen_helper_0_0(helper_restored);
                                break;
                            case 2: /* UA2005 allclean */
                            case 3: /* UA2005 otherw */
                            case 4: /* UA2005 normalw */
                            case 5: /* UA2005 invalw */
                                // XXX
                            default:
                                goto illegal_insn;
                            }
#else
                            tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
                            tcg_gen_helper_0_1(helper_wrpsr, cpu_dst);
                            save_state(dc, cpu_cond);
                            gen_op_next_insn();
                            tcg_gen_exit_tb(0);
                            dc->is_br = 1;
#endif
                        }
                        break;
                    case 0x32: /* wrwim, V9 wrpr */
                        {
                            if (!supervisor(dc))
                                goto priv_insn;
                            tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);
#ifdef TARGET_SPARC64
                            switch (rd) {
                            case 0: // tpc
                                {
                                    TCGv r_tsptr;

                                    r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                                   offsetof(CPUState, tsptr));
                                    tcg_gen_st_tl(cpu_dst, r_tsptr,
                                                  offsetof(trap_state, tpc));
                                }
                                break;
                            case 1: // tnpc
                                {
                                    TCGv r_tsptr;

                                    r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                                   offsetof(CPUState, tsptr));
                                    tcg_gen_st_tl(cpu_dst, r_tsptr,
                                                  offsetof(trap_state, tnpc));
                                }
                                break;
                            case 2: // tstate
                                {
                                    TCGv r_tsptr;

                                    r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                                   offsetof(CPUState, tsptr));
                                    tcg_gen_st_tl(cpu_dst, r_tsptr,
                                                  offsetof(trap_state, tstate));
                                }
                                break;
                            case 3: // tt
                                {
                                    TCGv r_tsptr;

                                    r_tsptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tsptr, cpu_env,
                                                   offsetof(CPUState, tsptr));
                                    tcg_gen_st_i32(cpu_dst, r_tsptr,
                                                   offsetof(trap_state, tt));
                                }
                                break;
                            case 4: // tick
                                {
                                    TCGv r_tickptr;

                                    r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                                   offsetof(CPUState, tick));
                                    tcg_gen_helper_0_2(helper_tick_set_count,
                                                       r_tickptr, cpu_dst);
                                }
                                break;
                            case 5: // tba
                                tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, tbr));
                                break;
                            case 6: // pstate
                                save_state(dc, cpu_cond);
                                tcg_gen_helper_0_1(helper_wrpstate, cpu_dst);
                                gen_op_next_insn();
                                tcg_gen_exit_tb(0);
                                dc->is_br = 1;
                                break;
                            case 7: // tl
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, tl));
                                break;
                            case 8: // pil
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, psrpil));
                                break;
                            case 9: // cwp
                                tcg_gen_helper_0_1(helper_wrcwp, cpu_dst);
                                break;
                            case 10: // cansave
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, cansave));
                                break;
                            case 11: // canrestore
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, canrestore));
                                break;
                            case 12: // cleanwin
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, cleanwin));
                                break;
                            case 13: // otherwin
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, otherwin));
                                break;
                            case 14: // wstate
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, wstate));
                                break;
                            case 16: // UA2005 gl
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, gl));
                                break;
                            case 26: // UA2005 strand status
                                if (!hypervisor(dc))
                                    goto priv_insn;
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, ssr));
                                break;
                            default:
                                goto illegal_insn;
                            }
#else
                            tcg_gen_andi_tl(cpu_dst, cpu_dst, ((1 << NWINDOWS) - 1));
                            tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                            tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, wim));
#endif
                        }
                        break;
                    case 0x33: /* wrtbr, UA2005 wrhpr */
                        {
#ifndef TARGET_SPARC64
                            if (!supervisor(dc))
                                goto priv_insn;
                            tcg_gen_xor_tl(cpu_dst, cpu_dst, cpu_src2);
                            tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState, tbr));
#else
                            if (!hypervisor(dc))
                                goto priv_insn;
                            tcg_gen_xor_tl(cpu_dst, cpu_dst, cpu_src2);
                            switch (rd) {
                            case 0: // hpstate
                                // XXX gen_op_wrhpstate();
                                save_state(dc, cpu_cond);
                                gen_op_next_insn();
                                tcg_gen_exit_tb(0);
                                dc->is_br = 1;
                                break;
                            case 1: // htstate
                                // XXX gen_op_wrhtstate();
                                break;
                            case 3: // hintp
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, hintp));
                                break;
                            case 5: // htba
                                tcg_gen_trunc_tl_i32(cpu_tmp32, cpu_dst);
                                tcg_gen_st_i32(cpu_tmp32, cpu_env, offsetof(CPUSPARCState, htba));
                                break;
                            case 31: // hstick_cmpr
                                {
                                    TCGv r_tickptr;

                                    tcg_gen_st_tl(cpu_dst, cpu_env, offsetof(CPUSPARCState,
                                                                 hstick_cmpr));
                                    r_tickptr = tcg_temp_new(TCG_TYPE_PTR);
                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,
                                                   offsetof(CPUState, hstick));
                                    tcg_gen_helper_0_2(helper_tick_set_limit,
                                                       r_tickptr, cpu_dst);
                                }
                                break;
                            case 6: // hver readonly
                            default:
                                goto illegal_insn;
                            }
#endif
                        }
                        break;
#endif
#ifdef TARGET_SPARC64
                    case 0x2c: /* V9 movcc */
                        {
                            int cc = GET_FIELD_SP(insn, 11, 12);
                            int cond = GET_FIELD_SP(insn, 14, 17);
                            TCGv r_cond;
                            int l1;

                            r_cond = tcg_temp_new(TCG_TYPE_TL);
                            if (insn & (1 << 18)) {
                                if (cc == 0)
                                    gen_cond(r_cond, 0, cond);
                                else if (cc == 2)
                                    gen_cond(r_cond, 1, cond);
                                else
                                    goto illegal_insn;
                            } else {
                                gen_fcond(r_cond, cc, cond);
                            }

                            l1 = gen_new_label();

                            tcg_gen_brcond_tl(TCG_COND_EQ, r_cond,
                                              tcg_const_tl(0), l1);
                            if (IS_IMM) {       /* immediate */
                                rs2 = GET_FIELD_SPs(insn, 0, 10);
                                gen_movl_TN_reg(rd, tcg_const_tl((int)rs2));
                            } else {
                                rs2 = GET_FIELD_SP(insn, 0, 4);
                                gen_movl_reg_TN(rs2, cpu_tmp0);
                                gen_movl_TN_reg(rd, cpu_tmp0);
                            }
                            gen_set_label(l1);
                            break;
                        }
                    case 0x2d: /* V9 sdivx */
                        gen_op_sdivx(cpu_dst, cpu_src1, cpu_src2);
                        gen_movl_TN_reg(rd, cpu_dst);
                        break;
                    case 0x2e: /* V9 popc */
                        {
                            cpu_src2 = get_src2(insn, cpu_src2);
                            tcg_gen_helper_1_1(helper_popc, cpu_dst,
                                               cpu_src2);
                            gen_movl_TN_reg(rd, cpu_dst);
                        }
                    case 0x2f: /* V9 movr */
                        {
                            int cond = GET_FIELD_SP(insn, 10, 12);
                            int l1;

                            cpu_src1 = get_src1(insn, cpu_src1);

                            l1 = gen_new_label();

                            tcg_gen_brcond_tl(gen_tcg_cond_reg[cond], cpu_src1,
                                              tcg_const_tl(0), l1);
                            if (IS_IMM) {       /* immediate */
                                rs2 = GET_FIELD_SPs(insn, 0, 9);
                                gen_movl_TN_reg(rd, tcg_const_tl((int)rs2));
                            } else {
                                rs2 = GET_FIELD_SP(insn, 0, 4);
                                gen_movl_reg_TN(rs2, cpu_tmp0);
                                gen_movl_TN_reg(rd, cpu_tmp0);
                            }
                            gen_set_label(l1);
                            break;
                        }
#endif
                    default:
                        goto illegal_insn;
                    }
                }
            } else if (xop == 0x36) { /* UltraSparc shutdown, VIS, V8 CPop1 */
#ifdef TARGET_SPARC64
                int opf = GET_FIELD_SP(insn, 5, 13);
                rs1 = GET_FIELD(insn, 13, 17);
                rs2 = GET_FIELD(insn, 27, 31);
                if (gen_trap_ifnofpu(dc, cpu_cond))
                    goto jmp_insn;

                switch (opf) {
                case 0x000: /* VIS I edge8cc */
                case 0x001: /* VIS II edge8n */
                case 0x002: /* VIS I edge8lcc */
                case 0x003: /* VIS II edge8ln */
                case 0x004: /* VIS I edge16cc */
                case 0x005: /* VIS II edge16n */
                case 0x006: /* VIS I edge16lcc */
                case 0x007: /* VIS II edge16ln */
                case 0x008: /* VIS I edge32cc */
                case 0x009: /* VIS II edge32n */
                case 0x00a: /* VIS I edge32lcc */
                case 0x00b: /* VIS II edge32ln */
                    // XXX
                    goto illegal_insn;
                case 0x010: /* VIS I array8 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    tcg_gen_helper_1_2(helper_array8, cpu_dst, cpu_src1,
                                       cpu_src2);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x012: /* VIS I array16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    tcg_gen_helper_1_2(helper_array8, cpu_dst, cpu_src1,
                                       cpu_src2);
                    tcg_gen_shli_i64(cpu_dst, cpu_dst, 1);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x014: /* VIS I array32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    tcg_gen_helper_1_2(helper_array8, cpu_dst, cpu_src1,
                                       cpu_src2);
                    tcg_gen_shli_i64(cpu_dst, cpu_dst, 2);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x018: /* VIS I alignaddr */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    cpu_src1 = get_src1(insn, cpu_src1);
                    gen_movl_reg_TN(rs2, cpu_src2);
                    tcg_gen_helper_1_2(helper_alignaddr, cpu_dst, cpu_src1,
                                       cpu_src2);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x019: /* VIS II bmask */
                case 0x01a: /* VIS I alignaddrl */
                    // XXX
                    goto illegal_insn;
                case 0x020: /* VIS I fcmple16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmple16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x022: /* VIS I fcmpne16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpne16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x024: /* VIS I fcmple32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmple32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x026: /* VIS I fcmpne32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpne32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x028: /* VIS I fcmpgt16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpgt16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x02a: /* VIS I fcmpeq16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpeq16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x02c: /* VIS I fcmpgt32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpgt32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x02e: /* VIS I fcmpeq32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fcmpeq32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x031: /* VIS I fmul8x16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmul8x16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x033: /* VIS I fmul8x16au */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmul8x16au);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x035: /* VIS I fmul8x16al */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmul8x16al);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x036: /* VIS I fmul8sux16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmul8sux16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x037: /* VIS I fmul8ulx16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmul8ulx16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x038: /* VIS I fmuld8sux16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmuld8sux16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x039: /* VIS I fmuld8ulx16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fmuld8ulx16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x03a: /* VIS I fpack32 */
                case 0x03b: /* VIS I fpack16 */
                case 0x03d: /* VIS I fpackfix */
                case 0x03e: /* VIS I pdist */
                    // XXX
                    goto illegal_insn;
                case 0x048: /* VIS I faligndata */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_faligndata);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x04b: /* VIS I fpmerge */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fpmerge);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x04c: /* VIS II bshuffle */
                    // XXX
                    goto illegal_insn;
                case 0x04d: /* VIS I fexpand */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fexpand);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x050: /* VIS I fpadd16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fpadd16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x051: /* VIS I fpadd16s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fpadd16s);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x052: /* VIS I fpadd32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fpadd32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x053: /* VIS I fpadd32s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fpadd32s);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x054: /* VIS I fpsub16 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fpsub16);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x055: /* VIS I fpsub16s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fpsub16s);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x056: /* VIS I fpsub32 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fpadd32);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x057: /* VIS I fpsub32s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fpsub32s);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x060: /* VIS I fzero */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    tcg_gen_helper_0_0(helper_movl_DT0_0);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x061: /* VIS I fzeros */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    tcg_gen_helper_0_0(helper_movl_FT0_0);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x062: /* VIS I fnor */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fnor);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x063: /* VIS I fnors */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fnors);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x064: /* VIS I fandnot2 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT1(DFPREG(rs1));
                    gen_op_load_fpr_DT0(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fandnot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x065: /* VIS I fandnot2s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT1(rs1);
                    gen_op_load_fpr_FT0(rs2);
                    tcg_gen_helper_0_0(helper_fandnots);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x066: /* VIS I fnot2 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fnot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x067: /* VIS I fnot2s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fnot);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x068: /* VIS I fandnot1 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fandnot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x069: /* VIS I fandnot1s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fandnots);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x06a: /* VIS I fnot1 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT1(DFPREG(rs1));
                    tcg_gen_helper_0_0(helper_fnot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x06b: /* VIS I fnot1s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT1(rs1);
                    tcg_gen_helper_0_0(helper_fnot);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x06c: /* VIS I fxor */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fxor);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x06d: /* VIS I fxors */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fxors);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x06e: /* VIS I fnand */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fnand);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x06f: /* VIS I fnands */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fnands);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x070: /* VIS I fand */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fand);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x071: /* VIS I fands */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fands);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x072: /* VIS I fxnor */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fxnor);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x073: /* VIS I fxnors */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fxnors);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x074: /* VIS I fsrc1 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x075: /* VIS I fsrc1s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x076: /* VIS I fornot2 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT1(DFPREG(rs1));
                    gen_op_load_fpr_DT0(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fornot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x077: /* VIS I fornot2s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT1(rs1);
                    gen_op_load_fpr_FT0(rs2);
                    tcg_gen_helper_0_0(helper_fornots);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x078: /* VIS I fsrc2 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs2));
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x079: /* VIS I fsrc2s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs2);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x07a: /* VIS I fornot1 */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_fornot);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x07b: /* VIS I fornot1s */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fornots);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x07c: /* VIS I for */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_DT0(DFPREG(rs1));
                    gen_op_load_fpr_DT1(DFPREG(rs2));
                    tcg_gen_helper_0_0(helper_for);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x07d: /* VIS I fors */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    gen_op_load_fpr_FT0(rs1);
                    gen_op_load_fpr_FT1(rs2);
                    tcg_gen_helper_0_0(helper_fors);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x07e: /* VIS I fone */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    tcg_gen_helper_0_0(helper_movl_DT0_1);
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                case 0x07f: /* VIS I fones */
                    CHECK_FPU_FEATURE(dc, VIS1);
                    tcg_gen_helper_0_0(helper_movl_FT0_1);
                    gen_op_store_FT0_fpr(rd);
                    break;
                case 0x080: /* VIS I shutdown */
                case 0x081: /* VIS II siam */
                    // XXX
                    goto illegal_insn;
                default:
                    goto illegal_insn;
                }
#else
                goto ncp_insn;
#endif
            } else if (xop == 0x37) { /* V8 CPop2, V9 impdep2 */
#ifdef TARGET_SPARC64
                goto illegal_insn;
#else
                goto ncp_insn;
#endif
#ifdef TARGET_SPARC64
            } else if (xop == 0x39) { /* V9 return */
                save_state(dc, cpu_cond);
                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {   /* immediate */
                    rs2 = GET_FIELDs(insn, 19, 31);
                    tcg_gen_addi_tl(cpu_dst, cpu_src1, (int)rs2);
                } else {                /* register */
                    rs2 = GET_FIELD(insn, 27, 31);
                    if (rs2) {
                        gen_movl_reg_TN(rs2, cpu_src2);
                        tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);
                    } else
                        tcg_gen_mov_tl(cpu_dst, cpu_src1);
                }
                tcg_gen_helper_0_0(helper_restore);
                gen_mov_pc_npc(dc, cpu_cond);
                tcg_gen_helper_0_2(helper_check_align, cpu_dst, tcg_const_i32(3));
                tcg_gen_mov_tl(cpu_npc, cpu_dst);
                dc->npc = DYNAMIC_PC;
                goto jmp_insn;
#endif
            } else {
                cpu_src1 = get_src1(insn, cpu_src1);
                if (IS_IMM) {   /* immediate */
                    rs2 = GET_FIELDs(insn, 19, 31);
                    tcg_gen_addi_tl(cpu_dst, cpu_src1, (int)rs2);
                } else {                /* register */
                    rs2 = GET_FIELD(insn, 27, 31);
                    if (rs2) {
                        gen_movl_reg_TN(rs2, cpu_src2);
                        tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);
                    } else
                        tcg_gen_mov_tl(cpu_dst, cpu_src1);
                }
                switch (xop) {
                case 0x38:      /* jmpl */
                    {
                        gen_movl_TN_reg(rd, tcg_const_tl(dc->pc));
                        gen_mov_pc_npc(dc, cpu_cond);
                        tcg_gen_helper_0_2(helper_check_align, cpu_dst, tcg_const_i32(3));
                        tcg_gen_mov_tl(cpu_npc, cpu_dst);
                        dc->npc = DYNAMIC_PC;
                    }
                    goto jmp_insn;
#if !defined(CONFIG_USER_ONLY) && !defined(TARGET_SPARC64)
                case 0x39:      /* rett, V9 return */
                    {
                        if (!supervisor(dc))
                            goto priv_insn;
                        gen_mov_pc_npc(dc, cpu_cond);
                        tcg_gen_helper_0_2(helper_check_align, cpu_dst, tcg_const_i32(3));
                        tcg_gen_mov_tl(cpu_npc, cpu_dst);
                        dc->npc = DYNAMIC_PC;
                        tcg_gen_helper_0_0(helper_rett);
                    }
                    goto jmp_insn;
#endif
                case 0x3b: /* flush */
                    if (!((dc)->features & CPU_FEATURE_FLUSH))
                        goto unimp_flush;
                    tcg_gen_helper_0_1(helper_flush, cpu_dst);
                    break;
                case 0x3c:      /* save */
                    save_state(dc, cpu_cond);
                    tcg_gen_helper_0_0(helper_save);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
                case 0x3d:      /* restore */
                    save_state(dc, cpu_cond);
                    tcg_gen_helper_0_0(helper_restore);
                    gen_movl_TN_reg(rd, cpu_dst);
                    break;
#if !defined(CONFIG_USER_ONLY) && defined(TARGET_SPARC64)
                case 0x3e:      /* V9 done/retry */
                    {
                        switch (rd) {
                        case 0:
                            if (!supervisor(dc))
                                goto priv_insn;
                            dc->npc = DYNAMIC_PC;
                            dc->pc = DYNAMIC_PC;
                            tcg_gen_helper_0_0(helper_done);
                            goto jmp_insn;
                        case 1:
                            if (!supervisor(dc))
                                goto priv_insn;
                            dc->npc = DYNAMIC_PC;
                            dc->pc = DYNAMIC_PC;
                            tcg_gen_helper_0_0(helper_retry);
                            goto jmp_insn;
                        default:
                            goto illegal_insn;
                        }
                    }
                    break;
#endif
                default:
                    goto illegal_insn;
                }
            }
            break;
        }
        break;
    case 3:                     /* load/store instructions */
        {
            unsigned int xop = GET_FIELD(insn, 7, 12);

            save_state(dc, cpu_cond);
            cpu_src1 = get_src1(insn, cpu_src1);
            if (xop == 0x3c || xop == 0x3e)
            {
                rs2 = GET_FIELD(insn, 27, 31);
                gen_movl_reg_TN(rs2, cpu_src2);
            }
            else if (IS_IMM) {       /* immediate */
                rs2 = GET_FIELDs(insn, 19, 31);
                tcg_gen_addi_tl(cpu_addr, cpu_src1, (int)rs2);
            } else {            /* register */
                rs2 = GET_FIELD(insn, 27, 31);
                if (rs2 != 0) {
                    gen_movl_reg_TN(rs2, cpu_src2);
                    tcg_gen_add_tl(cpu_addr, cpu_src1, cpu_src2);
                } else
                    tcg_gen_mov_tl(cpu_addr, cpu_src1);
            }
            if (xop < 4 || (xop > 7 && xop < 0x14 && xop != 0x0e) ||
                (xop > 0x17 && xop <= 0x1d ) ||
                (xop > 0x2c && xop <= 0x33) || xop == 0x1f || xop == 0x3d) {
                switch (xop) {
                case 0x0:       /* load unsigned word */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld32u(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x1:       /* load unsigned byte */
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld8u(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x2:       /* load unsigned halfword */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld16u(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x3:       /* load double word */
                    if (rd & 1)
                        goto illegal_insn;
                    else {
                        tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                        ABI32_MASK(cpu_addr);
                        tcg_gen_qemu_ld64(cpu_tmp64, cpu_addr, dc->mem_idx);
                        tcg_gen_trunc_i64_tl(cpu_tmp0, cpu_tmp64);
                        tcg_gen_andi_tl(cpu_tmp0, cpu_tmp0, 0xffffffffULL);
                        gen_movl_TN_reg(rd + 1, cpu_tmp0);
                        tcg_gen_shri_i64(cpu_tmp64, cpu_tmp64, 32);
                        tcg_gen_trunc_i64_tl(cpu_val, cpu_tmp64);
                        tcg_gen_andi_tl(cpu_val, cpu_val, 0xffffffffULL);
                    }
                    break;
                case 0x9:       /* load signed byte */
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld8s(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0xa:       /* load signed halfword */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld16s(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0xd:       /* ldstub -- XXX: should be atomically */
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld8s(cpu_val, cpu_addr, dc->mem_idx);
                    tcg_gen_qemu_st8(tcg_const_tl(0xff), cpu_addr, dc->mem_idx);
                    break;
                case 0x0f:      /* swap register with memory. Also atomically */
                    CHECK_IU_FEATURE(dc, SWAP);
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_movl_reg_TN(rd, cpu_val);
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld32u(cpu_tmp32, cpu_addr, dc->mem_idx);
                    tcg_gen_qemu_st32(cpu_val, cpu_addr, dc->mem_idx);
                    tcg_gen_extu_i32_tl(cpu_val, cpu_tmp32);
                    break;
#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)
                case 0x10:      /* load word alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_ld_asi(cpu_val, cpu_addr, insn, 4, 0);
                    break;
                case 0x11:      /* load unsigned byte alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    gen_ld_asi(cpu_val, cpu_addr, insn, 1, 0);
                    break;
                case 0x12:      /* load unsigned halfword alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    gen_ld_asi(cpu_val, cpu_addr, insn, 2, 0);
                    break;
                case 0x13:      /* load double word alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    if (rd & 1)
                        goto illegal_insn;
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    gen_ldda_asi(cpu_tmp0, cpu_val, cpu_addr, insn);
                    gen_movl_TN_reg(rd + 1, cpu_tmp0);
                    break;
                case 0x19:      /* load signed byte alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    gen_ld_asi(cpu_val, cpu_addr, insn, 1, 1);
                    break;
                case 0x1a:      /* load signed halfword alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    gen_ld_asi(cpu_val, cpu_addr, insn, 2, 1);
                    break;
                case 0x1d:      /* ldstuba -- XXX: should be atomically */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    gen_ldstub_asi(cpu_val, cpu_addr, insn);
                    break;
                case 0x1f:      /* swap reg with alt. memory. Also atomically */
                    CHECK_IU_FEATURE(dc, SWAP);
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_movl_reg_TN(rd, cpu_val);
                    gen_swap_asi(cpu_val, cpu_addr, insn);
                    break;

#ifndef TARGET_SPARC64
                case 0x30: /* ldc */
                case 0x31: /* ldcsr */
                case 0x33: /* lddc */
                    goto ncp_insn;
#endif
#endif
#ifdef TARGET_SPARC64
                case 0x08: /* V9 ldsw */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld32s(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x0b: /* V9 ldx */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_ld64(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x18: /* V9 ldswa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_ld_asi(cpu_val, cpu_addr, insn, 4, 1);
                    break;
                case 0x1b: /* V9 ldxa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    gen_ld_asi(cpu_val, cpu_addr, insn, 8, 0);
                    break;
                case 0x2d: /* V9 prefetch, no effect */
                    goto skip_move;
                case 0x30: /* V9 ldfa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_ldf_asi(cpu_addr, insn, 4, rd);
                    goto skip_move;
                case 0x33: /* V9 lddfa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_ldf_asi(cpu_addr, insn, 8, DFPREG(rd));
                    goto skip_move;
                case 0x3d: /* V9 prefetcha, no effect */
                    goto skip_move;
                case 0x32: /* V9 ldqfa */
                    CHECK_FPU_FEATURE(dc, FLOAT128);
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_ldf_asi(cpu_addr, insn, 16, QFPREG(rd));
                    goto skip_move;
#endif
                default:
                    goto illegal_insn;
                }
                gen_movl_TN_reg(rd, cpu_val);
#ifdef TARGET_SPARC64
            skip_move: ;
#endif
            } else if (xop >= 0x20 && xop < 0x24) {
                if (gen_trap_ifnofpu(dc, cpu_cond))
                    goto jmp_insn;
                switch (xop) {
                case 0x20:      /* load fpreg */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    tcg_gen_qemu_ld32u(cpu_tmp32, cpu_addr, dc->mem_idx);
                    tcg_gen_st_i32(cpu_tmp32, cpu_env,
                                   offsetof(CPUState, fpr[rd]));
                    break;
                case 0x21:      /* load fsr */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    tcg_gen_qemu_ld32u(cpu_tmp32, cpu_addr, dc->mem_idx);
                    tcg_gen_st_i32(cpu_tmp32, cpu_env,
                                   offsetof(CPUState, ft0));
                    tcg_gen_helper_0_0(helper_ldfsr);
                    break;
                case 0x22:      /* load quad fpreg */
                    CHECK_FPU_FEATURE(dc, FLOAT128);
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr,
                                       tcg_const_i32(7));
                    tcg_gen_helper_0_2(helper_ldqf, cpu_addr, dc->mem_idx);
                    gen_op_store_QT0_fpr(QFPREG(rd));
                    break;
                case 0x23:      /* load double fpreg */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr,
                                       tcg_const_i32(7));
                    tcg_gen_helper_0_2(helper_lddf, cpu_addr,
                                       tcg_const_i32(dc->mem_idx));
                    gen_op_store_DT0_fpr(DFPREG(rd));
                    break;
                default:
                    goto illegal_insn;
                }
            } else if (xop < 8 || (xop >= 0x14 && xop < 0x18) || \
                       xop == 0xe || xop == 0x1e) {
                gen_movl_reg_TN(rd, cpu_val);
                switch (xop) {
                case 0x4: /* store word */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_st32(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x5: /* store byte */
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_st8(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x6: /* store halfword */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_st16(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x7: /* store double word */
                    if (rd & 1)
                        goto illegal_insn;
                    else {
                        TCGv r_low;

                        tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                        r_low = tcg_temp_new(TCG_TYPE_I32);
                        gen_movl_reg_TN(rd + 1, r_low);
                        tcg_gen_helper_1_2(helper_pack64, cpu_tmp64, cpu_val,
                                           r_low);
                        tcg_gen_qemu_st64(cpu_tmp64, cpu_addr, dc->mem_idx);
                    }
                    break;
#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)
                case 0x14: /* store word alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_st_asi(cpu_val, cpu_addr, insn, 4);
                    break;
                case 0x15: /* store byte alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    gen_st_asi(cpu_val, cpu_addr, insn, 1);
                    break;
                case 0x16: /* store halfword alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(1));
                    gen_st_asi(cpu_val, cpu_addr, insn, 2);
                    break;
                case 0x17: /* store double word alternate */
#ifndef TARGET_SPARC64
                    if (IS_IMM)
                        goto illegal_insn;
                    if (!supervisor(dc))
                        goto priv_insn;
#endif
                    if (rd & 1)
                        goto illegal_insn;
                    else {
                        tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                        gen_stda_asi(cpu_val, cpu_addr, insn, rd);
                    }
                    break;
#endif
#ifdef TARGET_SPARC64
                case 0x0e: /* V9 stx */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    ABI32_MASK(cpu_addr);
                    tcg_gen_qemu_st64(cpu_val, cpu_addr, dc->mem_idx);
                    break;
                case 0x1e: /* V9 stxa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    gen_st_asi(cpu_val, cpu_addr, insn, 8);
                    break;
#endif
                default:
                    goto illegal_insn;
                }
            } else if (xop > 0x23 && xop < 0x28) {
                if (gen_trap_ifnofpu(dc, cpu_cond))
                    goto jmp_insn;
                switch (xop) {
                case 0x24: /* store fpreg */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env,
                                   offsetof(CPUState, fpr[rd]));
                    tcg_gen_qemu_st32(cpu_tmp32, cpu_addr, dc->mem_idx);
                    break;
                case 0x25: /* stfsr, V9 stxfsr */
#ifdef CONFIG_USER_ONLY
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
#endif
                    tcg_gen_helper_0_0(helper_stfsr);
                    tcg_gen_ld_i32(cpu_tmp32, cpu_env,
                                   offsetof(CPUState, ft0));
                    tcg_gen_qemu_st32(cpu_tmp32, cpu_addr, dc->mem_idx);
                    break;
                case 0x26:
#ifdef TARGET_SPARC64
                    /* V9 stqf, store quad fpreg */
                    CHECK_FPU_FEATURE(dc, FLOAT128);
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr,
                                       tcg_const_i32(7));
                    gen_op_load_fpr_QT0(QFPREG(rd));
                    tcg_gen_helper_0_2(helper_stqf, cpu_addr, dc->mem_idx);
                    break;
#else /* !TARGET_SPARC64 */
                    /* stdfq, store floating point queue */
#if defined(CONFIG_USER_ONLY)
                    goto illegal_insn;
#else
                    if (!supervisor(dc))
                        goto priv_insn;
                    if (gen_trap_ifnofpu(dc, cpu_cond))
                        goto jmp_insn;
                    goto nfq_insn;
#endif
#endif
                case 0x27: /* store double fpreg */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr,
                                       tcg_const_i32(7));
                    gen_op_load_fpr_DT0(DFPREG(rd));
                    tcg_gen_helper_0_2(helper_stdf, cpu_addr,
                                       tcg_const_i32(dc->mem_idx));
                    break;
                default:
                    goto illegal_insn;
                }
            } else if (xop > 0x33 && xop < 0x3f) {
                switch (xop) {
#ifdef TARGET_SPARC64
                case 0x34: /* V9 stfa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_op_load_fpr_FT0(rd);
                    gen_stf_asi(cpu_addr, insn, 4, rd);
                    break;
                case 0x36: /* V9 stqfa */
                    CHECK_FPU_FEATURE(dc, FLOAT128);
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr,
                                       tcg_const_i32(7));
                    gen_op_load_fpr_QT0(QFPREG(rd));
                    gen_stf_asi(cpu_addr, insn, 16, QFPREG(rd));
                    break;
                case 0x37: /* V9 stdfa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_op_load_fpr_DT0(DFPREG(rd));
                    gen_stf_asi(cpu_addr, insn, 8, DFPREG(rd));
                    break;
                case 0x3c: /* V9 casa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(3));
                    gen_cas_asi(cpu_val, cpu_addr, cpu_val, insn, rd);
                    gen_movl_TN_reg(rd, cpu_val);
                    break;
                case 0x3e: /* V9 casxa */
                    tcg_gen_helper_0_2(helper_check_align, cpu_addr, tcg_const_i32(7));
                    gen_casx_asi(cpu_val, cpu_addr, cpu_val, insn, rd);
                    gen_movl_TN_reg(rd, cpu_val);
                    break;
#else
                case 0x34: /* stc */
                case 0x35: /* stcsr */
                case 0x36: /* stdcq */
                case 0x37: /* stdc */
                    goto ncp_insn;
#endif
                default:
                    goto illegal_insn;
                }
            }
            else
                goto illegal_insn;
        }
        break;
    }
    /* default case for non jump instructions */
    if (dc->npc == DYNAMIC_PC) {
        dc->pc = DYNAMIC_PC;
        gen_op_next_insn();
    } else if (dc->npc == JUMP_PC) {
        /* we can do a static jump */
        gen_branch2(dc, dc->jump_pc[0], dc->jump_pc[1], cpu_cond);
        dc->is_br = 1;
    } else {
        dc->pc = dc->npc;
        dc->npc = dc->npc + 4;
    }
 jmp_insn:
    return;
 illegal_insn:
    save_state(dc, cpu_cond);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_ILL_INSN));
    dc->is_br = 1;
    return;
 unimp_flush:
    save_state(dc, cpu_cond);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_UNIMP_FLUSH));
    dc->is_br = 1;
    return;
#if !defined(CONFIG_USER_ONLY)
 priv_insn:
    save_state(dc, cpu_cond);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_PRIV_INSN));
    dc->is_br = 1;
    return;
#endif
 nfpu_insn:
    save_state(dc, cpu_cond);
    gen_op_fpexception_im(FSR_FTT_UNIMPFPOP);
    dc->is_br = 1;
    return;
#if !defined(CONFIG_USER_ONLY) && !defined(TARGET_SPARC64)
 nfq_insn:
    save_state(dc, cpu_cond);
    gen_op_fpexception_im(FSR_FTT_SEQ_ERROR);
    dc->is_br = 1;
    return;
#endif
#ifndef TARGET_SPARC64
 ncp_insn:
    save_state(dc, cpu_cond);
    tcg_gen_helper_0_1(raise_exception, tcg_const_i32(TT_NCP_INSN));
    dc->is_br = 1;
    return;
#endif
}

static void tcg_macro_func(TCGContext *s, int macro_id, const int *dead_args)
{
}

static inline int gen_intermediate_code_internal(TranslationBlock * tb,
                                                 int spc, CPUSPARCState *env)
{
    target_ulong pc_start, last_pc;
    uint16_t *gen_opc_end;
    DisasContext dc1, *dc = &dc1;
    int j, lj = -1;

    memset(dc, 0, sizeof(DisasContext));
    dc->tb = tb;
    pc_start = tb->pc;
    dc->pc = pc_start;
    last_pc = dc->pc;
    dc->npc = (target_ulong) tb->cs_base;
    dc->mem_idx = cpu_mmu_index(env);
    dc->features = env->features;
    if ((dc->features & CPU_FEATURE_FLOAT)) {
        dc->fpu_enabled = cpu_fpu_enabled(env);
#if defined(CONFIG_USER_ONLY)
        dc->features |= CPU_FEATURE_FLOAT128;
#endif
    } else
        dc->fpu_enabled = 0;
    gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;

    cpu_tmp0 = tcg_temp_new(TCG_TYPE_TL);
    cpu_tmp32 = tcg_temp_new(TCG_TYPE_I32);
    cpu_tmp64 = tcg_temp_new(TCG_TYPE_I64);

    do {
        if (env->nb_breakpoints > 0) {
            for(j = 0; j < env->nb_breakpoints; j++) {
                if (env->breakpoints[j] == dc->pc) {
                    if (dc->pc != pc_start)
                        save_state(dc, cpu_cond);
                    tcg_gen_helper_0_0(helper_debug);
                    tcg_gen_exit_tb(0);
                    dc->is_br = 1;
                    goto exit_gen_loop;
                }
            }
        }
        if (spc) {
            if (loglevel > 0)
                fprintf(logfile, "Search PC...\n");
            j = gen_opc_ptr - gen_opc_buf;
            if (lj < j) {
                lj++;
                while (lj < j)
                    gen_opc_instr_start[lj++] = 0;
                gen_opc_pc[lj] = dc->pc;
                gen_opc_npc[lj] = dc->npc;
                gen_opc_instr_start[lj] = 1;
            }
        }
        last_pc = dc->pc;
        disas_sparc_insn(dc);

        if (dc->is_br)
            break;
        /* if the next PC is different, we abort now */
        if (dc->pc != (last_pc + 4))
            break;
        /* if we reach a page boundary, we stop generation so that the
           PC of a TT_TFAULT exception is always in the right page */
        if ((dc->pc & (TARGET_PAGE_SIZE - 1)) == 0)
            break;
        /* if single step mode, we generate only one instruction and
           generate an exception */
        if (env->singlestep_enabled) {
            tcg_gen_movi_tl(cpu_pc, dc->pc);
            tcg_gen_exit_tb(0);
            break;
        }
    } while ((gen_opc_ptr < gen_opc_end) &&
             (dc->pc - pc_start) < (TARGET_PAGE_SIZE - 32));

 exit_gen_loop:
    if (!dc->is_br) {
        if (dc->pc != DYNAMIC_PC &&
            (dc->npc != DYNAMIC_PC && dc->npc != JUMP_PC)) {
            /* static PC and NPC: we can use direct chaining */
            gen_goto_tb(dc, 0, dc->pc, dc->npc);
        } else {
            if (dc->pc != DYNAMIC_PC)
                tcg_gen_movi_tl(cpu_pc, dc->pc);
            save_npc(dc, cpu_cond);
            tcg_gen_exit_tb(0);
        }
    }
    *gen_opc_ptr = INDEX_op_end;
    if (spc) {
        j = gen_opc_ptr - gen_opc_buf;
        lj++;
        while (lj <= j)
            gen_opc_instr_start[lj++] = 0;
#if 0
        if (loglevel > 0) {
            page_dump(logfile);
        }
#endif
        gen_opc_jump_pc[0] = dc->jump_pc[0];
        gen_opc_jump_pc[1] = dc->jump_pc[1];
    } else {
        tb->size = last_pc + 4 - pc_start;
    }
#ifdef DEBUG_DISAS
    if (loglevel & CPU_LOG_TB_IN_ASM) {
        fprintf(logfile, "--------------\n");
        fprintf(logfile, "IN: %s\n", lookup_symbol(pc_start));
        target_disas(logfile, pc_start, last_pc + 4 - pc_start, 0);
        fprintf(logfile, "\n");
    }
#endif
    return 0;
}

int gen_intermediate_code(CPUSPARCState * env, TranslationBlock * tb)
{
    return gen_intermediate_code_internal(tb, 0, env);
}

int gen_intermediate_code_pc(CPUSPARCState * env, TranslationBlock * tb)
{
    return gen_intermediate_code_internal(tb, 1, env);
}

void gen_intermediate_code_init(CPUSPARCState *env)
{
    unsigned int i;
    static int inited;
    static const char * const gregnames[8] = {
        NULL, // g0 not used
        "g1",
        "g2",
        "g3",
        "g4",
        "g5",
        "g6",
        "g7",
    };

    /* init various static tables */
    if (!inited) {
        inited = 1;

        tcg_set_macro_func(&tcg_ctx, tcg_macro_func);
        cpu_env = tcg_global_reg_new(TCG_TYPE_PTR, TCG_AREG0, "env");
        cpu_regwptr = tcg_global_mem_new(TCG_TYPE_PTR, TCG_AREG0,
                                         offsetof(CPUState, regwptr),
                                         "regwptr");
#ifdef TARGET_SPARC64
        cpu_xcc = tcg_global_mem_new(TCG_TYPE_I32,
                                     TCG_AREG0, offsetof(CPUState, xcc),
                                     "xcc");
#endif
        /* XXX: T0 and T1 should be temporaries */
        cpu_T[0] = tcg_global_mem_new(TCG_TYPE_TL,
                                      TCG_AREG0, offsetof(CPUState, t0), "T0");
        cpu_T[1] = tcg_global_mem_new(TCG_TYPE_TL,
                                      TCG_AREG0, offsetof(CPUState, t1), "T1");
        cpu_cond = tcg_global_mem_new(TCG_TYPE_TL,
                                      TCG_AREG0, offsetof(CPUState, cond), "cond");
        cpu_cc_src = tcg_global_mem_new(TCG_TYPE_TL,
                                        TCG_AREG0, offsetof(CPUState, cc_src),
                                        "cc_src");
        cpu_cc_src2 = tcg_global_mem_new(TCG_TYPE_TL, TCG_AREG0,
                                         offsetof(CPUState, cc_src2),
                                         "cc_src2");
        cpu_cc_dst = tcg_global_mem_new(TCG_TYPE_TL,
                                        TCG_AREG0, offsetof(CPUState, cc_dst),
                                        "cc_dst");
        cpu_psr = tcg_global_mem_new(TCG_TYPE_I32,
                                     TCG_AREG0, offsetof(CPUState, psr),
                                     "psr");
        cpu_fsr = tcg_global_mem_new(TCG_TYPE_TL,
                                     TCG_AREG0, offsetof(CPUState, fsr),
                                     "fsr");
        cpu_pc = tcg_global_mem_new(TCG_TYPE_TL,
                                    TCG_AREG0, offsetof(CPUState, pc),
                                    "pc");
        cpu_npc = tcg_global_mem_new(TCG_TYPE_TL,
                                    TCG_AREG0, offsetof(CPUState, npc),
                                    "npc");
        for (i = 1; i < 8; i++)
            cpu_gregs[i] = tcg_global_mem_new(TCG_TYPE_TL, TCG_AREG0,
                                              offsetof(CPUState, gregs[i]),
                                              gregnames[i]);
    }
}

void gen_pc_load(CPUState *env, TranslationBlock *tb,
                unsigned long searched_pc, int pc_pos, void *puc)
{
    target_ulong npc;
    env->pc = gen_opc_pc[pc_pos];
    npc = gen_opc_npc[pc_pos];
    if (npc == 1) {
        /* dynamic NPC: already stored */
    } else if (npc == 2) {
        target_ulong t2 = (target_ulong)(unsigned long)puc;
        /* jump PC: use T2 and the jump targets of the translation */
        if (t2)
            env->npc = gen_opc_jump_pc[0];
        else
            env->npc = gen_opc_jump_pc[1];
    } else {
        env->npc = npc;
    }
}
