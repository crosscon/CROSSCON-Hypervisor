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
// #define SMCC_FID_FN_NUM_MSK (0xFFFF)

static long sdgpos_smc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    UNUSED_ARG(smc_fid);
    UNUSED_ARG(vcpu);
    return -1;
}

static long sdgpos_hvc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    UNUSED_ARG(smc_fid);
    UNUSED_ARG(vcpu);
    return -1;
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
