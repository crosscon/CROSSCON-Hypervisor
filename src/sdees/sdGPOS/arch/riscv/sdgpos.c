#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include <hypercall.h>
#include "vm.h"
#include "vmm.h"


static int64_t sdgpos_smc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    /* if(vcpu->vm->id != 2) */
    /*     return 0; */
    /* int64_t ret = -HC_E_FAILURE; */
    /* uint64_t x1 = vcpu->regs->x[1]; */
    /* uint64_t x2 = vcpu->regs->x[2]; */
    /* uint64_t x3 = vcpu->regs->x[3]; */

    /* uint64_t pc_step = 2 + (2 * 1); */
    /* vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step); */
    /* ret = 0; */
    /* return ret; */
    return 0;
}

int64_t sdgpos_hvc_handler(struct vcpu* vcpu, uint64_t fid)
{
    if(vcpu->vm->type != 0)
        return 0;

    int64_t ret;

    unsigned long arg0 = vcpu_readreg(vcpu, REG_A0);
    unsigned long arg1 = vcpu_readreg(vcpu, REG_A1);
    unsigned long arg2 = vcpu_readreg(vcpu, REG_A2);
    /* unsigned long arg3 = vcpu_readreg(vcpu, REG_A3); */

    /* TODO: sdee */
    switch(fid) {
        case HC_IPC:
            ret = ipc_hypercall(vcpu, arg0, arg1, arg2);
            break;
        default:
            /* WARNING("Unknown hypercall id %x", fid); */
            ret = -1;
    }

    return ret;
}
static struct hndl_smc smc = {
    /* TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdgpos_smc_handler,
};

static struct hndl_hvc hvc = {
    /* TODO: obtain this to decide whether to invoke handler early on */
    .handler = sdgpos_hvc_handler,
};


bool sdgpos_arch_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    if(vm->type == 0){
        vm_hndl_smc_add(vm, &smc);
        vm_hndl_hvc_add(vm, &hvc);
    }

    return ret;
}
