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

#ifdef MEM_PROT_MPU
    spmp_init(&vcpu->arch.spmp, PRIV_VM);
#endif
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    memset(&vcpu->regs, 0, sizeof(struct arch_regs));

    // csrs_sscratch_write(cpu());

    vcpu->regs.hstatus = HSTATUS_SPV | (1ULL << HSTATUS_VGEIN_OFF);
    vcpu->regs.sstatus = SSTATUS_SPP_BIT | SSTATUS_FS_DIRTY | SSTATUS_XS_DIRTY;
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
    vcpu->regs.hie = 0;
    vcpu->regs.vstimecmp = ~0U;

    vfp_reset(&vcpu->regs.vfp);

    if (CPU_HAS_EXTENSION(CPU_EXT_SSTC)) {
        csrs_stimecmp_write(~0U);
        csrs_henvcfg_set(HENVCFG_STCE);
    } else {
        csrs_henvcfg_clear(HENVCFG_STCE);
    }

    csrs_hcounteren_write(HCOUNTEREN_TM);
    csrs_htimedelta_write(0);
    csrs_vsstatus_write(SSTATUS_SD | SSTATUS_FS_DIRTY | SSTATUS_XS_DIRTY);
    csrs_hie_write(0);
    csrs_vstvec_write(0);
    csrs_vsscratch_write(0);
    csrs_vsepc_write(0);
    csrs_vscause_write(0);
    csrs_vstval_write(0);
    csrs_hvip_write(0);
    csrs_vsatp_write(0);
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

void vcpu_arch_run(struct vcpu* vcpu)
{
    if (vcpu->arch.sbi_ctx.state == STARTED) {
        vcpu_arch_entry();
    } else {
        cpu_idle();
    }
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
    if (CPU_HAS_EXTENSION(CPU_EXT_SSTC)) {
        csrs_vstimecmp_write(vcpu->regs.vstimecmp);
    }

    csrs_hie_write(vcpu->regs.hie);
    csrs_hvip_write(vcpu->regs.hvip);
    csrs_hgatp_write(vcpu->vm->arch.hgatp);

    timer_event_add(&vcpu->arch.timer_event);
    vfp_restore_state(&vcpu->regs.vfp);

#ifdef MEM_PROT_MPU
    spmp_set_active(&vcpu->arch.spmp, true);
    spmp_restore(&vcpu->arch.spmp);
#endif
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
    if (CPU_HAS_EXTENSION(CPU_EXT_SSTC)) {
        vcpu->regs.vstimecmp = csrs_vstimecmp_read();
    }

    vcpu->regs.hie = csrs_hie_read();
    vcpu->regs.hvip = csrs_hvip_read();

    timer_event_remove(&vcpu->arch.timer_event);
    vfp_save_state(&vcpu->regs.vfp);
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    return vcpu->arch.sbi_ctx.state == STARTED;
}
