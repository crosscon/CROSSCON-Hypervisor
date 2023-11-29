/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Angelo Ruocco <angeloruocco90@gmail.com>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <interrupts.h>

#include <cpu.h>
#include <vm.h>
#include <bitmap.h>
#include <string.h>
#include <vmstack.h>
#include "bao.h"

BITMAP_ALLOC(hyp_interrupt_bitmap, MAX_INTERRUPTS);
BITMAP_ALLOC(global_interrupt_bitmap, MAX_INTERRUPTS);
BITMAP_ALLOC(interrupt_shared, MAX_INTERRUPTS);
uint64_t interrupt_owner[MAX_INTERRUPTS];
irq_handler_t interrupt_handlers[MAX_INTERRUPTS];

inline void interrupts_cpu_sendipi(cpuid_t target_cpu, irqid_t ipi_id)
{
    interrupts_arch_ipi_send(target_cpu, ipi_id);
}

inline void interrupts_cpu_enable(irqid_t int_id, bool en)
{
    interrupts_arch_enable(int_id, en);
}

inline bool interrupts_check(irqid_t int_id)
{
    return interrupts_arch_check(int_id);
}

inline void interrupts_clear(irqid_t int_id)
{
    interrupts_arch_clear(int_id);
}

inline void interrupts_init()
{
    interrupts_arch_init();

    if (cpu.id == CPU_MASTER) {
        interrupts_reserve(IPI_CPU_MSG, cpu_msg_handler);
    }

    interrupts_cpu_enable(IPI_CPU_MSG, true);
}

static inline bool interrupt_is_reserved(irqid_t int_id)
{
    return bitmap_get(hyp_interrupt_bitmap, int_id);
}



inline void interrupts_vm_inject(struct vcpu* vcpu, uint64_t id)
{
   interrupts_arch_vm_inject(vcpu, id);
}

static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

enum irq_res interrupts_handle(irqid_t int_id)
{
    struct vcpu *vcpu = NULL;

    if(interrupts_is_shared(int_id) || (cpu.vcpu->vm->id == interrupts_get_vmid(int_id))){
        vcpu = cpu.vcpu;
    } else {
        vcpu = cpu_get_vcpu(interrupts_get_vmid(int_id));
    }

    /* TODO */
    if((vcpu != NULL) && vm_has_interrupt(vcpu->vm, int_id)){
        list_foreach(vcpu->vm->irq_list, struct hndl_irq_node, node)
        {
            /* IF ... */
            sdirq_handler_t handler = node->hndl_irq.handler;
            if (handler != NULL) {
                handler(vcpu, int_id);
            }
        }

        return FORWARD_TO_VM;

    } else if (interrupt_is_reserved(int_id)) {
        interrupt_handlers[int_id](int_id);

        return HANDLED_BY_HYP;

    } else {
        ERROR("received unknown interrupt id = %d", int_id);
    }
}

void interrupts_vm_assign(struct vm *vm, irqid_t id)
{
    if (interrupts_arch_conflict(global_interrupt_bitmap, id)) {
        ERROR("Interrupts conflict, id = %d\n", id);
    }
    if(bitmap_get(hyp_interrupt_bitmap, id) ||
        (!interrupts_is_shared(id) && interrupt_owner[id] != 0)){
        ERROR("Trying to assign cpu interrupt multiple times\n", id);
    }

    interrupts_arch_vm_assign(vm, id);
    interrupt_owner[id] = vm->id; /* TODO */
    bitmap_set(vm->interrupt_bitmap, id);
    bitmap_set(global_interrupt_bitmap, id);
}

void interrupts_reserve(irqid_t int_id, irq_handler_t handler)
{
    if (int_id < MAX_INTERRUPTS) {
        interrupt_handlers[int_id] = handler;
        bitmap_set(hyp_interrupt_bitmap, int_id);
        bitmap_set(global_interrupt_bitmap, int_id);
    }
}

void interrupts_set_shared(uint64_t id){

    if(bitmap_get(hyp_interrupt_bitmap, id) || interrupt_owner[id] != 0){
        return;
    }

    bitmap_set(interrupt_shared, id);
}

bool interrupts_is_shared(uint64_t id){
    return !!bitmap_get(interrupt_shared, id);
}
