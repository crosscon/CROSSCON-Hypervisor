/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef MEM_ARCH_H
#define MEM_ARCH_H

#include <crossconhyp.h>
#include <arch/spmp.h>

struct addr_space_arch {
    EMPTY_STRUCT_FIELDS
};

typedef spmp_cfg_t mem_flags_t;

#define PTE_INVALID       ((mem_flags_t){ .a = SPMPCFG_A_OFF })
#define PTE_HYP_FLAGS     ((mem_flags_t){ .r = 1, .w = 1, .x = 1 })
#define PTE_HYP_RX_FLAGS  ((mem_flags_t){ .r = 1, .x = 1 })
#define PTE_HYP_DEV_FLAGS ((mem_flags_t){ .r = 1, .w = 1 })
#define PTE_VM_FLAGS      ((mem_flags_t){ .r = 1, .w = 1, .x = 1 })
#define PTE_VM_DEV_FLAGS  ((mem_flags_t){ .r = 1, .w = 1 })

static inline size_t mpu_granularity(void)
{
    return (size_t)PAGE_SIZE;
}

#endif
