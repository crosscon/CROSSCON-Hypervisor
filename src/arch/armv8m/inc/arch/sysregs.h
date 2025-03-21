/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_SYSREGS_H__
#define __ARCH_SYSREGS_H__

#include <bao.h>
#include <bit.h>

#define NS_ALIAS_OFFSET          (0x20000UL)

/* System Control Block */
#define SCB_BASE                 (0xE000ED00UL)
#define SCB_BASE_NS              (SCB_BASE + NS_ALIAS_OFFSET)

#define SCB_SHCSR_SYSTICKENA     (1 << 1)
#define SCB_SHCSR_MEMFAULTENA    (1 << 16)
#define SCB_SHCSR_BUSFAULTENA    (1 << 17)
#define SCB_SHCSR_USGFAULTENA    (1 << 18)
#define SCB_SHCSR_SECUREFAULTEMA (1 << 19)
#define SCB_SHCSR_SYSTICKACT     (1UL << 11UL)

#define SCB_AIRCR_VECTKEY        (0x5FAUL << 16)
#define SCB_AIRCR_VECTKEY_MSK    (0xFFFFUL << 16)
#define SCB_AIRCR_PRIS           (1 << 14)
#define SCB_AIRCR_BFHFNMINS      (1 << 13)

#define SCB_ICSR_PENDSTSET       (1 << 26)
#define SCB_ICSR_PENDSTCLR       (1 << 25)
#define SCB_ICSR_VECTACTIVE_MSK  (0x1FFUL)

#define SCB_SHCSR_EN_ALL_FAULTS                                              \
    (SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA | \
        SCB_SHCSR_SECUREFAULTEMA)

#ifndef __ASSEMBLER__
static struct scb* const scb_s = (struct scb*)SCB_BASE;
static struct scb* const scb_ns = (struct scb*)SCB_BASE_NS;
struct scb {
    volatile const uint32_t cpuid;
    volatile uint32_t icsr;
    volatile uint32_t vtor;
    volatile uint32_t aircr;
    volatile uint32_t scr;
    volatile uint32_t ccr;
    volatile uint32_t shpr1; //[7:0] MemManage | [15:8] Bus Fault | [23:16] UsageFault | [31:24]
                             // SecureFault
    volatile uint32_t shpr2; //[31:24] SVCall
    volatile uint32_t shpr3; //[7:0] DebugMonitor | [23:16] PendSV | [31:24] SysTick
    volatile uint32_t shcsr;
    volatile uint32_t cfsr;
    volatile uint32_t hfsr;
    volatile uint32_t dfsr;
    volatile uint32_t mmfar;
    volatile uint32_t bfar;
    volatile uint32_t afsr;
    volatile const uint32_t id_pfr0;
    volatile const uint32_t id_pfr1;
    volatile const uint32_t id_dfr0;
    volatile const uint32_t id_afr0;
    volatile const uint32_t id_mmfr0;
    volatile const uint32_t id_mmfr1;
    volatile const uint32_t id_mmfr2;
    volatile const uint32_t id_mmfr3;
    volatile const uint32_t id_isar0;
    volatile const uint32_t id_isar1;
    volatile const uint32_t id_isar2;
    volatile const uint32_t id_isar3;
    volatile const uint32_t id_isar4;
    volatile const uint32_t id_isar5;
    volatile const uint32_t clidr;
    volatile const uint32_t ctr;
    volatile uint32_t ccsidr;
    volatile uint32_t csselr;
    volatile uint32_t cpacr;
    volatile uint32_t nsacr;
};

#endif

/* Debug Control Block */
#define DCB_BASE    (0xE000EDF0UL)
#define DCB_BASE_NS (DCB_BASE + NS_ALIAS_OFFSET)

#ifndef __ASSEMBLER__
static struct dcb* const dcb_s = (struct dcb*)DCB_BASE;
static struct dcb* const dcb_ns = (struct dcb*)DCB_BASE_NS;
struct dcb {
    volatile uint32_t dhcsr;
    volatile const uint32_t dcrsr;
    volatile uint32_t dcrdr;
    volatile uint32_t demcr;
    volatile const uint32_t dscemcr;
    volatile uint32_t dauthctrl;
    volatile uint32_t dscsr;
};

#endif

/* Security Attribution Unit */
#define SAU_BASE              (0xE000EDD0UL)

#define SAU_CTRL_ENABLE       (1UL << 0)
#define SAU_CTRL_ALLNS        (1UL << 1)

#define SAU_TYPE_RNR_MSK      (0xFFUL)
#define SAU_TYPE_N_RGN(REG)   (REG & SAU_TYPE_RNR_MSK)

#define SAU_RNR_REGION_MSK    (0xFFUL)

#define SAU_RBAR_RES_MSK      (0x1FUL)
#define SAU_RBAR_BADDR_MSK    (~SAU_RBAR_RES_MSK)
#define SAU_RBAR_BASE(BASE)   ((BASE) & SAU_RBAR_BADDR_MSK)

#define SAU_RLAR_EN           (1 << 0)
#define SAU_RLAR_NSC          (1 << 1)
#define SAU_RLAR_FLAGS_MSK    (0x1FUL)
#define SAU_RLAR_FLAGS(RLAR)  ((RLAR) & SAU_RLAR_FLAGS_MSK)
#define SAU_RLAR_LADDR_MSK    (~SAU_RLAR_FLAGS_MSK)
#define SAU_RLAR_LIMIT(LIMIT) (((LIMIT) & SAU_RLAR_LADDR_MSK) | SAU_RLAR_FLAGS_MSK)

#ifndef __ASSEMBLER__
static struct sau* const sau = (struct sau*)SAU_BASE;
struct sau {
    volatile uint32_t ctrl;
    volatile const uint32_t type;
    volatile uint32_t rnr;
    volatile uint32_t rbar;
    volatile uint32_t rlar;
    volatile uint32_t sfsr;
    volatile uint32_t sfar;
};

#endif

/* Memory Protection Unit */
#define MPU_BASE              (0xE000ED90UL)
#define MPU_NS_BASE           (MPU_BASE + NS_ALIAS_OFFSET)

#define MPU_CTRL_ENABLE       (1 << 0)
#define MPU_CTRL_HFNMIENA     (1 << 1)
#define MPU_CTRL_PRIVDEFENA   (1 << 2)

#define MPU_TYPE_N_RGN_MSK    (0xFF00UL)
#define MPU_TYPE_N_RGN(REG)   ((REG & MPU_TYPE_N_RGN_MSK) >> 8)

#define MPU_RNR_REGION_MSK    (0xFFUL)

#define MPU_RBAR_XN           (1 << 0)
#define MPU_RBAR_SH_MSK       (0x3UL)
#define MPU_RBAR_SH_NS        (0 << 1)
#define MPU_RBAR_SH_OS        (2 << 1)
#define MPU_RBAR_SH_IS        (3 << 1)
#define MPU_RBAR_AP_RW_PLVL   (0 << 3) // Priv: Read/Write, Unpriv: No access
#define MPU_RBAR_AP_RW_ALL    (1 << 3) // Priv: Read/Write, Unpriv: Read/Write
#define MPU_RBAR_AP_RO_PLVL   (2 << 3) // Priv: Read-Only, Unpriv: No access
#define MPU_RBAR_AP_RO_ALL    (3 << 3) // Priv: Read-Only, Unpriv: Read-Only
#define MPU_RBAR_FLAGS_MSK    (0x1FUL)
#define MPU_RBAR_FLAGS(RBAR)  ((RBAR) & MPU_RBAR_FLAGS_MSK)
#define MPU_RBAR_BASE_MSK     (~MPU_RBAR_FLAGS_MSK)
#define MPU_RBAR_BASE(BASE)   ((BASE) & MPU_RBAR_BASE_MSK)

#define MPU_RLAR_EN           (1 << 0)
#define MPU_RLAR_ATTR_MSK     (0x3UL << 1)
#define MPU_RLAR_ATTR(N)      (((N) << 1) & MPU_RLAR_ATTR_MSK)
#define MPU_RLAR_PXN          (1 << 4)
#define MPU_RLAR_FLAGS_MSK    (0x1FUL)
#define MPU_RLAR_FLAGS(RLAR)  ((RLAR) & MPU_RLAR_FLAGS_MSK)
#define MPU_RLAR_LIMIT_MSK    (~MPU_RLAR_FLAGS_MSK)
#define MPU_RLAR_LIMIT(LIMIT) (((LIMIT) & MPU_RLAR_LIMIT_MSK) | MPU_RLAR_FLAGS_MSK)

#define MPU_MAIR_ATTR_WIDTH   (8)
#define MPU_MAIR_ATTR_NUM     (8)

#define MPU_MAIR_DEV_nGnRnE   (0x0 << 2)
#define MPU_MAIR_DEV_nGnRE    (0x1 << 2)
#define MPU_MAIR_DEV_nGRE     (0x2 << 2)
#define MPU_MAIR_DEV_GRE      (0x3 << 2)

#define MPU_MAIR_OWTT         (0x0 << 6)
#define MPU_MAIR_ONC          (0x1 << 6)
#define MPU_MAIR_OWBT         (0x1 << 6)
#define MPU_MAIR_OWTNT        (0x2 << 6)
#define MPU_MAIR_OWBNT        (0x3 << 6)

#define MPU_MAIR_IWTT         (0x0 << 2)
#define MPU_MAIR_INC          (0x1 << 2)
#define MPU_MAIR_IWBT         (0x1 << 2)
#define MPU_MAIR_IWTNT        (0x2 << 2)
#define MPU_MAIR_IWBNT        (0x3 << 2)

/**
 * Default hypervisor memory attributes 0 -> Device-nGnRnE 1 -> Normal, Inner/Outer  WB/WA/RA 2 ->
 * Device-nGnRE
 */
#define MAIR_SEC_PLVL_DFLT \
    (MPU_MAIR_ONC | MPU_MAIR_INC) | (MPU_MAIR_DEV_nGnRnE << MPU_MAIR_ATTR_WIDTH)

#ifndef __ASSEMBLER__
static struct mpu* const mpu_s = (struct mpu*)MPU_BASE;
static struct mpu* const mpu_ns = (struct mpu*)MPU_NS_BASE;
struct mpu {
    volatile const uint32_t type;
    volatile uint32_t ctrl;
    volatile uint32_t rnr;
    volatile uint32_t rbar;
    volatile uint32_t rlar;
    volatile uint32_t rbar_a1;
    volatile uint32_t rlar_a1;
    volatile uint32_t rbar_a2;
    volatile uint32_t rlar_a2;
    volatile uint32_t rbar_a3;
    volatile uint32_t rlar_a3;
    uint32_t reserved;
    volatile uint32_t mair0;
    volatile uint32_t mair1;
};

#endif

/* SysTick */
#define SYSTICK_BASE          (0xE000E010UL)
#define SYSTICK_NS_BASE       (SYSTICK_BASE + NS_ALIAS_OFFSET)

#define SYSTICK_CSR_ENABLE    (1UL << 0UL)
#define SYSTICK_CSR_TICKINT   (1UL << 1UL)
#define SYSTICK_CSR_CLKSOURCE (1UL << 2UL)

#ifndef __ASSEMBLER__
static struct systick* const systick_s = (struct systick*)SYSTICK_BASE;
static struct systick* const systick_ns = (struct systick*)SYSTICK_NS_BASE;
struct systick {
    volatile uint32_t csr;
    volatile uint32_t rvr;
    volatile uint32_t cvr;
    volatile const uint32_t calib;
};

#endif

/* Nested Vectored Interrupt Controller */
#define NVIC_BASE           (0xE000E100UL)
#define NVIC_NS_BASE        (NVIC_BASE + NS_ALIAS_OFFSET)

#define NVIC_MAX_INTERRUPTS (496U)
#ifndef PLAT_NVIC_MAX_INTERRUPTS
#define PLAT_NVIC_MAX_INTERRUPTS NVIC_MAX_INTERRUPTS
#endif

#define NVIC_PRIO_BITS 3U // Number of priority bits implemented in the NVIC

#ifndef __ASSEMBLER__
static struct nvic* const nvic_s = (struct nvic*)NVIC_BASE;
static struct nvic* const nvic_ns = (struct nvic*)NVIC_NS_BASE;
enum exc_numbers {
    EXC_RESET = 1U,
    EXC_NMI = 2U,
    EXC_HARD_FAULT = 3U,
    EXC_MEM_MANAGE = 4U,
    EXC_BUS_FAULT = 5U,
    EXC_USAGE_FAULT = 6U,
    EXC_SEC_FAULT = 7U,
    EXC_SVCALL = 11U,
    EXC_DEBUG_MON = 12U,
    EXC_PENDSV = 14U,
    EXC_SYSTICK = 15U,
    EXT_IRQ_BASE = 16U,
};

struct nvic {
    volatile uint32_t iser[16]; // 0x000 + x*4: Interrupt Set-Enable Register x
    uint32_t reserved0[16];
    volatile uint32_t icer[16]; // 0x080 + x*4: Interrupt Clear-Enable Register x
    uint32_t reserved1[16];
    volatile uint32_t ispr[16]; // 0x100 + x*4: Interrupt Set-Pending Register x
    uint32_t reserved2[16];
    volatile uint32_t icpr[16]; // 0x180 + x*4: Interrupt Clear-Pending Register x
    uint32_t reserved3[16];
    volatile uint32_t iabr[16]; // 0x200 + x*4: Interrupt Active Bit Register x
    uint32_t reserved4[16];
    volatile uint32_t itns[16]; // 0x280 + x*4: Interrupt Target Non-Secure Register x
    uint32_t reserved5[16];
    volatile uint32_t ipr[496]; // 0x300 + x*4: Interrupt Priority Register x
    uint32_t reserved6[580];
    volatile uint32_t stir;     // 0xe00: Software Trigger Interrupt Register
};

static inline void interrupts_arch_enable_all(void)
{
    __asm__ volatile("cpsie i");
}

static inline void interrupts_arch_disable_all(void)
{
    __asm__ volatile("cpsid i");
}

#endif

#ifndef __ASSEMBLER__

#define SP_REG_GEN_ACCESSORS(name, reg)                              \
    static inline unsigned long sp_reg_##name##_read(void)           \
    {                                                                \
        unsigned long _temp;                                         \
        __asm__ volatile("mrs %0, " XSTR(reg) "\n\r" : "=r"(_temp)); \
        return _temp;                                                \
    }                                                                \
    static inline void sp_reg_##name##_write(unsigned long val)      \
    {                                                                \
        __asm__ volatile("msr " XSTR(reg) ", %0\n\r" : : "r"(val));  \
    }

SP_REG_GEN_ACCESSORS(psp_s, psp)

#endif /* |__ASSEMBLER__ */

#endif /* __ARCH_SYSREGS_H__ */
