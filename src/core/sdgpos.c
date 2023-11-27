#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"

int64_t smc_handler(uint64_t smc_fid) 
{

    if(cpu.vcpu->vm->id != 2)
        return 0;
    int64_t ret = -HC_E_FAILURE;
    struct vcpu *vcpu = cpu.vcpu;
    uint64_t x1 = vcpu->regs->x[1];
    uint64_t x2 = vcpu->regs->x[2];
    uint64_t x3 = vcpu->regs->x[3];


    if (is_psci_fid(smc_fid)) {
        ret = psci_smc_handler(smc_fid, x1, x2, x3);
        ret = 0;
    }
    /* every smc call increments pc */
    vcpu_writereg(vcpu, 0, ret);
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);
    return ret;
}


extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}
void handle_interrupt(irqid_t int_id)
{
   struct vcpu *vcpu = cpu_get_vcpu(interrupts_get_vmid(int_id));

   interrupts_vm_inject(vcpu, int_id);
}

#include <vmm.h>
static struct hndl_smc smc = {
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = smc_handler,
};

static struct hndl_irq irq = {
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = handle_interrupt,
};

int64_t sdgpos_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    if(vm->id == 2){
        vm_hndl_smc_add(vm, &smc);
        vm_hndl_irq_add(vm, &irq);
    }

    return ret;
}

