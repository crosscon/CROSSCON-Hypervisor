#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/sdgpos.h>
#include <sdgpos.h>
#include <interrupts.h>

extern uint64_t interrupt_owner[MAX_INTERRUPT_LINES];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

static void sdgpos_interrupt_handle(struct vcpu* vcpu, irqid_t int_id)
{
    #if defined(ARCH_SUB_ARMV8M)
        interrupts_vm_inject(vcpu, int_id);
    #else
    if(vcpu->vm->type == 0){ //if interrupt owner is non-secure
        if (vcpu->vm->id != cpu()->vcpu->vm->id) {
            pause_vcpu_to_interrupt(cpu()->vcpu, int_id, vcpu);
            vmstack_push(vcpu);
            interrupts_vm_inject(vcpu, int_id);
            vcpu_context_switch();
        }else{ //if interrupt is secure and execution is secure
            interrupts_vm_inject(vcpu, int_id);
            cpu()->next_vcpu = vcpu;
        }
    }
    // if (vcpu->vm->type != 0) {
    //     //this means that execution is currently in secure context
    //     //and hyp recieved an interrupt probably belonging to non-secure
    //     //so we need to forward the interrupt to the non-secure context
    //     struct vcpu* owner_vcpu = cpu_get_vcpu_by_vmid((vmid_t)interrupts_get_vmid(int_id));
    //     if (owner_vcpu != NULL) {
    //         interrupts_vm_inject(owner_vcpu, int_id);
    //         cpu()->next_vcpu = owner_vcpu;
    //         pause_vcpu_to_interrupt(vcpu, int_id, owner_vcpu);
    //     }else{
    //         ERROR("sdgpos_interrupt_handle: interrupt %d not assigned to this vcpu\n", int_id);
    //     }
    //     return;
    // }

    // interrupts_vm_inject(vcpu, int_id);
    // cpu()->next_vcpu = vcpu;
    // return;
    #endif
}

struct hndl_irq irq = {
    /* CROSSCON TODO: obtain this from config file */
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .num = 11,
    .irqs = { 125, 337, 138, 321, 73, 74, 75, 76, 77, 78, 79 },
    .handler = sdgpos_interrupt_handle,
};

int64_t sdgpos_handler_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    sdgpos_arch_setup(vm);

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    if (vm->type == 0) {
        vm_hndl_irq_add(vm, &irq);
    }

    return ret;
}
