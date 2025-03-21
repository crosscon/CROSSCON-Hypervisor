/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef MEM_ARCH_H
#define MEM_ARCH_H

#include <bao.h>
#include <arch/sysregs.h>
#include <arch/mpu.h>

typedef mpu_flags_t mem_flags_t;

#define PTE_FLAGS(_rbar, _rlar) \
    ((mem_flags_t){             \
        .rbar = (_rbar),        \
        .rlar = (_rlar),        \
    })

#define PTE_INVALID PTE_FLAGS(0, 0)

// TODO:ARMV8M - Use separate attributes for code and data
#define PTE_HYP_FLAGS \
    PTE_FLAGS(MPU_RBAR_XN | MPU_RBAR_AP_RW_PLVL | MPU_RBAR_SH_NS, MPU_RLAR_ATTR(1) | MPU_RLAR_EN)
#define PTE_HYP_FLAGS_CODE \
    PTE_FLAGS(MPU_RBAR_AP_RO_PLVL | MPU_RBAR_SH_NS, MPU_RLAR_ATTR(1) | MPU_RLAR_EN)
#define PTE_HYP_DEV_FLAGS \
    PTE_FLAGS(MPU_RBAR_XN | MPU_RBAR_AP_RW_PLVL | MPU_RBAR_SH_NS, MPU_RLAR_ATTR(2) | MPU_RLAR_EN)

// TODO:ARMV8M - We are missing flags to distinguish flags for mem regions RX or RW

#define PTE_VM_FLAGS     PTE_FLAGS(0, SAU_RLAR_EN)
#define PTE_VM_DEV_FLAGS PTE_FLAGS(0, SAU_RLAR_EN)
#define PTE_VM_HC_FLAGS  PTE_FLAGS(0, SAU_RLAR_EN | SAU_RLAR_NSC)

#define MPU_ARCH_MAX_NUM_ENTRIES \
    (8) // TODO:ARMV8M - This is implementation-def so it should be defined in the platform
#define SAU_ARCH_MAX_NUM_ENTRIES \
    (8) // TODO:ARMV8M - This is implementation-def so it should be defined in the platform

static inline size_t mpu_granularity(void)
{
    return (size_t)PAGE_SIZE;
}

bool mpu_arch_perms_compatible(mem_flags_t perms1, mem_flags_t perms2);

#endif /* MEM_ARCH_H */
