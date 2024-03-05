#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/sdgpos.h>


extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}
void sdgpos_interrupt_handle(struct vcpu* vcpu, irqid_t int_id)
{
   interrupts_vm_inject(vcpu, int_id);
}


static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    /* TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = sdgpos_interrupt_handle,
};

int64_t sdgpos_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    sdgpos_arch_setup(vm);

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    if(vm->id == 2){
        vm_hndl_irq_add(vm, &irq);
    }

    return ret;
}

