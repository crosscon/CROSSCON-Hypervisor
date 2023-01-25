/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
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

int cpu_different;

void vmstack_push(struct vcpu* vcpu){

    if(cpu.vcpu != NULL && vcpu->state != VCPU_INACTIVE){
        return;
    }

    if(cpu.vcpu != NULL){
        vcpu_save_state(cpu.vcpu);
        cpu.vcpu->state = VCPU_STACKED;
        list_push(&cpu.vcpu_stack, &cpu.vcpu->node);
        vcpu->parent = cpu.vcpu;
    }
    
    vcpu_restore_state(vcpu);
    vcpu->state = VCPU_ACTIVE;
    cpu.vcpu = vcpu;
}

struct vcpu* vmstack_pop(){

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
}

enum {
    VMSTACK_RESUME = 1,
    VMSTACK_GOTO = 2,
    VMSTACK_RETURN = 3,
};

extern uint8_t *result;

int64_t vmstack_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    int64_t res = HC_E_SUCCESS;
    
    struct vcpu* child = NULL;

    uint64_t tmp = MRS(DAIF);

    switch(id) {
        case VMSTACK_RESUME:
        case VMSTACK_GOTO:
            tmp |= (1ULL << 7);
            MSR(DAIF, tmp);
            if((child = vcpu_get_child(cpu.vcpu, arg0)) != NULL){
                if(id == VMSTACK_GOTO) {
                    vcpu_writepc(child, arg1);
                }
                vmstack_push(child);
            } else {
                res = -HC_E_INVAL_ARGS;
            }
            break;
        case VMSTACK_RETURN:
            tmp &= ~(1ULL << 7);
            MSR(DAIF, tmp);
            vmstack_pop();
            break;
        default:
            res = -HC_E_FAILURE;
    }

    /**
     * If there is a new vcpu we need to check if its PSCI state is on. If not,
     * let's enter idle and wait for someaone to wake us up.
     * TODO: rewrite this check with an arch agnostic api
     */
    if (cpu.vcpu->arch.psci_ctx.state == OFF) {
        cpu_idle();
    }
    
    return res;
}
