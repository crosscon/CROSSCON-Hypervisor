/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vmm.h>
#include <arch/sysregs.h>
#include <interrupts.h>
#include <platform.h>

void vmm_arch_init()
{
    /* TODO check this against crosscon hyp */
    vmm_arch_profile_init();

    uint64_t hcr = HCR_VM_BIT | HCR_IMO_BIT | HCR_FMO_BIT | HCR_TSC_BIT;

    if (DEFINED(AARCH64)) {
        hcr |= HCR_RW_BIT | HCR_APK_BIT | HCR_API_BIT;
    }

    sysreg_hcr_el2_write(hcr);

    sysreg_cptr_el2_write(0);

    if (cpu_is_master()) {
        interrupts_set_shared(platform.arch.generic_timer.irqs.virtual);
    }
}
