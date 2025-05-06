/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/sysregs.h>
#include <vfp.h>
#include <vtimer.h>

unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg)
{
    if (reg > 30) {
        return 0;
    }
    return vcpu->regs.x[reg];
}

void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val)
{
    if (reg > 30) {
        return;
    }
    vcpu->regs.x[reg] = val;
}

unsigned long vcpu_readpc(struct vcpu* vcpu)
{
    return vcpu->regs.elr_el2;
}

void vcpu_writepc(struct vcpu* vcpu, unsigned long pc)
{
    vcpu->regs.elr_el2 = pc;
}

void vcpu_subarch_reset(struct vcpu* vcpu)
{
    vcpu->regs.spsr_el2 = SPSR_EL1h | SPSR_F | SPSR_I | SPSR_A | SPSR_D;
}

void vcpu_restore_state(struct vcpu* vcpu)
{
    sysreg_cptr_el2_write(vcpu->arch.sysregs.hyp.cptr_el2);
    sysreg_elr_el2_write(vcpu->regs.elr_el2);
    sysreg_spsr_el2_write(vcpu->regs.spsr_el2);
    sysreg_vttbr_el2_write(vcpu->arch.sysregs.hyp.vttbr_el2);
    sysreg_vmpidr_el2_write(vcpu->arch.sysregs.hyp.vmpidr_el2);
    sysreg_cntvoff_el2_write(vcpu->arch.sysregs.hyp.cntvoff_el2);
    sysreg_vbar_el1_write(vcpu->arch.sysregs.vm.vbar_el1);
    sysreg_tpidr_el1_write(vcpu->arch.sysregs.vm.tpidr_el1);
    sysreg_mair_el1_write(vcpu->arch.sysregs.vm.mair_el1);
    sysreg_amair_el1_write(vcpu->arch.sysregs.vm.amair_el1);
    sysreg_tcr_el1_write(vcpu->arch.sysregs.vm.tcr_el1);
    sysreg_ttbr0_el1_write(vcpu->arch.sysregs.vm.ttbr0_el1);
    sysreg_ttbr1_el1_write(vcpu->arch.sysregs.vm.ttbr1_el1);
    sysreg_sp_el0_write(vcpu->arch.sysregs.vm.sp_el0);
    sysreg_sp_el1_write(vcpu->arch.sysregs.vm.sp_el1);
    sysreg_spsr_el1_write(vcpu->arch.sysregs.vm.spsr_el1);
    sysreg_sctlr_el1_write(vcpu->arch.sysregs.vm.sctlr_el1);
    sysreg_actlr_el1_write(vcpu->arch.sysregs.vm.actlr_el1);
    sysreg_par_el1_write(vcpu->arch.sysregs.vm.par_el1);
    sysreg_far_el1_write(vcpu->arch.sysregs.vm.far_el1);
    sysreg_esr_el1_write(vcpu->arch.sysregs.vm.esr_el1);
    sysreg_elr_el1_write(vcpu->arch.sysregs.vm.elr_el1);
    sysreg_afsr0_el1_write(vcpu->arch.sysregs.vm.afsr0_el1);
    sysreg_afsr1_el1_write(vcpu->arch.sysregs.vm.afsr1_el1);
    sysreg_tpidrro_el0_write(vcpu->arch.sysregs.vm.tpidrro_el0);
    sysreg_tpidr_el0_write(vcpu->arch.sysregs.vm.tpidr_el0);
    sysreg_cntv_ctl_el0_write(vcpu->arch.sysregs.vm.cntv_ctl_el0);
    sysreg_cntv_cval_el0_write(vcpu->arch.sysregs.vm.cntv_cval_el0);
    sysreg_cntkctl_el1_write(vcpu->arch.sysregs.vm.cntkctl_el1);
    sysreg_cpacr_el1_write(vcpu->arch.sysregs.vm.cpacr_el1);
    sysreg_contextidr_el1_write(vcpu->arch.sysregs.vm.contextidr_el1);
    sysreg_csselr_el1_write(vcpu->arch.sysregs.vm.csselr_el1);

    vcpu_arch_profile_restore_state(vcpu);

    vfp_restore_state(&vcpu->regs.vfp);
    vtimer_restore_state(vcpu);
    vgic_restore_state(vcpu);
}

void vcpu_save_state(struct vcpu* vcpu)
{
    vcpu->arch.sysregs.vm.cpacr_el1 = sysreg_cpacr_el1_read();
    vcpu->arch.sysregs.vm.contextidr_el1 = sysreg_contextidr_el1_read();
    vcpu->arch.sysregs.vm.csselr_el1 = sysreg_csselr_el1_read();

    vcpu->arch.sysregs.hyp.cptr_el2 = sysreg_cptr_el2_read();
    vcpu->regs.elr_el2 = sysreg_elr_el2_read();
    vcpu->regs.spsr_el2 = sysreg_spsr_el2_read();
    vcpu->arch.sysregs.hyp.vttbr_el2 = sysreg_vttbr_el2_read();
    vcpu->arch.sysregs.hyp.vmpidr_el2 = sysreg_vmpidr_el2_read();
    vcpu->arch.sysregs.hyp.cntvoff_el2 = sysreg_cntvoff_el2_read();
    vcpu->arch.sysregs.vm.vbar_el1 = sysreg_vbar_el1_read();
    vcpu->arch.sysregs.vm.tpidr_el1 = sysreg_tpidr_el1_read();
    vcpu->arch.sysregs.vm.mair_el1 = sysreg_mair_el1_read();
    vcpu->arch.sysregs.vm.amair_el1 = sysreg_amair_el1_read();
    vcpu->arch.sysregs.vm.tcr_el1 = sysreg_tcr_el1_read();
    vcpu->arch.sysregs.vm.ttbr0_el1 = sysreg_ttbr0_el1_read();
    vcpu->arch.sysregs.vm.ttbr1_el1 = sysreg_ttbr1_el1_read();
    vcpu->arch.sysregs.vm.sp_el0 = sysreg_sp_el0_read();
    vcpu->arch.sysregs.vm.sp_el1 = sysreg_sp_el1_read();
    vcpu->arch.sysregs.vm.spsr_el1 = sysreg_spsr_el1_read();
    vcpu->arch.sysregs.vm.sctlr_el1 = sysreg_sctlr_el1_read();
    vcpu->arch.sysregs.vm.actlr_el1 = sysreg_actlr_el1_read();
    vcpu->arch.sysregs.vm.par_el1 = sysreg_par_el1_read();
    vcpu->arch.sysregs.vm.far_el1 = sysreg_far_el1_read();
    vcpu->arch.sysregs.vm.esr_el1 = sysreg_esr_el1_read();
    vcpu->arch.sysregs.vm.elr_el1 = sysreg_elr_el1_read();
    vcpu->arch.sysregs.vm.afsr0_el1 = sysreg_afsr0_el1_read();
    vcpu->arch.sysregs.vm.afsr1_el1 = sysreg_afsr1_el1_read();
    vcpu->arch.sysregs.vm.tpidrro_el0 = sysreg_tpidrro_el0_read();
    vcpu->arch.sysregs.vm.tpidr_el0 = sysreg_tpidr_el0_read();
    vcpu->arch.sysregs.vm.cntv_ctl_el0 = sysreg_cntv_ctl_el0_read();
    vcpu->arch.sysregs.vm.cntv_cval_el0 = sysreg_cntv_cval_el0_read();
    vcpu->arch.sysregs.vm.cntkctl_el1 = sysreg_cntkctl_el1_read();

    vcpu_arch_profile_save_state(vcpu);
    vfp_save_state(&vcpu->regs.vfp);
    vtimer_save_state(vcpu);
    vgic_save_state(vcpu);
}
