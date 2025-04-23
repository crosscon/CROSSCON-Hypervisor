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

extern irq_handler_t interrupt_handlers[MAX_INTERRUPT_HANDLERS];

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

void interrupts_arch_ipi_send(cpuid_t cpu_target, irqid_t ipi_id)
{
    UNUSED_ARG(cpu_target);
    UNUSED_ARG(ipi_id);
}
