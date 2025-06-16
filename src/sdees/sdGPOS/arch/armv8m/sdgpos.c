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

static int64_t sdgpos_smc_handler(struct vcpu* vcpu, unsigned long smc_fid)
{
    UNUSED_ARG(smc_fid);
    UNUSED_ARG(vcpu);
    return -1;
}

static long sdgpos_hvc_handler(struct vcpu* vcpu, uint64_t smc_fid)
{
    long int ret = -HC_E_INVAL_ID;
    uint64_t id;
    //aqui falta ir buscar o valor de R0 e defenir o id=<função que recolhe o R0>;
    //r0 vem como parametro da hypercall
    id = smc_fid; //isto esta aqui apenas para não dar erros de compilação

    switch (id) {
        case HC_IPC:
            ret = ipc_hypercall(vcpu);
            break;
        default:
            //WARNING("Unknown hypercall id %d\n", id);
            break;
    }

    return ret;
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
