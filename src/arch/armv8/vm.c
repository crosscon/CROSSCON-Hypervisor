/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/sysregs.h>
#include <fences.h>
#include <string.h>
#include <config.h>
#include "util.h"

void vm_arch_init(struct vm* vm, const struct vm_config* vm_config)
{
    if (vm->master == cpu()->id) {
        vgic_init(vm, &vm_config->platform.arch.gic);
    }
    cpu_sync_and_clear_msgs(&vm->sync);
}

struct vcpu* vm_get_vcpu_by_mpidr(struct vm* vm, unsigned long mpidr)
{
    for (cpuid_t vcpuid = 0; vcpuid < vm->cpu_num; vcpuid++) {
        struct vcpu* vcpu = vm_get_vcpu(vm, vcpuid);
        if ((vcpu->arch.vmpidr & MPIDR_AFF_MSK) == (mpidr & MPIDR_AFF_MSK)) {
            return vcpu;
        }
    }

    return NULL;
}

static unsigned long vm_cpuid_to_mpidr(struct vm* vm, vcpuid_t cpuid)
{
    if (cpuid > vm->cpu_num) {
        return ~(~MPIDR_RES1 & MPIDR_RES0_MSK); // return an invlid mpidr by
                                                // inverting res bits
    }

    unsigned long mpidr = cpuid | MPIDR_RES1;

    if (vm->cpu_num == 1) {
        mpidr |= MPIDR_U_BIT;
    }

    return mpidr;
}

void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm)
{
    /* CROSSCON TODO */
    /*vcpu->arch.vmpidr = vm_cpuid_to_mpidr(vm, vcpu->id);
    sysreg_vmpidr_el2_write(vcpu->arch.vmpidr); */
    vcpu->arch.sysregs.hyp.vmpidr_el2 = vm_cpuid_to_mpidr(vm, vcpu->id);
    vcpu->arch.sysregs.hyp.cntvoff_el2 = 0;

    vcpu->arch.psci_ctx.state = vcpu->id == 0 ? ON : OFF;

    vcpu_arch_profile_init(vcpu, vm);

    vgic_cpu_init(vcpu);
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    memset(&vcpu->regs, 0, sizeof(struct arch_regs));

    vcpu_subarch_reset(vcpu);
    /* CROSSCON TODO */
    vcpu->regs.elr_el2 = entry;
    vcpu->regs.spsr_el2 = SPSR_EL1h | SPSR_F | SPSR_I | SPSR_A | SPSR_D;

    vcpu->arch.sysregs.hyp.cntvoff_el2 = 0;
    // sysreg_cntvoff_el2_write(0);
    vcpu_writepc(vcpu, entry);

    /**
     *  See ARMv8-A ARM section D1.9.1 for registers that must be in a known state at reset.
     */
    /* CROSSCON TODO */
    // sysreg_sctlr_el1_write(SCTLR_RES1);
    vcpu->arch.sysregs.vm.sctlr_el1 = SCTLR_RES1;

    vcpu->arch.sysregs.vm.cntkctl_el1 = 0;
    /* sysreg_cntkctl_el1_write(0); */
    /* CROSSCON TODO sysreg_pmcr_el0_write(0); */

    /**
     *  TODO: ARMv8-A ARM mentions another implementation optional registers that reset to a known
     * value.
     */
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    return vcpu->arch.psci_ctx.state == ON;
}

