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

void vmstack_push(struct vcpu* vcpu){

    if(cpu.vcpu != NULL && vcpu->state != VCPU_INACTIVE){
        return;
    }

    if(cpu.vcpu != NULL){
        vcpu_save_state(cpu.vcpu);
        cpu.vcpu->state = VCPU_STACKED;
        list_push_front(&cpu.vcpu_stack, &cpu.vcpu->node);
        vcpu->parent = cpu.vcpu;
    }

    vcpu_restore_state(vcpu);
    vcpu->state = VCPU_ACTIVE;
    cpu.vcpu = vcpu;

    /* INFO("Current VM on pCPU %d is VM %d", cpu.id, cpu.vcpu->vm->id); */
}

struct vcpu* vmstack_pop(){

    /* TODO: our nodes do not allow the same vcpu to be in the stack more than
     * once */
    struct vcpu* vcpu = (struct vcpu*) list_pop(&cpu.vcpu_stack);

    if(vcpu != NULL){
        vcpu->parent = NULL;
        struct vcpu *temp = vcpu;
        vcpu = cpu.vcpu;
        cpu.vcpu = temp;
        vcpu_save_state(vcpu);
        vcpu->state = VCPU_INACTIVE;
        vcpu_restore_state(cpu.vcpu);
        cpu.vcpu->state = VCPU_ACTIVE;
    }

    /* INFO("Current VM on pCPU %d is VM %d", cpu.id, cpu.vcpu->vm->id); */

    return vcpu;
}

void vmstack_unwind(struct vcpu* vcpu){

    if(vcpu->state != VCPU_STACKED){
        return;
    }

    struct vcpu* temp_vcpu = NULL;

    do {
        temp_vcpu = (struct vcpu*) list_pop(&cpu.vcpu_stack);
        temp_vcpu->state = VCPU_INACTIVE;
        temp_vcpu->parent = NULL;
    } while(temp_vcpu != vcpu);

    vcpu_save_state(cpu.vcpu);
    cpu.vcpu->state = VCPU_INACTIVE;
    cpu.vcpu = vcpu;
    vcpu_restore_state(cpu.vcpu);
    cpu.vcpu->state = VCPU_ACTIVE;

    /* INFO("Current VM on pCPU %d is VM %d", cpu.id, cpu.vcpu->vm->id); */
}
