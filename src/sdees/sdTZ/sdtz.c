#include <sdtz.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "bao.h"
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/sdtz.h>


static inline void sdtz_copy_args(struct vcpu *vcpu_dst, struct vcpu *vcpu_src,
        size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid));
    }
}

static inline void sdtz_copy_args_call_done(struct vcpu *vcpu_dst, struct vcpu
        *vcpu_src, size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid+1));
    }
}

int64_t sdtz_handler(struct vcpu* vcpu, uint64_t fid) {
    int64_t ret = -HC_E_FAILURE;

    if (vcpu->vm->id == 2) {
	/* normal world */
        if (vmstack_pop() != NULL) {
	    tee_arch_interrupt_disable();
            sdtz_copy_args(cpu.vcpu, vcpu, 7);
            /* TODO: more generic stepping */
            /* in arm steeping is done here, but in RISC-V it is done outside */
            tee_step(cpu.vcpu);
            ret = HC_E_SUCCESS;
        }
    } else {
	/* secure world */
        /* TODO: get parent */
        struct vcpu *ree_vcpu = vcpu_get_child(vcpu, 0);
        if (ree_vcpu != NULL) {
            switch (ID_TO_FUNCID(fid)) {
                case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
                case TEEHC_FUNCID_RETURN_ON_DONE:
                    sdtz_copy_args(ree_vcpu, cpu.vcpu, 1);
                    vmstack_push(ree_vcpu);
		    tee_arch_interrupt_enable();
                    break;
                case TEEHC_FUNCID_RETURN_CALL_DONE:
                    if(vcpu_readreg(cpu.vcpu, 1) == 0xffff0004){
                        /* interrupted */
                        /* TODO Not sure if needed */
                        sdtz_copy_args_call_done(ree_vcpu, cpu.vcpu, 4);
                    } else
                        sdtz_copy_args_call_done(ree_vcpu, cpu.vcpu, 6);
                case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                    vmstack_push(ree_vcpu);
		    tee_arch_interrupt_enable();
                    break;
                default:
                    ERROR("unknown tee call %0lx by vm %d", fid, cpu.vcpu->vm->id);
            }
            ret = HC_E_SUCCESS;
        }
    }

    return ret;
}

extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

void sdtz_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    /* TODO: check current active handler */

   if(vcpu != cpu.vcpu && vcpu->state == VCPU_INACTIVE){
       if(cpu.vcpu->vm->id == 1){ /* currently running secure world */
           /* TODO */
           /* interrupts_vm_inject(cpu.vcpu, 40); */
       }
   }
}

static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    /* TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = sdtz_handle_interrupt,
};

int64_t sdtz_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    sdtz_arch_handler_setup(vm);

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_irq_add(vm, &irq);

    return ret;
}
