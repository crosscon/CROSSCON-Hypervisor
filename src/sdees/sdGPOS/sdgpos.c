#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"

int64_t sdgpos_smc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    if(vcpu->vm->id != 2)
        return 0;
    int64_t ret = -HC_E_FAILURE;
    uint64_t x1 = vcpu->regs->x[1];
    uint64_t x2 = vcpu->regs->x[2];
    uint64_t x3 = vcpu->regs->x[3];

    if (is_psci_fid(smc_fid)) {
        ret = psci_smc_handler(smc_fid, x1, x2, x3);
        ret = 0;
        vcpu_writereg(vcpu, 0, ret);
    }
    /* every smc call here increments pc */
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);
    ret = 0;
    return ret;
}


extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}
void sdgpos_interrupt_handle(struct vcpu* vcpu, irqid_t int_id)
{

   interrupts_vm_inject(vcpu, int_id);
}

static struct hndl_smc smc = {
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdgpos_smc_handler,
};

static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = sdgpos_interrupt_handle,
};

int64_t sdgpos_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    if(vm->id == 2){
        vm_hndl_smc_add(vm, &smc);
        vm_hndl_irq_add(vm, &irq);
    }

    return ret;
}

