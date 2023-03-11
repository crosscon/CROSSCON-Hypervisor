/**
 * Bao Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <vm.h>
#include <page_table.h>
#include <arch/csrs.h>
#include <arch/vplic.h>
#include <arch/instructions.h>
#include <string.h>

void vm_arch_init(struct vm *vm, const struct vm_config *config)
{
    paddr_t root_pt_pa;
    mem_translate(&cpu.as, (vaddr_t)vm->as.pt.root, &root_pt_pa);

    unsigned long hgatp = (root_pt_pa >> PAGE_SHIFT) | (HGATP_MODE_DFLT) |
                          ((vm->id << HGATP_VMID_OFF) & HGATP_VMID_MSK);

    vm->arch.hgatp = hgatp;

    vplic_init(vm, platform.arch.plic_base);
}

void vcpu_arch_init(struct vcpu *vcpu, struct vm *vm) {
    vcpu->arch.sbi_ctx.lock = SPINLOCK_INITVAL;
    vcpu->arch.sbi_ctx.state = vcpu->id == 0 ?  STARTED : STOPPED;
}

void vcpu_arch_reset(struct vcpu *vcpu, vaddr_t entry)
{
    memset(vcpu->regs, 0, sizeof(struct arch_regs));

    vcpu->regs->hstatus = HSTATUS_SPV | HSTATUS_VSXL_64;
    vcpu->regs->sstatus= SSTATUS_SPP_BIT | SSTATUS_FS_DIRTY | SSTATUS_XS_DIRTY;
    vcpu->regs->sepc = entry;
    vcpu->regs->a0 = vcpu->arch.hart_id = vcpu->id;
    vcpu->regs->a1 = 0;  // according to sbi it should be the dtb load address

    CSRW(CSR_HCOUNTEREN, HCOUNTEREN_TM);
    CSRW(CSR_HTIMEDELTA, 0);
    CSRW(CSR_VSSTATUS, SSTATUS_SD | SSTATUS_FS_DIRTY | SSTATUS_XS_DIRTY);
    CSRW(CSR_HIE, 0);
    CSRW(CSR_VSTVEC, 0);
    CSRW(CSR_VSSCRATCH, 0);
    CSRW(CSR_VSEPC, 0);
    CSRW(CSR_VSCAUSE, 0);
    CSRW(CSR_VSTVAL, 0);
    CSRW(CSR_HVIP, 0);
    CSRW(CSR_VSATP, 0);
}

unsigned long vcpu_readreg(struct vcpu *vcpu, unsigned long reg)
{
    if ((reg <= 0) || (reg > 31)) return 0;
    return vcpu->regs->x[reg - 1];
}

void vcpu_writereg(struct vcpu *vcpu, unsigned long reg, unsigned long val)
{
    if ((reg <= 0) || (reg > 31)) return;
    vcpu->regs->x[reg - 1] = val;
}

unsigned long vcpu_readpc(struct vcpu *vcpu)
{
    return vcpu->regs->sepc;
}

void vcpu_writepc(struct vcpu *vcpu, unsigned long pc)
{
    vcpu->regs->sepc = pc;
}

int vcpu_is_off(struct vcpu* vcpu)
{
    return cpu.vcpu->arch.sbi_ctx.state == STOPPED;
}

void vcpu_arch_run(struct vcpu *vcpu){

    if(vcpu->arch.sbi_ctx.state == STARTED){
        vcpu_arch_entry();
    } else {
        cpu_idle();
    }    

}

void vcpu_save_state(struct vcpu* vcpu){
    if(vcpu == NULL) return;

    vcpu->regs->hstatus = CSRR(CSR_HSTATUS);
    vcpu->regs->sstatus= CSRR(sstatus);
    vcpu->regs->sepc    = CSRR(sepc);

    vcpu->regs->vsstatus = CSRR(CSR_VSSTATUS);
    vcpu->regs->vstvec = CSRR(CSR_VSTVEC);
    vcpu->regs->vsscratch = CSRR(CSR_VSSCRATCH);
    vcpu->regs->vsepc = CSRR(CSR_VSEPC);
    vcpu->regs->vscause = CSRR(CSR_VSCAUSE);
    vcpu->regs->vsatp = CSRR(CSR_VSATP);

    vcpu->regs->vstval = CSRR(CSR_VSTVAL);
    vcpu->regs->hvip = CSRR(CSR_HVIP);
    vcpu->regs->hie = CSRR(CSR_HIE);
    
    /* vgic_save_state(vcpu); */
    /* vtimer_save_state(vcpu); */
}

void vcpu_restore_state(struct vcpu* vcpu){
    if(vcpu == NULL) return;

    // hstatus, sstatus ans sepc are restored in vcpu_arch_entry

    CSRW(CSR_VSSTATUS, vcpu->regs->vsstatus);
    CSRW(CSR_VSTVEC, vcpu->regs->vstvec);
    CSRW(CSR_VSSCRATCH, vcpu->regs->vsscratch);
    CSRW(CSR_VSEPC, vcpu->regs->vsepc);
    CSRW(CSR_VSCAUSE, vcpu->regs->vscause);
    CSRW(CSR_VSTVAL, vcpu->regs->vstval);
    CSRW(CSR_VSATP, vcpu->regs->vsatp);
    CSRW(CSR_HVIP, vcpu->regs->hvip);
    CSRW(CSR_HIE, vcpu->regs->hie);

    CSRW(CSR_HGATP, vcpu->vm->arch.hgatp);
}
