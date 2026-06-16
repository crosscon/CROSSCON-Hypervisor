#include <sdtzm.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/sdtzm.h>

//#define is_psci_fid(_fid) 0

void sdtzm_copy_args(struct vcpu* vcpu_dst, struct vcpu* vcpu_src, size_t num_args)
{
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = i;
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid));
    }
}

void sdtzm_copy_args_call_done(struct vcpu* vcpu_dst, struct vcpu* vcpu_src, size_t num_args)
{
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = i;
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid + 1));
    }
}

/* CROSSCON TODO Not good */
extern long sdtzm_handler(struct vcpu* vcpu, uint64_t fid);

static long sdtzm_hvc_handler(struct vcpu* vcpu, uint32_t smc_fid)
{
    long ret = -HC_E_FAILURE;

    //struct vcpu* calling_vcpu = cpu()->vcpu;

    ret = sdtzm_handler(vcpu, smc_fid);

    // if (calling_vcpu->vm->type == 0) { /* normal world */ //not needed for v8-m case
    //     if (is_psci_fid(smc_fid)) {
    //         /* CROSSCON TODO: signal trusted OS a PSCI event is comming up */
    //         /* potentially handle core going to sleep */
    //         return HC_E_SUCCESS;
    //     } else {
    //         /* CROSSCON TODO: If HVC call is for trusted OS */
    //         ret = sdtzm_handler(vcpu, smc_fid);
    //     }
    // } else {
    //     ret = sdtzm_handler(vcpu, smc_fid);
    // }

    return ret;
}

static struct hndl_hvc hvc = {
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdtzm_hvc_handler,
};

int64_t sdtzm_arch_handler_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_hvc_add(vm, &hvc);

    return ret;
}

void tee_arch_interrupt_disable() { }

void tee_arch_interrupt_enable() { }

void tee_step(struct vcpu* vcpu)
{
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + (unsigned long)pc_step);
}
