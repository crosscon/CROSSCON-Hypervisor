/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <dynconfig.h>


static void *adjust_ptr(void *p, uintptr_t o) {
    if(p){
        return (void *)((uintptr_t)p + o);
    }
    return p;
}


static void dynconfig_adjust_vm(struct vm_config *vm_cfg, struct dynconfig* dynconfig, paddr_t load_addr)
{
    vm_cfg->image.load_addr += load_addr;

    vm_cfg->platform.regions = adjust_ptr(vm_cfg->platform.regions, (uintptr_t)dynconfig);

    if ((vm_cfg->platform.devs = adjust_ptr(vm_cfg->platform.devs, (uintptr_t)dynconfig))) {
        for (size_t j = 0; j < vm_cfg->platform.dev_num; j++) {
            vm_cfg->platform.devs[j].interrupts = adjust_ptr(vm_cfg->platform.devs[j].interrupts, (uintptr_t)dynconfig);
        }
    }

    if((vm_cfg->platform.ipcs = adjust_ptr(vm_cfg->platform.ipcs, (uintptr_t)dynconfig))){
        for (size_t j = 0; j < vm_cfg->platform.ipc_num; j++) {
            vm_cfg->platform.ipcs[j].interrupts = adjust_ptr(vm_cfg->platform.ipcs[j].interrupts, (uintptr_t)dynconfig);
        }
    }
}


void dynconfig_init(struct dynconfig* dynconfig, paddr_t load_addr)
{
    dynconfig_adjust_vm(&dynconfig->vm_cfg,  dynconfig, load_addr);
}
