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

#include <vmstack.h>

#include <hypercall.h>
#include <vm.h>
#include <cpu.h>
#include <string.h>
#include "list.h"

void vmstack_push(struct vcpu* vcpu)
{
    if (cpu()->vcpu != NULL && vcpu->state != VCPU_INACTIVE) {
        return;
    }

    struct vcpu* old_vcpu = cpu()->vcpu;

    if (old_vcpu != NULL) {
        vcpu_save_state(old_vcpu);
        old_vcpu->state = VCPU_STACKED;
    }

    list_push_front(&vcpu->root_vcpu->vcpu_stack_lst, &vcpu->vmstack_node);
    vcpu->parent = old_vcpu;

    vcpu_restore_state(vcpu);
    vcpu->state = VCPU_ACTIVE;
    cpu()->vcpu = vcpu;
    cpu()->next_vcpu = vcpu;

    /* INFO("Current VM on pCPU %d is VM %d\n", cpu()->id, cpu()->vcpu->vm->id); */
}

struct vcpu* vmstack_pop()
{
    /* CROSSCON TODO: our nodes do not allow the same vcpu to be in the stack more than
     * once */
    if (cpu()->vcpu->parent == NULL) {
        return NULL;
    }
    node_t node = list_pop(&cpu()->vcpu->root_vcpu->vcpu_stack_lst);
    struct vcpu* popped_vcpu = CONTAINER_OF(struct vcpu, vmstack_node, node);
    if (popped_vcpu == NULL) {
        ERROR("Pop operation failed")
    }

    node = list_peek(&cpu()->vcpu->root_vcpu->vcpu_stack_lst);
    struct vcpu* stack_top = CONTAINER_OF(struct vcpu, vmstack_node, node);

    popped_vcpu->parent = NULL;

    vcpu_save_state(popped_vcpu);
    popped_vcpu->state = VCPU_INACTIVE;
    vcpu_restore_state(stack_top);
    stack_top->state = VCPU_ACTIVE;
    cpu()->vcpu = stack_top;
    cpu()->next_vcpu = stack_top;

    /* INFO("Current VM on pCPU %d is VM %d\n", cpu()->id, cpu()->vcpu->vm->id); */

    return popped_vcpu;
}

void vmstack_unwind(struct vcpu* vcpu)
{
    if (vcpu->state != VCPU_STACKED) {
        return;
    }

    struct vcpu* temp_vcpu = NULL;

    do {
        node_t node = list_pop(&vcpu->vcpu_stack_lst);
        temp_vcpu = CONTAINER_OF(struct vcpu, vmstack_node, node);
        temp_vcpu->state = VCPU_INACTIVE;
        temp_vcpu->parent = NULL;
    } while (temp_vcpu != vcpu);

    vcpu_save_state(cpu()->vcpu);
    cpu()->vcpu->state = VCPU_INACTIVE;
    cpu()->vcpu = vcpu;
    vcpu_restore_state(cpu()->vcpu);
    cpu()->vcpu->state = VCPU_ACTIVE;

    /* INFO("Current VM on pCPU %d is VM %d\n", cpu()->id, cpu()->vcpu->vm->id); */
}
