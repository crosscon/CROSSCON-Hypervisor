
/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_SAU_H__
#define __ARCH_SAU_H__

#include <bao.h>
#include <arch/sysregs.h>
#include <mem.h>

struct sau_vm {
    BITMAP_ALLOC(bitmap, SAU_ARCH_MAX_NUM_ENTRIES);
    /* A locked region means that it can never be removed from the SAU. */
    BITMAP_ALLOC(locked, SAU_ARCH_MAX_NUM_ENTRIES);

    struct {
        unsigned long rbar;
        unsigned long rlar;
    } entry[SAU_ARCH_MAX_NUM_ENTRIES];
};

void sau_arch_init(void);
void sau_arch_enable(void);
void sau_arch_disable(void);
bool sau_add_region(struct addr_space* as, struct mp_region* reg, bool locked);
bool sau_remove_region(struct addr_space* as, struct mp_region* reg);
bool sau_update_region(struct addr_space* as, struct mp_region* reg);
bool sau_perms_compatible(mem_flags_t perms1, mem_flags_t perms2);
void sau_restore(struct sau_vm* sau_vm);

#endif /* __ARCH_SAU_H__ */
