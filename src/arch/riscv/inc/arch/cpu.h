/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_CPU_H__
#define __ARCH_CPU_H__

#include <crossconhyp.h>
#include <arch/fpu.h>
#include <arch/csrs.h>

#ifdef MEM_PROT_MPU
#include <arch/spmp.h>
#endif

#define CPU_HAS_EXTENSION(EXT) (DEFINED(EXT))

extern cpuid_t CPU_MASTER;

struct cpu_arch {
    unsigned long extra_scratch;
    unsigned hart_id;
#if (IRQC == PLIC)
    unsigned plic_cntxt;
#endif
    enum fpu_state fpu_state;
    unsigned long imsic_guest_int_file_avail;
#ifdef MEM_PROT_MPU
    struct {
        uint16_t entry_allocation_count[SPMP_MAX_NUM_ENTRIES];
        BITMAP_ALLOC(entry_locked, SPMP_MAX_NUM_ENTRIES);
        bool* resident[SPMP_MAX_NUM_ENTRIES];
        struct spmp* active_guest_spmp;
    } spmp_mngmnt;

    struct spmp spmp;
#endif
};

static inline struct cpu* cpu(void)
{
    return (struct cpu*)csrs_sscratch_read();
}

#endif /* __ARCH_CPU_H__ */
