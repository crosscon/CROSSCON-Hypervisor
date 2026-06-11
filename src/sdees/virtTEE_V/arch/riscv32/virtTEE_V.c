#include <virtTEE_V.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/virtTEE_V.h>

#define is_psci_fid(_fid) 0

void virtteev_copy_args(struct vcpu* vcpu_dst, struct vcpu* vcpu_src, size_t num_args)
{
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = REG_A6-i;
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid));
    }
}

void virtteev_copy_args_call_done(struct vcpu* vcpu_dst, struct vcpu* vcpu_src, size_t num_args)
{
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = REG_A6-i;
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid + 1));
    }
}

/* CROSSCON TODO Not good */
extern long virtteev_handler(struct vcpu* vcpu, uint32_t fid);


static long virtteev_smc_handler(struct vcpu* vcpu, uint32_t smc_fid)

{
    long ret = -HC_E_FAILURE;

    struct vcpu* calling_vcpu = cpu()->vcpu;

    if (calling_vcpu->vm->type == 0) { /* normal world */
        if (is_psci_fid(smc_fid)) {
            /* CROSSCON TODO: signal trusted OS a PSCI event is comming up */
            /* potentially handle core going to sleep */
            return HC_E_SUCCESS;
        } else {
            /* CROSSCON TODO: If SMC call is for trusted OS */
            ret = virtteev_handler(vcpu, smc_fid);
        }
    } else {
        ret = virtteev_handler(vcpu, smc_fid);
    }

    return ret;
}

static struct hndl_smc smc = {
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .handler = virtteev_smc_handler,
};

int32_t virtteev_arch_handler_setup(struct vm* vm)
{
    int32_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_smc_add(vm, &smc);

    return ret;
}

void tee_arch_interrupt_disable(void)
{
    /* CROSSCON TODO disable interrupts */
    csrs_hie_clear(HIE_VSEIE);
    csrs_hideleg_clear(HIE_VSEIE);
}

void tee_arch_interrupt_enable(void)
{
    csrs_hie_set(HIE_VSEIE);
    csrs_hideleg_set(HIE_VSEIE);
}