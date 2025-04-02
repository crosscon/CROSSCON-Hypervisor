#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vmm.h"
#include "arch/sdgpos.h"

// #define SMCC64_BIT   (0x40000000)

//  #define SMCC_E_NOT_SUPPORTED  (-1)
// #define SMCC32_FID_VND_HYP_SRVC (0x86000000)
// #define SMCC64_FID_VND_HYP_SRVC (SMCC32_FID_VND_HYP_SRVC  | SMCC64_BIT)
#define SMCC_FID_FN_NUM_MSK (0xFFFF)

static int64_t sdgpos_smc_handler(struct vcpu* vcpu, long unsigned int smc_fid)
{
    if (vcpu->vm->type != 0) {
        return 0;
    }
    long unsigned int ret = (long unsigned)-HC_E_FAILURE;
    int res = 0;
    long unsigned int x1 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(0));
    long unsigned int x2 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(1));
    long unsigned int x3 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(2));

    if (is_psci_fid(smc_fid)) {
        res = psci_smc_handler((uint32_t)smc_fid, x1, x2, x3);
        vcpu_writereg(vcpu, 0, ret);
    }

    /* every smc call here increments pc */
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);
    ret = 0;
    return res;
}

static int64_t sdgpos_hvc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    UNUSED_ARG(smc_fid);
    if (vcpu->vm->type != 0) {
        return 0;
    }

    long int res;
    unsigned long fid = vcpu_readreg(vcpu, 0);

    switch (fid & SMCC_FID_FN_NUM_MSK) {
        case HC_IPC:
            res = ipc_hypercall(vcpu);
            vcpu_writereg(vcpu, 0, (unsigned long int)res);
            break;
        default:
            /* WARNING("Unknown hypercall id %x", fid); */
            res = -1;
    }

    return res;
}

static struct hndl_smc smc = {
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdgpos_smc_handler,
};

static struct hndl_hvc hvc = {
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .handler = sdgpos_hvc_handler,
};

bool sdgpos_arch_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    if (vm->type == 0) {
        vm_hndl_smc_add(vm, &smc);
        vm_hndl_hvc_add(vm, &hvc);
    }

    return ret;
}
