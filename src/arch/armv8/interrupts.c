/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <interrupts.h>

#include <cpu.h>
#include <platform.h>
#include <arch/gic.h>
#include <mem.h>
#include <arch/sysregs.h>
#include <vm.h>

#ifndef GIC_VERSION
#error "GIC_VERSION not defined for this platform"
#endif
static struct {
    bool active;
    irqid_t int_id;
    struct vcpu* interrupted;
    struct vcpu* owner;
} vm_irq_service = {
    .int_id = INVALID_IRQID,
};

void pause_vcpu_to_interrupt(struct vcpu* interrupted, irqid_t int_id, struct vcpu* owner)
{
    if(vm_irq_service.active) {
        ERROR("Trying to pause a vcpu to interrupt, but another vcpu is already paused for an interrupt. This should not happen.");
    }
    if(vm_irq_service.int_id != INVALID_IRQID) {
        ERROR("Trying to pause a vcpu to interrupt, but another interrupt is already being serviced. This should not happen.");
    }
    vm_irq_service.interrupted = interrupted;
    vm_irq_service.int_id = int_id;
    vm_irq_service.active = true;
    vm_irq_service.owner = owner;
}

void resume_vcpu_from_interrupt(struct vcpu* owner)
{
    if(owner != vm_irq_service.owner) {
        ERROR("Trying to resume a vcpu from interrupt, but the owner vcpu does not match the one that was paused. This should not happen.");
    }

    if(vm_irq_service.active) {
        cpu()->next_vcpu = vm_irq_service.interrupted;
    }

    vm_irq_service.owner = NULL;
    vm_irq_service.active = false;
    vm_irq_service.int_id = INVALID_IRQID;
    vm_irq_service.interrupted = NULL;
}



void interrupts_arch_init()
{
    gic_init();
}

void interrupts_arch_ipi_send(cpuid_t target_cpu)
{
    gic_send_sgi(target_cpu, interrupts_ipi_id);
}

inline irqid_t interrupts_arch_reserve(irqid_t pint_id)
{
    return pint_id;
}

void interrupts_arch_enable(irqid_t int_id, bool en)
{
    gic_set_enable(int_id, en);
    gic_set_prio(int_id, 0x01);
    if (GIC_VERSION == GICV2) {
        gicd_set_trgt(int_id, (uint8_t)(1U << cpu()->id));
    } else {
        gicd_set_route(int_id, cpu()->arch.mpidr);
    }
}

bool interrupts_arch_check(irqid_t int_id)
{
    return gic_get_pend(int_id);
}

bool interrupts_arch_conflict(bitmap_t* interrupt_bitmap, irqid_t int_id)
{
    return (bitmap_get(interrupt_bitmap, int_id) && int_id > GIC_CPU_PRIV);
}

void interrupts_arch_clear(irqid_t int_id)
{
    gic_set_act(int_id, false);
    gic_set_pend(int_id, false);
}

void interrupts_arch_vm_assign(struct vm* vm, irqid_t id)
{
    vgic_set_hw(vm, id);
}
