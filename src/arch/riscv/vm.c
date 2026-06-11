/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <page_table.h>
#include <arch/csrs.h>
#include <irqc.h>
#include <arch/instructions.h>
#include <string.h>
#include <config.h>

void vm_arch_init(struct vm* vm, const struct vm_config* vm_config)
{
#ifdef MEM_PROT_MMU
    paddr_t root_pt_pa;
    mem_translate(&cpu()->as, (vaddr_t)vm->as.pt.root, &root_pt_pa);

    unsigned long hgatp = (root_pt_pa >> PAGE_SHIFT) | (HGATP_MODE_DFLT) |
        ((vm->id << HGATP_VMID_OFF) & HGATP_VMID_MSK);

    vm->arch.hgatp = hgatp;
#endif

    vm_arch_mem_prot_init(vm);
    virqc_init(vm, &vm_config->platform.arch.irqc);
}

void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm)
{
    UNUSED_ARG(vm);

    vcpu->arch.sbi_ctx.lock = SPINLOCK_INITVAL;
    vcpu->arch.sbi_ctx.state = vcpu->id == 0 ? STARTED : STOPPED;

#if (IRQC == AIA)
    ssize_t guest_int_file = imsic_alloc_guest_int_file();
    if (guest_int_file < 0) {
        ERROR("Not enough IMSIC guest interrupt files available.\n");
    }
    vcpu->arch.imsic_guest_file_index = (size_t)guest_int_file;
#endif
    vcpu_arch_mem_prot_init(vcpu);
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    memset(&vcpu->regs, 0, sizeof(struct arch_regs));

    vcpu->regs.hstatus = HSTATUS_SPV;

    #if defined (RV64) 
        vcpu->regs.hstatus |= HSTATUS_VSXL_64;
    #endif

#if (IRQC == AIA)
    vcpu->regs.hstatus |= ((unsigned long)vcpu->arch.imsic_guest_file_index) << HSTATUS_VGEIN_OFF;
#endif

    vcpu->regs.sstatus = SSTATUS_SPP_BIT | SSTATUS_FS_INITIAL;
    vcpu->regs.sepc = entry;
    vcpu->regs.a0 = vcpu->arch.hart_id = vcpu->id;
    vcpu->regs.a1 = 0; // according to sbi it should be the dtb load address

    vcpu->regs.vsstatus = SSTATUS_SD | SSTATUS_FS_DIRTY | SSTATUS_XS_DIRTY;
    vcpu->regs.vstvec = 0;
    vcpu->regs.vsscratch = 0;
    vcpu->regs.vsepc = 0;
    vcpu->regs.vscause = 0;
    vcpu->regs.vstval = 0;
    vcpu->regs.vsatp = 0;
    vcpu->regs.hvip = 0;
    vcpu->regs.hie = 0;
    vcpu->regs.vstimecmp = ~0UL;

    if (CPU_HAS_EXTENSION(CPU_EXT_SSCSRIND)) {
        vcpu->regs.vsiselect = 0;
    }

    vcpu_arch_mem_prot_reset(vcpu);

#if !CPU_HAS_EXTENSION(CPU_EXT_SSTC)
    timer_event_remove(&vcpu->arch.timer_event);
    vcpu->arch.timer_event.timer = ~0ULL;
#endif

    vfpu_reset(vcpu);
}

unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg)
{
    if ((reg <= 0) || (reg > 31)) {
        return 0;
    }
    return vcpu->regs.x[reg - 1];
}

void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val)
{
    if ((reg <= 0) || (reg > 31)) {
        return;
    }
    vcpu->regs.x[reg - 1] = val;
}

unsigned long vcpu_readpc(struct vcpu* vcpu)
{
    return vcpu->regs.sepc;
}

void vcpu_writepc(struct vcpu* vcpu, unsigned long pc)
{
    vcpu->regs.sepc = pc;
}

void vcpu_restore_state(struct vcpu* vcpu)
{
    csrs_vsstatus_write(vcpu->regs.vsstatus);
    csrs_vstvec_write(vcpu->regs.vstvec);
    csrs_vsscratch_write(vcpu->regs.vsscratch);
    csrs_vsepc_write(vcpu->regs.vsepc);
    csrs_vscause_write(vcpu->regs.vscause);
    csrs_vstval_write(vcpu->regs.vstval);
    csrs_vsatp_write(vcpu->regs.vsatp);

    csrs_hie_write(vcpu->regs.hie);
    csrs_hvip_write(vcpu->regs.hvip);
    csrs_hgatp_write(vcpu->vm->arch.hgatp);

    vcpu_arch_mem_prot_restore_state(vcpu);

#if CPU_HAS_EXTENSION(CPU_EXT_SSTC)
    csrs_vstimecmp_write(vcpu->regs.vstimecmp);
#else
    timer_event_add(&vcpu->arch.timer_event);
#endif

    vfpu_restore_state(vcpu);

    if (CPU_HAS_EXTENSION(CPU_EXT_SSCSRIND)) {
        csrs_vsiselect_write(vcpu->regs.vsiselect);
    }
}

void vcpu_save_state(struct vcpu* vcpu)
{
    vcpu->regs.vsstatus = csrs_vsstatus_read();
    vcpu->regs.vstvec = csrs_vstvec_read();
    vcpu->regs.vsscratch = csrs_vsscratch_read();
    vcpu->regs.vsepc = csrs_vsepc_read();
    vcpu->regs.vscause = csrs_vscause_read();
    vcpu->regs.vstval = csrs_vstval_read();
    vcpu->regs.vsatp = csrs_vsatp_read();

    vcpu->regs.hie = csrs_hie_read();
    vcpu->regs.hvip = csrs_hvip_read();

    vcpu_arch_mem_prot_save_state(vcpu);

#if CPU_HAS_EXTENSION(CPU_EXT_SSTC)
    vcpu->regs.vstimecmp = csrs_vstimecmp_read();
#else
    timer_event_remove(&vcpu->arch.timer_event);
#endif

    vfpu_save_state(vcpu);

    if (CPU_HAS_EXTENSION(CPU_EXT_SSCSRIND)) {
        vcpu->regs.vsiselect = csrs_vsiselect_read();
    }
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    return vcpu->arch.sbi_ctx.state == STARTED;
}
