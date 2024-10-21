#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vmm.h"
#include "arch/sdgpos.h"

#define SMCC64_BIT   (0x40000000)

#define SMCC_E_NOT_SUPPORTED  (-1)
#define SMCC32_FID_VND_HYP_SRVC (0x86000000)
#define SMCC64_FID_VND_HYP_SRVC (SMCC32_FID_VND_HYP_SRVC  | SMCC64_BIT)
#define SMCC_FID_FN_NUM_MSK     (0xFFFF)

int64_t sdgpos_smc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    if(vcpu->vm->type != 0)
        return 0;
    int64_t ret = -HC_E_FAILURE;
    uint64_t x1 = vcpu->regs->x[1];
    uint64_t x2 = vcpu->regs->x[2];
    uint64_t x3 = vcpu->regs->x[3];

    if (is_psci_fid(smc_fid)) {
        ret = psci_smc_handler(smc_fid, x1, x2, x3);
        vcpu_writereg(vcpu, 0, ret);
    }

    /* every smc call here increments pc */
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);
    ret = 0;
    return ret;
}

int64_t sdgpos_hvc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    if(vcpu->vm->type != 0)
        return 0;

    unsigned long ret;
    unsigned long fid = vcpu_readreg(vcpu, 0);

    unsigned long ipc_id = vcpu_readreg(vcpu, HYPCALL_ARG_REG(1));
    unsigned long arg1   = vcpu_readreg(vcpu, HYPCALL_ARG_REG(2));
    unsigned long arg2   = vcpu_readreg(vcpu, HYPCALL_ARG_REG(3));

    switch(fid & SMCC_FID_FN_NUM_MSK){
        case HC_IPC:
            ret = ipc_hypercall(vcpu, ipc_id, arg1, arg2);
            vcpu_writereg(vcpu, 0, ret);
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
