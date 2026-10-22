/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <config.h>
#include <arch/sysregs.h>

void vm_arch_profile_init(struct vm* vm)
{
    if (DEFINED(MEM_PROT_MPU) && DEFINED(AARCH64) && vm->config->platform.mmu) {
        unsigned long vtcr = VTCR_MSA;
        sysreg_vtcr_el2_write(vtcr);
    }
}

void vcpu_arch_profile_init(struct vcpu* vcpu, struct vm* vm)
{
    vcpu->arch.sysregs.hyp.vsctlr_el2 =
        (vm->id << VSCTLR_EL2_VMID_OFF) & VSCTLR_EL2_VMID_MSK;
}

void vcpu_arch_profile_save_state(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);
}

void vcpu_arch_profile_restore_state(struct vcpu* vcpu)
{
    sysreg_vsctlr_el2_write((unsigned long)vcpu->arch.sysregs.hyp.vsctlr_el2);
}
