/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <mem.h>
#include <cpu.h>
#include <arch/spmp.h>

void as_arch_init(struct addr_space* as)
{
    UNUSED_ARG(as);
}

void mem_guest_ipa_translate(struct addr_space* as, vaddr_t ipa, paddr_t* pa)
{
    UNUSED_ARG(as);
    UNUSED_ARG(ipa);
    UNUSED_ARG(pa);
}

void mpu_enable()
{
    spmp_enable();
}

bool mpu_update(struct addr_space* as, struct mp_region* mpr)
{
    return spmp_update(as, mpr);
}

bool mpu_perms_compatible(struct addr_space* as, mem_flags_t perms1, mem_flags_t perms2)
{
    UNUSED_ARG(as);
    return spmp_perms_compatible(perms1, perms2);
}