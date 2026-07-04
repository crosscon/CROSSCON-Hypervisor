/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <interrupts.h>

#include <cpu.h>
#include <mem.h>
#include <platform.h>
#include <vm.h>
#include <fences.h>
#include <arch/sysregs.h>
#include <arch/nvic.h>
#include <arch/timer.h>

extern irq_handler_t interrupt_handlers[MAX_INTERRUPT_HANDLERS];

static struct {
    bool active;
    irqid_t int_id;
    struct vcpu* interrupted;
    struct vcpu* owner;
} vm_irq_service;

void interrupts_arch_ipi_init(void){
    //Joao Add this here to replace the weak implementation of this function. 
}

void interrupts_arch_init()
{
    nvic_init();

    // Prioritize secure exceptions
    scb_s->aircr = (scb_s->aircr & ~SCB_AIRCR_VECTKEY_MSK) |
        (SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIS | SCB_AIRCR_BFHFNMINS);

    // Enable all interrupts.
    interrupts_arch_enable_all();
}

void interrupts_arch_enable(irqid_t int_id, bool en)
{
    if (int_id > EXT_IRQ_BASE) {
        nvic_enable(nvic_s, int_id, en);
    } else if (int_id == EXC_SYSTICK) {
        systick_int_enable(systick_s, en);
    }
}

void interrupts_arch_handle(void)
{
    nvic_int_handle();
}

struct vcpu* get_vcpu_to_interrupt(void)
{
    return vm_irq_service.owner;
}

void interrupts_arch_pendsv_handle(void)
{
    if (!vm_irq_service.active) {
        return;
    }

    if (nvic_get_act(nvic_s, vm_irq_service.int_id)) {
        scb_s->icsr = SCB_ICSR_PENDSVSET;
        return;
    }

    interrupts_vm_inject(vm_irq_service.owner, vm_irq_service.int_id);
    cpu()->next_vcpu = vm_irq_service.owner;
}

bool interrupts_arch_vm_irq_enter(struct vcpu* owner, irqid_t int_id)
{
    if ((owner == NULL) || (owner == cpu()->vcpu)) {
        return false;
    }

    if (vm_irq_service.active) {
        return false;
    }

    vm_irq_service.active = true;
    vm_irq_service.int_id = int_id;
    vm_irq_service.interrupted = cpu()->vcpu;
    vm_irq_service.owner = owner;

    //interrupts_vm_inject(owner, int_id);
    cpu()->next_vcpu = cpu()->vcpu;
    scb_s->icsr = SCB_ICSR_PENDSVSET;

    return true;
}

bool interrupts_arch_vm_irq_resume(void)
{
    if (!vm_irq_service.active) {
        return false;
    }

    //note that we expect secure context place hyp call insince handler, so the interrupt is still active in nvic
    if ((cpu()->vcpu != vm_irq_service.owner) /*|| vm_nvic_act_irq()*/) {
        return false;
    }

    cpu()->next_vcpu = vm_irq_service.interrupted;

    vm_irq_service.active = false;
    vm_irq_service.int_id = INVALID_IRQID;
    vm_irq_service.interrupted = NULL;
    vm_irq_service.owner = NULL;

    return true;
}

bool interrupts_arch_check(irqid_t int_id)
{
    if (int_id > EXT_IRQ_BASE) {
        return nvic_get_pend(nvic_s, int_id);
    } else if (int_id == EXC_SYSTICK) {
        return systick_get_pend(systick_s);
    }
    return INVALID_IRQID;
}

void interrupts_arch_clear(irqid_t int_id)
{
    if (int_id > EXT_IRQ_BASE) {
        nvic_clr_pend(nvic_s, int_id);
    } else if (int_id == EXC_SYSTICK) {
        systick_clr_pend(systick_s);
    }
}

irqid_t interrupts_arch_reserve(irqid_t int_id)
{
    if (int_id > EXT_IRQ_BASE) {
        nvic_int_target(SECURE, int_id);
        return int_id;
    } else if (int_id == EXC_SYSTICK) {
        return int_id;
    }
    // TODO:ARMV8M - are we missing something here?!
    return INVALID_IRQID;
}

inline bool interrupts_arch_conflict(bitmap_t* interrupt_bitmap, irqid_t int_id)
{
    return bitmap_get(interrupt_bitmap, int_id);
}

void interrupts_arch_vm_assign(struct vm* vm, irqid_t int_id)
{
    UNUSED_ARG(vm);
    UNUSED_ARG(int_id);
}

void interrupts_arch_ipi_send(cpuid_t cpu_target)
{
    UNUSED_ARG(cpu_target);
}
