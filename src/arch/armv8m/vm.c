/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/sysregs.h>
#include <fences.h>
#include <string.h>
#include <config.h>
#include <list.h>
#include <platform.h>
#include <arch/vtimer.h>
#include <arch/vmpu.h>
#include <arch/sau.h>
#include <arch/vnvic.h>
#include <mem.h>

typedef enum regs { r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc } regs_t;

void vm_arch_init(struct vm* vm, const struct vm_config* vm_config)
{
    UNUSED_ARG(vm);
    UNUSED_ARG(vm_config);
}

void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm)
{
    UNUSED_ARG(vcpu);
    UNUSED_ARG(vm);

    vcpu->first_run = 0x0;
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    // TODO:ARMV8-M - to we need replicate the warm reset sequence? (TakeReset - page 1919)
    // msp = vector[0]
    // If so, we do this:
    // Set gp regs to reset values (except regs from exception stack frame r0-r3, r12, sp, lr, pc)
    memset(&vcpu->regs.gp_regs, 0, sizeof(((struct arch_regs*)NULL)->gp_regs));
    // Set PC to entry
    vcpu->regs.gp_regs.pc = entry;

    // Set sp regs to reset values (except regs from exception stack frame xPSR, msp)
    memset(&vcpu->regs.sp_regs, 0, sizeof(struct special_regs));

    vfp_reset(&vcpu->regs.vfp_regs);
    vnvic_reset();
    vtimer_reset(&vcpu->arch.vtimer);
    vmpu_reset(&vcpu->arch.vmpu);
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);

    return true;
}

unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg)
{
    if ((reg <= r0) || (reg > MAX_OF_GP_REGS)) {
        return 0;
    }
    switch (reg) {
        case r0:
        case r1:
        case r2:
        case r3:
            return vcpu->regs.esf_regs.r[reg];
        case r12:
            return vcpu->regs.esf_regs.r12;
        case lr:
            return vcpu->regs.gp_regs.lr;
        case pc:
            return vcpu->regs.gp_regs.pc;
        default:
            return vcpu->regs.gp_regs.r[reg - r4];
    }
}

void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val)
{
    if ((reg <= 0) || (reg > MAX_OF_GP_REGS)) {
        return;
    }
    switch (reg) {
        case r0:
        case r1:
        case r2:
        case r3:
            vcpu->regs.esf_regs.r[reg] = val;
            break;
        case r12:
            vcpu->regs.esf_regs.r12 = val;
            break;
        case lr:
            vcpu->regs.gp_regs.lr = val;
            break;
        case pc:
            vcpu->regs.gp_regs.pc = val;
            break;
        default:
            vcpu->regs.gp_regs.r[reg - r4] = val;
            break;
    }
    return;
}

unsigned long vcpu_readpc(struct vcpu* vcpu)
{
    return vcpu->regs.gp_regs.pc;
}

void vcpu_writepc(struct vcpu* vcpu, unsigned long reg)
{
    vcpu->regs.gp_regs.pc = reg;
}

void vcpu_restore_state(struct vcpu* vcpu)
{
    scb_ns->icsr = vcpu->regs.icsr;
    scb_ns->vtor = vcpu->regs.vtor;
    scb_ns->aircr = vcpu->regs.aircr;
    scb_ns->scr = vcpu->regs.scr;
    scb_ns->ccr = vcpu->regs.ccr;
    scb_ns->shpr1 = vcpu->regs.shpr1;
    scb_ns->shpr2 = vcpu->regs.shpr2;
    scb_ns->shpr3 = vcpu->regs.shpr3;
    scb_ns->shcsr = vcpu->regs.shcsr;
    scb_ns->mmfar = vcpu->regs.mmfar;
    scb_ns->bfar = vcpu->regs.bfar;
    scb_ns->csselr = vcpu->regs.csselr;
    scb_ns->cpacr = vcpu->regs.cpacr;
    dcb_ns->dhcsr = vcpu->regs.dhcsr;
    dcb_ns->dcrdr = vcpu->regs.dcrdr;
    dcb_ns->demcr = vcpu->regs.demcr;
    dcb_ns->dauthctrl = vcpu->regs.dauthctrl;

    vnvic_restore_state(&vcpu->arch.vnvic, vcpu->vm->interrupt_bitmap);

    vmpu_restore_state(&vcpu->arch.vmpu);
    // vfp_restore_state(&vcpu->regs.vfp_regs);
    vtimer_restore_state(vcpu, &vcpu->arch.vtimer);
    sau_restore(&vcpu->arch.sau_vm);
}

void vcpu_save_state(struct vcpu* vcpu)
{
    vtimer_save_state(&vcpu->arch.vtimer);

    vcpu->regs.icsr = scb_ns->icsr;
    vcpu->regs.vtor = scb_ns->vtor;
    vcpu->regs.aircr = scb_ns->aircr;
    vcpu->regs.scr = scb_ns->scr;
    vcpu->regs.ccr = scb_ns->ccr;
    vcpu->regs.shpr1 = scb_ns->shpr1;
    vcpu->regs.shpr2 = scb_ns->shpr2;
    vcpu->regs.shpr3 = scb_ns->shpr3;
    vcpu->regs.shcsr = scb_ns->shcsr;
    vcpu->regs.mmfar = scb_ns->mmfar;
    vcpu->regs.bfar = scb_ns->bfar;
    vcpu->regs.csselr = scb_ns->csselr;
    vcpu->regs.cpacr = scb_ns->cpacr;
    vcpu->regs.dhcsr = dcb_ns->dhcsr;
    vcpu->regs.dcrdr = dcb_ns->dcrdr;
    vcpu->regs.demcr = dcb_ns->demcr;
    vcpu->regs.dauthctrl = dcb_ns->dauthctrl;

    vnvic_save_state(&vcpu->arch.vnvic, vcpu->vm->interrupt_bitmap);
    vmpu_save_state(&vcpu->arch.vmpu);

    // vfp_save_state(&vcpu->regs.vfp_regs);
}

bool vm_nvic_act_irq(void)
{
    return nvic_any_act_irq(nvic_ns);
}
