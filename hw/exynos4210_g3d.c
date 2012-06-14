/*
 * Samsung exynos4210 MALI400 gpu (G3D) emulation
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd.
 * All rights reserved.
 *
 * Mitsyanko Igor <i.mitsyanko@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu-common.h"
#include "sysbus.h"
#ifdef CONFIG_BUILD_GLES
#include "gles2.h"
#endif

/* Debug messages configuration */
#define EXY_G3D_DEBUG               0

#if EXY_G3D_DEBUG == 0
    #define DPRINT_L1(fmt, args...)       do { } while (0)
    #define DPRINT_L2(fmt, args...)       do { } while (0)
    #define DPRINT_ERROR(fmt, args...)    do { } while (0)
#elif EXY_G3D_DEBUG == 1
    #define DPRINT_L1(fmt, args...) \
    do {fprintf(stderr, "QEMU G3D: "fmt, ## args); } while (0)
    #define DPRINT_L2(fmt, args...)       do { } while (0)
    #define DPRINT_ERROR(fmt, args...)  \
    do {fprintf(stderr, "QEMU G3D ERROR: "fmt, ## args); } while (0)
#else
    #define DPRINT_L1(fmt, args...) \
    do {fprintf(stderr, "QEMU G3D: "fmt, ## args); } while (0)
    #define DPRINT_L2(fmt, args...) \
    do {fprintf(stderr, "QEMU G3D: "fmt, ## args); } while (0)
    #define DPRINT_ERROR(fmt, args...)  \
    do {fprintf(stderr, "QEMU G3D ERROR: "fmt, ## args); } while (0)
#endif

#define NUM_OF_PIXPROC                                  4
#define EXYNOS4210_G3D_REG_MEM_SIZE                     0x10000
#define MALIGP_REGS_SIZE                                0x98
#define MALIPP_REGS_SIZE                                0x10f0
#define MALIMMU_REGS_SIZE                               0x24
#define MALI_L2CACHE_REGS_SIZE                          0x30

#define GP_OFF_START                                    0x0000
#define GP_OFF_END                       (GP_OFF_START + MALIGP_REGS_SIZE)
#define L2_OFF_START                                    0x1000
#define L2_OFF_END                       (L2_OFF_START + MALI_L2CACHE_REGS_SIZE)
#define PMU_OFF_START                                   0x2000
#define PMU_OFF_END                                     0x2FFC
#define GP_MMU_OFF_START                                0x3000
#define GP_MMU_OFF_END                   (GP_MMU_OFF_START + MALIMMU_REGS_SIZE)
#define PP0_MMU_OFF_START                               0x4000
#define PP0_MMU_OFF_END                  (PP0_MMU_OFF_START + MALIMMU_REGS_SIZE)
#define PP1_MMU_OFF_START                               0x5000
#define PP1_MMU_OFF_END                  (PP1_MMU_OFF_START + MALIMMU_REGS_SIZE)
#define PP2_MMU_OFF_START                               0x6000
#define PP2_MMU_OFF_END                  (PP2_MMU_OFF_START + MALIMMU_REGS_SIZE)
#define PP3_MMU_OFF_START                               0x7000
#define PP3_MMU_OFF_END                  (PP3_MMU_OFF_START + MALIMMU_REGS_SIZE)
#define PP0_OFF_START                                   0x8000
#define PP0_OFF_END                      (PP0_OFF_START + MALIPP_REGS_SIZE)
#define PP1_OFF_START                                   0xA000
#define PP1_OFF_END                      (PP1_OFF_START + MALIPP_REGS_SIZE)
#define PP2_OFF_START                                   0xC000
#define PP2_OFF_END                      (PP2_OFF_START + MALIPP_REGS_SIZE)
#define PP3_OFF_START                                   0xE000
#define PP3_OFF_END                      (PP3_OFF_START + MALIPP_REGS_SIZE)

/**************************************************
 * MALI geometry processor register defines
 **************************************************/

#define MALIGP_REG_OFF_VSCL_START_ADDR                  0x00
#define MALIGP_REG_OFF_VSCL_END_ADDR                    0x04
#define MALIGP_REG_OFF_PLBUCL_START_ADDR                0x08
#define MALIGP_REG_OFF_PLBUCL_END_ADDR                  0x0c
#define MALIGP_REG_OFF_PLBU_ALLOC_START_ADDR            0x10
#define MALIGP_REG_OFF_PLBU_ALLOC_END_ADDR              0x14

/* Command to geometry processor */
#define MALIGP_REG_OFF_CMD                              0x20
#define MALIGP_REG_VAL_CMD_START_VS                     (1 << 0)
#define MALIGP_REG_VAL_CMD_START_PLBU                   (1 << 1)
#define MALIGP_REG_VAL_CMD_UPDATE_PLBU_ALLOC            (1 << 4)
#define MALIGP_REG_VAL_CMD_RESET                        (1 << 5)
#define MALIGP_REG_VAL_CMD_FORCE_HANG                   (1 << 6)
#define MALIGP_REG_VAL_CMD_STOP_BUS                     (1 << 9)
#define MALIGP_REG_VAL_CMD_SOFT_RESET                   (1 << 10)

/* Raw interrupt ststus - can not be masked by INT_MASK */
#define MALIGP_REG_OFF_INT_RAWSTAT                      0x24
/* WO register - write 1 to clear irq flag in INT_RAWSTAT */
#define MALIGP_REG_OFF_INT_CLEAR                        0x28
/* MASK interrupt requests in IRQ_STATUS */
#define MALIGP_REG_OFF_INT_MASK                         0x2C
/* Bits are set only if not masked and if bit in RAWSTAT is set */
#define MALIGP_REG_OFF_INT_STAT                         0x30
/* Interrupt statuses of geometry processor */
#define MALIGP_REG_VAL_IRQ_VS_END_CMD_LST               (1 << 0)
#define MALIGP_REG_VAL_IRQ_PLBU_END_CMD_LST             (1 << 1)
#define MALIGP_REG_VAL_IRQ_PLBU_OUT_OF_MEM              (1 << 2)
#define MALIGP_REG_VAL_IRQ_VS_SEM_IRQ                   (1 << 3)
#define MALIGP_REG_VAL_IRQ_PLBU_SEM_IRQ                 (1 << 4)
#define MALIGP_REG_VAL_IRQ_HANG                         (1 << 5)
#define MALIGP_REG_VAL_IRQ_FORCE_HANG                   (1 << 6)
#define MALIGP_REG_VAL_IRQ_PERF_CNT_0_LIMIT             (1 << 7)
#define MALIGP_REG_VAL_IRQ_PERF_CNT_1_LIMIT             (1 << 8)
#define MALIGP_REG_VAL_IRQ_WRITE_BOUND_ERR              (1 << 9)
#define MALIGP_REG_VAL_IRQ_SYNC_ERROR                   (1 << 10)
#define MALIGP_REG_VAL_IRQ_AXI_BUS_ERROR                (1 << 11)
#define MALIGP_REG_VAL_IRQ_AXI_BUS_STOPPED              (1 << 12)
#define MALIGP_REG_VAL_IRQ_VS_INVALID_CMD               (1 << 13)
#define MALIGP_REG_VAL_IRQ_PLB_INVALID_CMD              (1 << 14)
#define MALIGP_REG_VAL_IRQ_RESET_COMPLETED              (1 << 19)
#define MALIGP_REG_VAL_IRQ_SEMAPHORE_UNDERFLOW          (1 << 20)
#define MALIGP_REG_VAL_IRQ_SEMAPHORE_OVERFLOW           (1 << 21)
#define MALIGP_REG_VAL_IRQ_PTR_ARRAY_OUT_OF_BOUNDS      (1 << 22)
#define MALIGP_REG_VAL_IRQ_MASK_ALL \
	(\
		MALIGP_REG_VAL_IRQ_VS_END_CMD_LST      | \
		MALIGP_REG_VAL_IRQ_PLBU_END_CMD_LST    | \
		MALIGP_REG_VAL_IRQ_PLBU_OUT_OF_MEM     | \
		MALIGP_REG_VAL_IRQ_VS_SEM_IRQ          | \
		MALIGP_REG_VAL_IRQ_PLBU_SEM_IRQ        | \
		MALIGP_REG_VAL_IRQ_HANG                | \
		MALIGP_REG_VAL_IRQ_FORCE_HANG          | \
		MALIGP_REG_VAL_IRQ_PERF_CNT_0_LIMIT    | \
		MALIGP_REG_VAL_IRQ_PERF_CNT_1_LIMIT    | \
		MALIGP_REG_VAL_IRQ_WRITE_BOUND_ERR     | \
		MALIGP_REG_VAL_IRQ_SYNC_ERROR          | \
		MALIGP_REG_VAL_IRQ_AXI_BUS_ERROR       | \
		MALIGP_REG_VAL_IRQ_AXI_BUS_STOPPED     | \
		MALIGP_REG_VAL_IRQ_VS_INVALID_CMD      | \
		MALIGP_REG_VAL_IRQ_PLB_INVALID_CMD     | \
		MALIGP_REG_VAL_IRQ_RESET_COMPLETED     | \
		MALIGP_REG_VAL_IRQ_SEMAPHORE_UNDERFLOW | \
		MALIGP_REG_VAL_IRQ_SEMAPHORE_OVERFLOW  | \
		MALIGP_REG_VAL_IRQ_PTR_ARRAY_OUT_OF_BOUNDS)


#define MALIGP_REG_OFF_WRITE_BOUND_LOW                  0x34
#define MALIGP_REG_OFF_PERF_CNT_0_ENABLE                0x3C
#define MALIGP_REG_OFF_PERF_CNT_1_ENABLE                0x40
#define MALIGP_REG_OFF_PERF_CNT_0_SRC                   0x44
#define MALIGP_REG_OFF_PERF_CNT_1_SRC                   0x48
#define MALIGP_REG_OFF_PERF_CNT_0_VALUE                 0x4C
#define MALIGP_REG_OFF_PERF_CNT_1_VALUE                 0x50
#define MALIGP_REG_OFF_STATUS                           0x68

/* Geometry processor version register */
#define MALIGP_REG_OFF_VERSION                          0x6C
#define MALI400_GP_PRODUCT_ID                           0xB07
#define MALIGP_VERSION_DEFAULT                   (MALI400_GP_PRODUCT_ID << 16)

#define MALIGP_REG_OFF_VSCL_START_ADDR_READ             0x80
#define MALIGP_REG_OFF_PLBCL_START_ADDR_READ            0x84
#define MALIGP_CONTR_AXI_BUS_ERROR_STAT                 0x94
#define MALIGP_REGISTER_ADDRESS_SPACE_SIZE              0x98


/**************************************************
 * MALI pixel processor registers defines
 **************************************************/

/* MALI Pixel Processor version register */
#define MALIPP_REG_OFF_VERSION                          0x1000
#define MALI400_PP_PRODUCT_ID                           0xCD07
#define MALIPP_MGMT_VERSION_DEFAULT              (MALI400_PP_PRODUCT_ID << 16)

#define MALIPP_REG_OFF_CURRENT_REND_LIST_ADDR           0x1004
#define MALIPP_REG_OFF_STATUS                           0x1008

/* Control and commands register */
#define MALIPP_REG_OFF_CTRL_MGMT                        0x100c
#define MALIPP_REG_VAL_CTRL_MGMT_STOP_BUS               (1 << 0)
#define MALIPP_REG_VAL_CTRL_MGMT_FORCE_RESET            (1 << 5)
#define MALIPP_REG_VAL_CTRL_MGMT_START_RENDERING        (1 << 6)
#define MALIPP_REG_VAL_CTRL_MGMT_SOFT_RESET             (1 << 7)

/* Interrupt control registers */
#define MALIPP_REG_OFF_INT_RAWSTAT                      0x1020
#define MALIPP_REG_OFF_INT_CLEAR                        0x1024
#define MALIPP_REG_OFF_INT_MASK                         0x1028
#define MALIPP_REG_OFF_INT_STATUS                       0x102c
/* Interrupt statuses */
#define MALIPP_REG_VAL_IRQ_END_OF_FRAME                 (1 << 0)
#define MALIPP_REG_VAL_IRQ_END_OF_TILE                  (1 << 1)
#define MALIPP_REG_VAL_IRQ_HANG                         (1 << 2)
#define MALIPP_REG_VAL_IRQ_FORCE_HANG                   (1 << 3)
#define MALIPP_REG_VAL_IRQ_BUS_ERROR                    (1 << 4)
#define MALIPP_REG_VAL_IRQ_BUS_STOP                     (1 << 5)
#define MALIPP_REG_VAL_IRQ_CNT_0_LIMIT                  (1 << 6)
#define MALIPP_REG_VAL_IRQ_CNT_1_LIMIT                  (1 << 7)
#define MALIPP_REG_VAL_IRQ_WRITE_BOUNDARY_ERROR         (1 << 8)
#define MALIPP_REG_VAL_IRQ_INVALID_PLIST_COMMAND        (1 << 9)
#define MALIPP_REG_VAL_IRQ_CALL_STACK_UNDERFLOW         (1 << 10)
#define MALIPP_REG_VAL_IRQ_CALL_STACK_OVERFLOW          (1 << 11)
#define MALIPP_REG_VAL_IRQ_RESET_COMPLETED              (1 << 12)


#define MALIPP_REG_OFF_WRITE_BOUNDARY_LOW               0x1044
#define MALIPP_REG_OFF_BUS_ERROR_STATUS                 0x1050
#define MALIPP_REG_OFF_PERF_CNT_0_ENABLE                0x1080
#define MALIPP_REG_OFF_PERF_CNT_0_SRC                   0x1084
#define MALIPP_REG_OFF_PERF_CNT_0_VALUE                 0x108c
#define MALIPP_REG_OFF_PERF_CNT_1_ENABLE                0x10a0
#define MALIPP_REG_OFF_PERF_CNT_1_SRC                   0x10a4
#define MALIPP_REG_OFF_PERF_CNT_1_VALUE                 0x10ac
#define MALIPP_REG_SIZEOF_REGISTER_BANK                 0x10f0

/**************************************************
 * MALI MMU register defines
 **************************************************/

/* Current Page Directory Pointer */
#define MALI_MMU_REG_DTE_ADDR                           0x0000

/* Status of the MMU */
#define MALI_MMU_REG_STATUS                             0x0004
/* MALI MMU ststus bits */
#define MALI_MMU_STATUS_PAGING_ENABLED                 (1 << 0)
#define MALI_MMU_STATUS_PAGE_FAULT_ACTIVE              (1 << 1)
#define MALI_MMU_STATUS_STALL_ACTIVE                   (1 << 2)
#define MALI_MMU_STATUS_IDLE                           (1 << 3)
#define MALI_MMU_STATUS_REPLAY_BUFFER_EMPTY            (1 << 4)
#define MALI_MMU_STATUS_PAGE_FAULT_IS_WRITE            (1 << 5)

/* Command register, used to control the MMU */
#define MALI_MMU_REG_COMMAND                            0x0008
/* MALI MMU commands */
/* Enable paging (memory translation) */
#define MALI_MMU_COMMAND_ENABLE_PAGING                  0x00
/* Disable paging (memory translation) */
#define MALI_MMU_COMMAND_DISABLE_PAGING                 0x01
/*  Enable stall on page fault */
#define MALI_MMU_COMMAND_ENABLE_STALL                   0x02
/* Disable stall on page fault */
#define MALI_MMU_COMMAND_DISABLE_STALL                  0x03
/* Zap the entire page table cache */
#define MALI_MMU_COMMAND_ZAP_CACHE                      0x04
/* Page fault processed */
#define MALI_MMU_COMMAND_PAGE_FAULT_DONE                0x05
/* Reset the MMU back to power-on settings */
#define MALI_MMU_COMMAND_SOFT_RESET                     0x06

/* Logical address of the last page fault */
#define MALI_MMU_REG_PAGE_FAULT_ADDR                    0x000C

/* Used to invalidate the mapping of a single page from the MMU */
#define MALI_MMU_REG_ZAP_ONE_LINE                       0x0010

/* Raw interrupt status, all interrupts visible */
#define MALI_MMU_REG_INT_RAWSTAT                        0x0014
/* Indicate to the MMU that the interrupt has been received */
#define MALI_MMU_REG_INT_CLEAR                          0x0018
/* Enable/disable types of interrupts */
#define MALI_MMU_REG_INT_MASK                           0x001C
/* Interrupt status based on the mask */
#define MALI_MMU_REG_INT_STATUS                         0x0020
/* MALI MMU interrupt registers bits */
/* A page fault occured */
#define MALI_MMU_INT_PAGE_FAULT                         0x01
/* A bus read error occured */
#define MALI_MMU_INT_READ_BUS_ERROR                     0x02


/**************************************************
 * MALI L2 cache register defines
 **************************************************/

/* MALI L2 cache status bits */
#define MALI_L2CACHE_REG_STATUS                         0x0008
/* Command handler of L2 cache is busy */
#define MALI_L2CACHE_STATUS_COMMAND_BUSY                0x01
/* L2 cache is busy handling data requests */
#define MALI_L2CACHE_STATUS_DATA_BUSY                   0x02

/* Misc cache commands, e.g. clear */
#define MALI_L2CACHE_REG_COMMAND                        0x0010
/* Clear the entire cache */
#define MALI_L2CACHE_CMD_CLEAR_ALL                      0x01

#define MALI_L2CACHE_REG_CLEAR_PAGE                     0x0014

/* Enable misc cache features */
#define MALI_L2CACHE_REG_ENABLE                         0x001C
/* Default state of enable register */
#define MALI_L2CACHE_ENABLE_DEFAULT                     0x0
/* Permit cacheable accesses */
#define MALI_L2CACHE_ENABLE_ACCESS                      0x01
/* Permit cache read allocate */
#define MALI_L2CACHE_ENABLE_READ_ALLOCATE               0x02

#define MALI_L2CACHE_REG_PERFCNT_SRC0                   0x0020
#define MALI_L2CACHE_REG_PERFCNT_VAL0                   0x0024
#define MALI_L2CACHE_REG_PERFCNT_SRC1                   0x0028
#define MALI_L2CACHE_REG_PERFCNT_VAL1                   0x002C

typedef struct GeometryProc {
    uint32_t vscl_start;
    uint32_t vscl_end;
    uint32_t plbucl_start;
    uint32_t plbucl_end;
    uint32_t plbu_alloc_start;
    uint32_t plbu_alloc_end;
    uint32_t cmd;
    uint32_t int_rawstat;
/*    uint32_t int_clear; Write only register */
    uint32_t int_mask;
    uint32_t int_stat;
    uint32_t write_bound;
    uint32_t perfcnt0_en;
    uint32_t perfcnt1_en;
    uint32_t perfcnt0_src;
    uint32_t perfcnt1_src;
    uint32_t perfcnt0_value;
    uint32_t perfcnt1_value;
    uint32_t status;
/*    uint32_t version; RO register */
    uint32_t vscl_start_read;
    uint32_t plbcl_start_read;
    uint32_t axi_error;
    uint32_t addr_space_size;

    qemu_irq irq_gp;              /* geometry processor interrupt */
} GeometryProc;

typedef struct PixelProc {
/*    uint32_t version; RO register */
    uint32_t cur_rend_list;
    uint32_t status;
    uint32_t ctrl_mgmt;
    uint32_t int_rawstat;
/*    uint32_t int_clear; Write only register */
    uint32_t int_mask;
    uint32_t int_stat;
    uint32_t write_bound;
    uint32_t bus_error;
    uint32_t perfcnt0_en;
    uint32_t perfcnt0_src;
    uint32_t perfcnt0_value;
    uint32_t perfcnt1_en;
    uint32_t perfcnt1_src;
    uint32_t perfcnt1_value;
    uint32_t sizeof_regs;

    qemu_irq irq_pp;             /* pixel processor interrupt */
} PixelProc;

typedef struct MaliMMU {
    uint32_t dte_addr;
    uint32_t status;
    uint32_t command;
    uint32_t page_fault_addr;
    uint32_t zap_one_line;
    uint32_t int_rawstat;
    uint32_t int_mask;
    uint32_t int_status;

    qemu_irq irq_mmu;             /* MALI MMU interrupt */
} MaliMMU;

typedef struct MaliL2Cache {
    uint32_t status;
    uint32_t command;
    uint32_t clear_page;
    uint32_t enable;
    uint32_t perfcnt_src0;
    uint32_t perfcnt_val0;
    uint32_t perfcnt_src1;
    uint32_t perfcnt_val1;
} MaliL2Cache;

typedef struct Exynos4210G3DState {
    SysBusDevice busdev;
    MemoryRegion iomem;

    GeometryProc gp;              /* Geometry processor */
    PixelProc pp[NUM_OF_PIXPROC];
    MaliMMU gp_mmu;
    MaliMMU pp_mmu[NUM_OF_PIXPROC];
    MaliL2Cache l2_cache;
#ifdef CONFIG_BUILD_GLES
    void *gles2;
#endif
    qemu_irq irq_pmu;             /* power unit interrupt */
} Exynos4210G3DState;

/* TODO maybe reset functions must do something else besides clearing reg-s */
static inline void  exynos4210_maligp_reset(GeometryProc *gp)
{
    memset(gp, 0, offsetof(GeometryProc, irq_gp));
}

static inline void  exynos4210_malipp_reset(PixelProc *pp)
{
    memset(pp, 0, offsetof(PixelProc, irq_pp));
}

static inline void  exynos4210_malimmu_reset(MaliMMU *mmu)
{
    memset(mmu, 0, offsetof(MaliMMU, irq_mmu));
}

static inline void  exynos4210_mali_l2cache_reset(MaliL2Cache *l2cach)
{
    memset(l2cach, 0, sizeof(MaliL2Cache));
}

static void exynos4210_g3d_reset(DeviceState *d)
{
	Exynos4210G3DState *s = DO_UPCAST(Exynos4210G3DState, busdev.qdev, d);
	int i;

    exynos4210_maligp_reset(&s->gp);
    exynos4210_malimmu_reset(&s->gp_mmu);
    exynos4210_mali_l2cache_reset(&s->l2_cache);
    for (i = 0; i < NUM_OF_PIXPROC; i++) {
        exynos4210_malipp_reset(&s->pp[i]);
        exynos4210_malimmu_reset(&s->pp_mmu[i]);
    }
}

/* Geometry processor register map read */
static inline uint32_t exynos4210_maligp_read(GeometryProc *gp,
		                           target_phys_addr_t offset)
{
	DPRINT_L2("MALI GEOMETRY PROCESSOR: read offset 0x"
			TARGET_FMT_plx "\n", offset);

	   switch (offset) {
	    /* Geometry processor */
	    case MALIGP_REG_OFF_CMD:
	    	return gp->cmd;
	    case MALIGP_REG_OFF_INT_RAWSTAT:
	    	return gp->int_rawstat;
	    case MALIGP_REG_OFF_INT_MASK:
	    	return gp->int_mask;
	    case MALIGP_REG_OFF_INT_STAT:
	    	return gp->int_stat;
	    case MALIGP_REG_OFF_VERSION:
	    	return MALIGP_VERSION_DEFAULT;
	    case MALIGP_REG_OFF_INT_CLEAR:
	    	DPRINT_L1("MALI GEOM PROC: read from WO register! Offset 0x"
	    			TARGET_FMT_plx "\n", offset);
	    	return 0;
	    default:
	    	DPRINT_L1("MALI GEOM PROC: read from non-existing register! "
	    			"Offset 0x" TARGET_FMT_plx "\n", offset);
	    	return 0;
	   }
}

/* Geometry processor register map write */
static uint32_t exynos4210_malipp_read(PixelProc *pp, target_phys_addr_t offset)
{
	DPRINT_L2("read offset 0x" TARGET_FMT_plx "\n", offset);

	switch (offset) {
    case MALIPP_REG_OFF_VERSION:
    	return MALIPP_MGMT_VERSION_DEFAULT;
    case MALIPP_REG_OFF_CTRL_MGMT:
    	return pp->ctrl_mgmt;
    case MALIPP_REG_OFF_INT_RAWSTAT:
    	return pp->int_rawstat;
    case MALIPP_REG_OFF_INT_MASK:
    	return pp->int_mask;
    case MALIPP_REG_OFF_INT_STATUS:
    	return pp->int_stat;
    case MALIPP_REG_OFF_INT_CLEAR:
    	DPRINT_L1("MALI PIX PROC: read from WO register! Offset 0x"
    			TARGET_FMT_plx "\n", offset);
    	return 0;
    default:
    	DPRINT_L1("MALI PIX PROC: read from non-existing register! Offset 0x"
    			TARGET_FMT_plx "\n", offset);
    	return 0;
    }
}

/* MALI MMU register map read */
static uint32_t exynos4210_malimmu_read(MaliMMU *mmu, target_phys_addr_t offset)
{
	DPRINT_L2("read offset 0x" TARGET_FMT_plx "\n", offset);

    switch (offset) {
    case MALI_MMU_REG_DTE_ADDR:
    	return mmu->dte_addr;
    case MALI_MMU_REG_COMMAND:
        return mmu->command;
    case MALI_MMU_REG_INT_RAWSTAT:
        return mmu->int_rawstat;
    case MALI_MMU_REG_INT_MASK:
        return mmu->int_mask;
    case MALI_MMU_REG_INT_STATUS:
        return mmu->int_status;
    case MALI_MMU_REG_INT_CLEAR:
    	DPRINT_L1("MALI MMU: read from WO register! Offset 0x"
    			TARGET_FMT_plx "\n", offset);
        return 0;
    default:
    	DPRINT_L1("MALI MMU: read from non-existing register! Offset 0x"
    			TARGET_FMT_plx "\n", offset);
    	return 0;
    }
}

/* MALI L2 cache register map read */
static uint32_t exynos4210_mali_l2cache_read(MaliL2Cache *cache,
		                                      target_phys_addr_t offset)
{
	DPRINT_L2("MALI L2 CACHE: read offset 0x"
			                        TARGET_FMT_plx "\n", offset);

    switch (offset) {
    case MALI_L2CACHE_REG_STATUS:
        return cache->status;
    case MALI_L2CACHE_REG_COMMAND:
        return cache->command;
    case MALI_L2CACHE_REG_ENABLE:
        return cache->enable;
    default:
    	return 0;
    }
}

static uint64_t exynos4210_g3d_read(void *opaque, target_phys_addr_t offset,
        unsigned size)
{
	Exynos4210G3DState *s = (Exynos4210G3DState *)opaque;

    switch (offset) {
    case GP_OFF_START ... GP_OFF_END:
        return exynos4210_maligp_read(&s->gp, offset - GP_OFF_START);
    case PP0_OFF_START ... PP0_OFF_END:
	    DPRINT_L2("MALI PIXEL PROCESSOR#0: ");
        return exynos4210_malipp_read(&s->pp[0], offset - PP0_OFF_START);
    case PP1_OFF_START ... PP1_OFF_END:
        DPRINT_L2("MALI PIXEL PROCESSOR#1: ");
        return exynos4210_malipp_read(&s->pp[1], offset - PP1_OFF_START);
    case PP2_OFF_START ... PP2_OFF_END:
        DPRINT_L2("MALI PIXEL PROCESSOR#2: ");
        return exynos4210_malipp_read(&s->pp[2], offset - PP2_OFF_START);
    case PP3_OFF_START ... PP3_OFF_END:
        DPRINT_L2("MALI PIXEL PROCESSOR#3: ");
        return exynos4210_malipp_read(&s->pp[3], offset - PP3_OFF_START);
    case GP_MMU_OFF_START ... GP_MMU_OFF_END:
        DPRINT_L2("MALI GEOM PROC MMU: ");
        return exynos4210_malimmu_read(&s->gp_mmu, offset - GP_MMU_OFF_START);
    case PP0_MMU_OFF_START ... PP0_MMU_OFF_END:
	    DPRINT_L2("MALI PIX PROC#0 MMU: ");
        return exynos4210_malimmu_read(&s->pp_mmu[0],
                offset - PP0_MMU_OFF_START);
    case PP1_MMU_OFF_START ... PP1_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#1 MMU: ");
        return exynos4210_malimmu_read(&s->pp_mmu[1],
                offset - PP1_MMU_OFF_START);
    case PP2_MMU_OFF_START ... PP2_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#2 MMU: ");
        return exynos4210_malimmu_read(&s->pp_mmu[2],
                offset - PP2_MMU_OFF_START);
    case PP3_MMU_OFF_START ... PP3_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#3 MMU: ");
        return exynos4210_malimmu_read(&s->pp_mmu[3],
                offset - PP3_MMU_OFF_START);
    case L2_OFF_START ... L2_OFF_END:
        return exynos4210_mali_l2cache_read(&s->l2_cache,
                offset - L2_OFF_START);
    default:
    	DPRINT_L1("MALI UNKNOWN REGION: read offset 0x"
    			TARGET_FMT_plx "\n", offset);
    	return 0;
    }
}

/* Geometry processor register map write */
static void exynos4210_maligp_write(GeometryProc *gp,
		                           target_phys_addr_t offset, uint32_t val)
{
	DPRINT_L2("MALI GEOMETRY PROCESSOR: write offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);

    switch (offset) {
    case MALIGP_REG_OFF_CMD:
    	gp->cmd = val;
    	if (val & MALIGP_REG_VAL_CMD_SOFT_RESET) {
    		exynos4210_maligp_reset(gp);
    		gp->int_rawstat |= MALIGP_REG_VAL_IRQ_RESET_COMPLETED;
    		gp->int_stat = gp->int_rawstat & gp->int_mask;
    		qemu_set_irq(gp->irq_gp, gp->int_stat);
    	}
    	break;
    case MALIGP_REG_OFF_INT_CLEAR:
    	gp->int_rawstat &= ~val;
    	gp->int_stat = gp->int_rawstat & gp->int_mask;
    	qemu_irq_lower(gp->irq_gp);
    	break;
    case MALIGP_REG_OFF_INT_MASK:
    	gp->int_mask = val;
    	break;
    case MALIGP_REG_OFF_VERSION: case MALIGP_REG_OFF_INT_RAWSTAT:
    case MALIGP_REG_OFF_INT_STAT:
    	DPRINT_L1("MALI GEOM PROC: writing to RO register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    default:
    	DPRINT_L1("MALI GEOM PROC: writing to non-existing register! "
    	   "Offset 0x" TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    }
}

/* Pixel processor register map write */
static void exynos4210_malipp_write(PixelProc *pp,
		                              target_phys_addr_t offset, uint32_t val)
{
	DPRINT_L2("write offset 0x"
		TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);

    switch (offset) {
    case MALIPP_REG_OFF_CTRL_MGMT:
    	pp->ctrl_mgmt = val;
    	if (val & MALIPP_REG_VAL_CTRL_MGMT_SOFT_RESET) {
    		exynos4210_malipp_reset(pp);
    		pp->int_rawstat |= MALIPP_REG_VAL_IRQ_RESET_COMPLETED;
    		pp->int_stat = pp->int_rawstat & pp->int_mask;
    		qemu_set_irq(pp->irq_pp, pp->int_stat);
    	}
    	break;
    case MALIPP_REG_OFF_INT_CLEAR:
    	pp->int_rawstat &= ~val;
    	pp->int_stat = pp->int_rawstat & pp->int_mask;
    	qemu_irq_lower(pp->irq_pp);
        break;
    case MALIPP_REG_OFF_INT_MASK:
    	pp->int_mask = val;
    	break;
    case MALIPP_REG_OFF_INT_STATUS: case MALIPP_REG_OFF_VERSION:
    case MALIPP_REG_OFF_INT_RAWSTAT:
    	DPRINT_L1("MALI PIX PROC: writing to RO register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    default:
    	DPRINT_L1("MALI PIX PROC: writing to non-existing register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    }
}

/* MALI MMU register map write */
static void exynos4210_malimmu_write(MaliMMU *mmu,
		                           target_phys_addr_t offset, uint32_t val)
{
	DPRINT_L2("write offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);

    switch (offset) {
    case MALI_MMU_REG_DTE_ADDR:
    	mmu->dte_addr = val;
    	break;
    case MALI_MMU_REG_COMMAND:
    	mmu->command = val;
    	if (val & MALI_MMU_COMMAND_SOFT_RESET) {
    		exynos4210_malimmu_reset(mmu);
    	}
        break;
    case MALI_MMU_REG_INT_CLEAR:
    	mmu->int_rawstat &= ~val;
    	mmu->int_status = mmu->int_rawstat & mmu->int_mask;
    	qemu_irq_lower(mmu->irq_mmu);
    	break;
    case MALI_MMU_REG_INT_MASK:
    	mmu->int_mask = val;
    	break;
    case MALI_MMU_REG_INT_STATUS: case MALI_MMU_REG_INT_RAWSTAT:
    	DPRINT_L1("MALI MMU: writing to RO register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    default:
    	DPRINT_L1("MALI MMU: writing to non-existing register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    }
}

/* MALI L2 cache register map write */
static void exynos4210_mali_l2cache_write(MaliL2Cache *cache,
		                           target_phys_addr_t offset, uint32_t val)
{
	DPRINT_L2("MALI L2 CACHE: write offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);

    switch (offset) {
    case MALI_L2CACHE_REG_COMMAND:
    	cache->command = val;
        break;
    case MALI_L2CACHE_REG_ENABLE:
    	cache->enable = val;
        break;
    case MALI_L2CACHE_REG_STATUS:
    	DPRINT_L1("MALI L2 CACHE: writing to RO register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    default:
    	DPRINT_L1("MALI L2 CACHE: writing non-existing register! Offset 0x"
			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    }
}

static void exynos4210_g3d_write(void *opaque, target_phys_addr_t offset,
                                 uint64_t val, unsigned size)
{
	Exynos4210G3DState *s = (Exynos4210G3DState *)opaque;

    switch (offset) {
    case GP_OFF_START ... GP_OFF_END:
        exynos4210_maligp_write(&s->gp, offset - GP_OFF_START, val);
        break;
    case PP0_OFF_START ... PP0_OFF_END:
	    DPRINT_L2("MALI PIXEL PROCESSOR#0: ");
        exynos4210_malipp_write(&s->pp[0], offset - PP0_OFF_START, val);
        break;
    case PP1_OFF_START ... PP1_OFF_END:
		DPRINT_L2("MALI PIXEL PROCESSOR#1: ");
		exynos4210_malipp_write(&s->pp[1], offset - PP1_OFF_START, val);
        break;
    case PP2_OFF_START ... PP2_OFF_END:
		DPRINT_L2("MALI PIXEL PROCESSOR#2: ");
		exynos4210_malipp_write(&s->pp[2], offset - PP2_OFF_START, val);
        break;
    case PP3_OFF_START ... PP3_OFF_END:
		DPRINT_L2("MALI PIXEL PROCESSOR#3: ");
		exynos4210_malipp_write(&s->pp[3], offset - PP3_OFF_START, val);
        break;
    case GP_MMU_OFF_START ... GP_MMU_OFF_END:
        DPRINT_L2("MALI GEOM PROC MMU: ");
        exynos4210_malimmu_write(&s->gp_mmu, offset - GP_MMU_OFF_START, val);
        break;
    case PP0_MMU_OFF_START ... PP0_MMU_OFF_END:
	    DPRINT_L2("MALI PIX PROC#0 MMU: ");
        exynos4210_malimmu_write(&s->pp_mmu[0],
                offset - PP0_MMU_OFF_START, val);
        break;
    case PP1_MMU_OFF_START ... PP1_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#1 MMU: ");
        exynos4210_malimmu_write(&s->pp_mmu[1],
                offset - PP1_MMU_OFF_START, val);
        break;
    case PP2_MMU_OFF_START ... PP2_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#2 MMU: ");
        exynos4210_malimmu_write(&s->pp_mmu[2],
                offset - PP2_MMU_OFF_START, val);
        break;
    case PP3_MMU_OFF_START ... PP3_MMU_OFF_END:
        DPRINT_L2("MALI PIX PROC#3 MMU: ");
        exynos4210_malimmu_write(&s->pp_mmu[3],
                offset - PP3_MMU_OFF_START, val);
        break;
    case L2_OFF_START ... L2_OFF_END:
        exynos4210_mali_l2cache_write(&s->l2_cache, offset - L2_OFF_START, val);
        break;
    default:
    	DPRINT_L1("MALI UNKNOWN REGION: write offset 0x"
    			TARGET_FMT_plx ", value=%d(0x%x) \n", offset, val, val);
    	break;
    }
}

static const MemoryRegionOps exynos4210_g3d_mmio_ops = {
    .read = exynos4210_g3d_read,
    .write = exynos4210_g3d_write,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false
    },
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int exynos4210_g3d_init(SysBusDevice *dev)
{
    Exynos4210G3DState *s = FROM_SYSBUS(Exynos4210G3DState, dev);

    sysbus_init_irq(dev, &s->pp[0].irq_pp);
    sysbus_init_irq(dev, &s->pp[1].irq_pp);
    sysbus_init_irq(dev, &s->pp[2].irq_pp);
    sysbus_init_irq(dev, &s->pp[3].irq_pp);
    sysbus_init_irq(dev, &s->gp.irq_gp);
    sysbus_init_irq(dev, &s->irq_pmu);
    sysbus_init_irq(dev, &s->pp_mmu[0].irq_mmu);
    sysbus_init_irq(dev, &s->pp_mmu[1].irq_mmu);
    sysbus_init_irq(dev, &s->pp_mmu[2].irq_mmu);
    sysbus_init_irq(dev, &s->pp_mmu[3].irq_mmu);
    sysbus_init_irq(dev, &s->gp_mmu.irq_mmu);

    memory_region_init_io(&s->iomem, &exynos4210_g3d_mmio_ops, s,
            "exynos4210.g3d", EXYNOS4210_G3D_REG_MEM_SIZE);
    sysbus_init_mmio(dev, &s->iomem);
#ifdef CONFIG_BUILD_GLES
    s->gles2 = gles2_init(first_cpu);
#endif

    return 0;
}

static void exynos4210_g3d_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    dc->reset = exynos4210_g3d_reset;
    k->init = exynos4210_g3d_init;
}

static TypeInfo exynos4210_g3d_info = {
    .name = "exynos4210.g3d",
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Exynos4210G3DState),
    .class_init = exynos4210_g3d_class_init,
};

static void exynos4210_g3d_register_devices(void)
{
    type_register_static(&exynos4210_g3d_info);
}

type_init(exynos4210_g3d_register_devices)
