/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include <types.h>
#include <vmm.h>
#include <arch/sdgpos.h>

#define SMCC_FID_FN_NUM_MSK (0xFFFF)

static int32_t sdgpos_smc_handler(struct vcpu* vcpu, unsigned long smc_fid)
{
    if (vcpu->vm->type != 0) {
        return 0;
    }

    unsigned long ret = (unsigned long)-HC_E_FAILURE;
    int32_t res = 0;
    unsigned long x1 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(0));
    unsigned long x2 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(1));
    unsigned long x3 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(2));

    if (is_psci_fid(smc_fid)) {
        res = psci_smc_handler((uint32_t)smc_fid, x1, x2, x3);
        vcpu_writereg(vcpu, 0, ret);
    }

    unsigned long pc_step = 2 + (2 * 1);
    vcpu_writepc(vcpu, vcpu_readpc(vcpu) + pc_step);
    return res;
}

static long sdgpos_hvc_handler(struct vcpu* vcpu, uint32_t hvc_fid)
{
    UNUSED_ARG(hvc_fid);
    if (vcpu->vm->type != 0) {
        return 0;
    }

    long res;
    unsigned long fid = vcpu_readreg(vcpu, 0);

    switch (fid & SMCC_FID_FN_NUM_MSK) {
        case HC_IPC:
            res = ipc_hypercall(vcpu);
            vcpu_writereg(vcpu, 0, (unsigned long)res);
            break;
        default:
            res = -1;
    }

    return res;
}

static struct hndl_smc smc = {
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdgpos_smc_handler,
};

static struct hndl_hvc hvc = {
    .handler = sdgpos_hvc_handler,
};

bool sdgpos_arch_setup(struct vm* vm)
{
    int32_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    if (vm->type == 0) {
        vm_hndl_smc_add(vm, &smc);
        vm_hndl_hvc_add(vm, &hvc);
    }

    return ret;
}
