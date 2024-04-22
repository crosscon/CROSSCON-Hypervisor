/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __ARCH_VM_H__
#define __ARCH_VM_H__

#include <crossconhyp.h>
#include <arch/vgic.h>
#include <arch/psci.h>
#include <list.h>

struct vm_arch {
    struct vgicd vgicd;
    vaddr_t vgicr_addr;
    struct list vgic_spilled;
    spinlock_t vgic_spilled_lock;
};

struct vcpu_arch {
    unsigned long vmpidr;
    struct vgic_priv vgic_priv;
    struct list vgic_spilled;
    struct psci_ctx psci_ctx;
    struct {

        struct {
            uint64_t elr_el2;
            uint64_t spsr_el2;
            uint64_t vttbr_el2;
            uint64_t vmpidr_el2;
            uint64_t cntvoff_el2;
        } hyp;

        struct {
            uint64_t vbar_el1;
            uint64_t tpidr_el1;
            uint64_t mair_el1;
            uint64_t amair_el1;
            uint64_t tcr_el1;
            uint64_t ttbr0_el1;
            uint64_t ttbr1_el1;
            uint64_t sp_el0;
            uint64_t sp_el1;
            uint64_t spsr_el1;
            uint64_t sctlr_el1;
            uint64_t actlr_el1;
            uint64_t par_el1;
            uint64_t far_el1;
            uint64_t esr_el1;
            uint64_t elr_el1;
            uint64_t afsr0_el1;
            uint64_t afsr1_el1;
            uint64_t tpidrro_el0;
            uint64_t tpidr_el0;
            uint64_t cntv_ctl_el0;
            uint64_t cntv_cval_el0;
            uint64_t cntkctl_el1;
        } vm;

    } sysregs;
};

struct arch_regs {
    uint64_t x[31];
    uint64_t elr_el2;
    uint64_t spsr_el2;
} __attribute__((aligned(16)));  // makes size always aligned to 16 to respect
                                 // stack alignment

struct vcpu* vm_get_vcpu_by_mpidr(struct vm* vm, unsigned long mpidr);
void vcpu_arch_entry();


static inline void vcpu_arch_inject_hw_irq(struct vcpu* vcpu, uint64_t id)
{
    vgic_inject_hw(vcpu, id);
}

static inline void vcpu_arch_inject_irq(struct vcpu* vcpu, uint64_t id)
{
    vgic_inject(vcpu, id, 0);
}

#endif /* __ARCH_VM_H__ */
