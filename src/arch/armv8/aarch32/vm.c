/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/sysregs.h>

// For the following functions which access state from different modes, for now
// we assume vcpu passed as argument is resident

static inline uint32_t vcpu_current_spsr_m(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);

    return sysreg_spsr_hyp_read() & SPSR_M_MSK;
}

static inline uint32_t vcpu_read_sp_current_el1_mode(struct vcpu* vcpu)
{
    uint32_t spsr_m = vcpu_current_spsr_m(vcpu);
    uint32_t sp_val = 0;
    switch (spsr_m) {
        case SPSR_USR:
        case SPSR_SYS:
            sp_val = sysreg_sp_usr_read();
            break;
        case SPSR_IRQ:
            sp_val = sysreg_sp_irq_read();
            break;
        case SPSR_FIQ:
            sp_val = sysreg_sp_fiq_read();
            break;
        case SPSR_SVC:
            sp_val = sysreg_sp_svc_read();
            break;
        case SPSR_ABT:
            sp_val = sysreg_sp_abt_read();
            break;
        case SPSR_UND:
            sp_val = sysreg_sp_und_read();
            break;
        default:
            ERROR("Got reserved mode from SPSR_HYP.M");
    }

    return sp_val;
}

static inline void vcpu_write_sp_current_el1_mode(struct vcpu* vcpu, uint32_t sp_val)
{
    uint32_t spsr_m = vcpu_current_spsr_m(vcpu);
    switch (spsr_m) {
        case SPSR_USR:
        case SPSR_SYS:
            sysreg_sp_usr_write(sp_val);
            break;
        case SPSR_IRQ:
            sysreg_sp_irq_write(sp_val);
            break;
        case SPSR_FIQ:
            sysreg_sp_fiq_write(sp_val);
            break;
        case SPSR_SVC:
            sysreg_sp_svc_write(sp_val);
            break;
        case SPSR_ABT:
            sysreg_sp_abt_write(sp_val);
            break;
        case SPSR_UND:
            sysreg_sp_und_write(sp_val);
            break;
        default:
            ERROR("Got reserved mode from SPSR_HYP.M");
    }
}

static inline uint32_t vcpu_read_lr_current_el1_mode(struct vcpu* vcpu)
{
    uint32_t spsr_m = vcpu_current_spsr_m(vcpu);
    uint32_t lr_val = 0;
    switch (spsr_m) {
        case SPSR_USR:
        case SPSR_SYS:
            /*
             * In Hyp mode r14 aliases LR_usr.  The exception entry assembly
             * saved the guest value before any BL changed Hyp's return
             * address, so never access LR_usr from an ordinary C function.
             */
            lr_val = vcpu->regs.x[14];
            break;
        case SPSR_IRQ:
            lr_val = sysreg_lr_irq_read();
            break;
        case SPSR_FIQ:
            lr_val = sysreg_lr_fiq_read();
            break;
        case SPSR_SVC:
            lr_val = sysreg_lr_svc_read();
            break;
        case SPSR_ABT:
            lr_val = sysreg_lr_abt_read();
            break;
        case SPSR_UND:
            lr_val = sysreg_lr_und_read();
            break;
        default:
            ERROR("Got reserved mode from SPSR_HYP.M");
    }

    return lr_val;
}

static inline void vcpu_write_lr_current_el1_mode(struct vcpu* vcpu, uint32_t lr_val)
{
    uint32_t spsr_m = vcpu_current_spsr_m(vcpu);
    switch (spsr_m) {
        case SPSR_USR:
        case SPSR_SYS:
            vcpu->regs.x[14] = lr_val;
            break;
        case SPSR_IRQ:
            sysreg_lr_irq_write(lr_val);
            break;
        case SPSR_FIQ:
            sysreg_lr_fiq_write(lr_val);
            break;
        case SPSR_SVC:
            sysreg_lr_svc_write(lr_val);
            break;
        case SPSR_ABT:
            sysreg_lr_abt_write(lr_val);
            break;
        case SPSR_UND:
            sysreg_lr_und_write(lr_val);
            break;
        default:
            ERROR("Got reserved mode from SPSR_HYP.M");
    }
}

static inline uint32_t vcpu_read_current_el1_fiq_rx(struct vcpu* vcpu, unsigned long reg)
{
    UNUSED_ARG(vcpu);

    uint32_t reg_val = 0;
    switch (reg) {
        case 8:
            reg_val = sysreg_r8_fiq_read();
            break;
        case 9:
            reg_val = sysreg_r9_fiq_read();
            break;
        case 10:
            reg_val = sysreg_r10_fiq_read();
            break;
        case 11:
            reg_val = sysreg_r11_fiq_read();
            break;
        case 12:
            reg_val = sysreg_r12_fiq_read();
            break;
        default:
            reg_val = 0;
    }

    return reg_val;
}

static inline uint32_t vcpu_write_current_el1_fiq_rx(struct vcpu* vcpu, unsigned long reg,
    unsigned long reg_val)
{
    UNUSED_ARG(vcpu);

    switch (reg) {
        case 8:
            sysreg_r8_fiq_write(reg_val);
            break;
        case 9:
            sysreg_r9_fiq_write(reg_val);
            break;
        case 10:
            sysreg_r10_fiq_write(reg_val);
            break;
        case 11:
            sysreg_r11_fiq_write(reg_val);
            break;
        case 12:
            sysreg_r12_fiq_write(reg_val);
            break;
        default:
            reg_val = 0;
    }

    return reg_val;
}

unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg)
{
    uint32_t reg_val = 0;

    if (reg > 14) {
        reg_val = 0;
    } else if (reg == 13) {
        reg_val = vcpu_read_sp_current_el1_mode(vcpu);
    } else if (reg == 14) {
        reg_val = vcpu_read_lr_current_el1_mode(vcpu);
    } else if ((reg >= 8) && (vcpu_current_spsr_m(vcpu) == SPSR_FIQ)) {
        reg_val = vcpu_read_current_el1_fiq_rx(vcpu, reg);
    } else {
        reg_val = vcpu->regs.x[reg];
    }

    return reg_val;
}

void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val)
{
    if (reg > 14) {
        return;
    } else if (reg == 13) {
        vcpu_write_sp_current_el1_mode(vcpu, val);
    } else if (reg == 14) {
        vcpu_write_lr_current_el1_mode(vcpu, val);
    } else if ((reg >= 8) && (vcpu_current_spsr_m(vcpu) == SPSR_FIQ)) {
        vcpu_write_current_el1_fiq_rx(vcpu, reg, val);
    } else {
        vcpu->regs.x[reg] = val;
    }
}

unsigned long vcpu_readpc(struct vcpu* vcpu)
{
    return vcpu->regs.elr_hyp;
}

void vcpu_writepc(struct vcpu* vcpu, unsigned long pc)
{
    vcpu->regs.elr_hyp = pc;
}

void vcpu_subarch_reset(struct vcpu* vcpu)
{
    vcpu->regs.spsr_hyp = SPSR_SVC | SPSR_F | SPSR_I | SPSR_A;
    vcpu->arch.banked = (struct arch_banked_regs){ 0 };
    vcpu->arch.fault = (struct arch_fault_regs){ 0 };
}

static void vcpu_save_banked_state(struct vcpu* vcpu)
{
     struct arch_banked_regs* regs = &vcpu->arch.banked;

     regs->sp_usr = sysreg_sp_usr_read();

    regs->sp_irq = sysreg_sp_irq_read();
    regs->lr_irq = sysreg_lr_irq_read();
    regs->spsr_irq = sysreg_spsr_irq_read();

    regs->sp_fiq = sysreg_sp_fiq_read();
    regs->lr_fiq = sysreg_lr_fiq_read();
    regs->spsr_fiq = sysreg_spsr_fiq_read();
    regs->r8_fiq = sysreg_r8_fiq_read();
    regs->r9_fiq = sysreg_r9_fiq_read();
    regs->r10_fiq = sysreg_r10_fiq_read();
    regs->r11_fiq = sysreg_r11_fiq_read();
    regs->r12_fiq = sysreg_r12_fiq_read();

    regs->sp_svc = sysreg_sp_svc_read();
    regs->lr_svc = sysreg_lr_svc_read();
    regs->spsr_svc = sysreg_spsr_svc_read();

    regs->sp_abt = sysreg_sp_abt_read();
    regs->lr_abt = sysreg_lr_abt_read();
    regs->spsr_abt = sysreg_spsr_abt_read();

    regs->sp_und = sysreg_sp_und_read();
    regs->lr_und = sysreg_lr_und_read();
    regs->spsr_und = sysreg_spsr_und_read();
}

static void vcpu_restore_banked_state(struct vcpu* vcpu)
{
     const struct arch_banked_regs* regs = &vcpu->arch.banked;

     sysreg_sp_usr_write(regs->sp_usr);

    sysreg_sp_irq_write(regs->sp_irq);
    sysreg_lr_irq_write(regs->lr_irq);
    sysreg_spsr_irq_write(regs->spsr_irq);

    sysreg_sp_fiq_write(regs->sp_fiq);
    sysreg_lr_fiq_write(regs->lr_fiq);
    sysreg_spsr_fiq_write(regs->spsr_fiq);
    sysreg_r8_fiq_write(regs->r8_fiq);
    sysreg_r9_fiq_write(regs->r9_fiq);
    sysreg_r10_fiq_write(regs->r10_fiq);
    sysreg_r11_fiq_write(regs->r11_fiq);
    sysreg_r12_fiq_write(regs->r12_fiq);

    sysreg_sp_svc_write(regs->sp_svc);
    sysreg_lr_svc_write(regs->lr_svc);
    sysreg_spsr_svc_write(regs->spsr_svc);

    sysreg_sp_abt_write(regs->sp_abt);
    sysreg_lr_abt_write(regs->lr_abt);
    sysreg_spsr_abt_write(regs->spsr_abt);

    sysreg_sp_und_write(regs->sp_und);
    sysreg_lr_und_write(regs->lr_und);
    sysreg_spsr_und_write(regs->spsr_und);
}

void vcpu_save_state(struct vcpu* vcpu)
{
    if (vcpu == NULL) {
        return;
    }

    vcpu->arch.sysregs.hyp.vmpidr_el2 = sysreg_vmpidr_el2_read();
    vcpu->arch.sysregs.hyp.cntvoff_el2 = sysreg_cntvoff_el2_read();
    vcpu->arch.sysregs.vm.vbar_el1 = sysreg_vbar_el1_read();
    vcpu->arch.sysregs.vm.tpidr_el1 = sysreg_tpidr_el1_read();
    vcpu->arch.sysregs.vm.tpidr_el0 = sysreg_tpidr_el0_read();
    vcpu->arch.sysregs.vm.tpidrro_el0 = sysreg_tpidrro_el0_read();
    vcpu->arch.sysregs.vm.mair_el1 = sysreg_mair_el1_read();
    vcpu->arch.sysregs.vm.actlr_el1 = sysreg_actlr_el1_read();
    vcpu->arch.sysregs.vm.par_el1 = sysreg_par_el1_read();
    vcpu->arch.sysregs.vm.cntv_ctl_el0 = sysreg_cntv_ctl_el0_read();
    vcpu->arch.sysregs.vm.cntv_cval_el0 = sysreg_cntv_cval_el0_read();
    vcpu->arch.sysregs.vm.cpacr_el1 = sysreg_cpacr_el1_read();
    vcpu->arch.sysregs.vm.contextidr_el1 = sysreg_contextidr_el1_read();
    vcpu->arch.sysregs.vm.csselr_el1 = sysreg_csselr_el1_read();
    vcpu->arch.fault.dfsr = sysreg_dfsr_el1_read();
    vcpu->arch.fault.ifsr = sysreg_ifsr_el1_read();
    vcpu->arch.fault.adfsr = sysreg_adfsr_el1_read();
    vcpu->arch.fault.aifsr = sysreg_aifsr_el1_read();
    vcpu->arch.fault.dfar = sysreg_dfar_el1_read();
    vcpu->arch.fault.ifar = sysreg_ifar_el1_read();

    vcpu_save_banked_state(vcpu);
    vcpu->arch.sysregs.vm.sctlr_el1 = sysreg_sctlr_el1_read();
    vcpu->arch.sysregs.vm.cntkctl_el1 = sysreg_cntkctl_el1_read();
    vcpu_arch_profile_save_state(vcpu);
    vgic_save_state(vcpu);
}

void vcpu_restore_state(struct vcpu* vcpu)
{
    if (vcpu == NULL) {
        return;
    }

    sysreg_vmpidr_el2_write((unsigned long)vcpu->arch.sysregs.hyp.vmpidr_el2);
    sysreg_cntvoff_el2_write(vcpu->arch.sysregs.hyp.cntvoff_el2);
    sysreg_vbar_el1_write((unsigned long)vcpu->arch.sysregs.vm.vbar_el1);
    sysreg_tpidr_el1_write((unsigned long)vcpu->arch.sysregs.vm.tpidr_el1);
    sysreg_tpidr_el0_write((unsigned long)vcpu->arch.sysregs.vm.tpidr_el0);
    sysreg_tpidrro_el0_write((unsigned long)vcpu->arch.sysregs.vm.tpidrro_el0);
    sysreg_mair_el1_write(vcpu->arch.sysregs.vm.mair_el1);
    sysreg_actlr_el1_write((unsigned long)vcpu->arch.sysregs.vm.actlr_el1);
    sysreg_par_el1_write(vcpu->arch.sysregs.vm.par_el1);
    sysreg_cntv_ctl_el0_write((unsigned long)vcpu->arch.sysregs.vm.cntv_ctl_el0);
    sysreg_cntv_cval_el0_write(vcpu->arch.sysregs.vm.cntv_cval_el0);
    sysreg_cpacr_el1_write((unsigned long)vcpu->arch.sysregs.vm.cpacr_el1);
    sysreg_contextidr_el1_write((unsigned long)vcpu->arch.sysregs.vm.contextidr_el1);
    sysreg_csselr_el1_write((unsigned long)vcpu->arch.sysregs.vm.csselr_el1);

    sysreg_dfsr_el1_write(vcpu->arch.fault.dfsr);
    sysreg_ifsr_el1_write(vcpu->arch.fault.ifsr);
    sysreg_adfsr_el1_write(vcpu->arch.fault.adfsr);
    sysreg_aifsr_el1_write(vcpu->arch.fault.aifsr);
    sysreg_dfar_el1_write(vcpu->arch.fault.dfar);
    sysreg_ifar_el1_write(vcpu->arch.fault.ifar);

    vcpu_restore_banked_state(vcpu);
    sysreg_sctlr_el1_write((unsigned long)vcpu->arch.sysregs.vm.sctlr_el1);
    sysreg_cntkctl_el1_write((unsigned long)vcpu->arch.sysregs.vm.cntkctl_el1);
    vcpu_arch_profile_restore_state(vcpu);
    vgic_restore_state(vcpu);
}
