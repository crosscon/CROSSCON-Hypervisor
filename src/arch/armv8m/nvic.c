/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <interrupts.h>

#include <cpu.h>
#include <mem.h>
#include <vm.h>
#include <arch/nvic.h>
#include <arch/sysregs.h>

extern irq_handler_t interrupt_handlers[MAX_INTERRUPT_HANDLERS];

void nvic_init(void) { }

void nvic_int_handle(void)
{
    irqid_t int_id = (irqid_t)(scb_s->icsr & SCB_ICSR_VECTACTIVE_MSK);

    if ((int_id > EXT_IRQ_BASE) && (int_id < MAX_INTERRUPT_LINES)) {
        interrupts_handle(int_id);
    }
}

bool nvic_any_act_irq(struct nvic* ic)
{
    bool ret = false;
    for (unsigned int i = 0; i < (sizeof(ic->iabr) / sizeof(ic->iabr[0])); i++) {
        if (ic->iabr[i]) {
            ret = true;
            break;
        }
    }
    return ret;
}
