#include <sdtz.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
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

/* TODO Not good */
extern int64_t sdtz_handler(struct vcpu* vcpu, uint64_t fid);

int64_t sdtz_smc_handler(struct vcpu* vcpu, uint64_t smc_fid) {

    int64_t ret = -HC_E_FAILURE;

    struct vcpu *calling_vcpu = cpu.vcpu;

    if (calling_vcpu->vm->type == 0) { /* normal world */
        if (is_psci_fid(smc_fid)) {
            /* TODO: signal trusted OS a PSCI event is comming up */
            /* potentially handle core going to sleep */
            return HC_E_SUCCESS;
        } else {
            /* TODO: If SMC call is for trusted OS */
            ret = sdtz_handler(vcpu, smc_fid);
        }
    }else{
            ret = sdtz_handler(vcpu, smc_fid);
    }

    return ret;
}


static struct hndl_smc smc = {
    /* TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdtz_smc_handler,
};


int64_t sdtz_arch_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_smc_add(vm, &smc);

    return ret;
}

void tee_arch_interrupt_disable()
{
}

void tee_arch_interrupt_enable()
{

}

void tee_step(struct vcpu* vcpu)
{
    uint64_t pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);

}
