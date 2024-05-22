/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <config.h>

void config_vm_adjust_to_va(struct vm_config *vm_config, struct config *config, paddr_t phys) {
    adjust_ptr(vm_config->image.load_addr, phys);

    adjust_ptr(vm_config->platform.regions, config);

    if (adjust_ptr(vm_config->platform.devs, config)) {
        for (size_t j = 0; j < vm_config->platform.dev_num; j++) {
            adjust_ptr(vm_config->platform.devs[j].interrupts, config);
        }
    }

    if(adjust_ptr(vm_config->platform.ipcs, config)){
        for (size_t j = 0; j < vm_config->platform.ipc_num; j++) {
            adjust_ptr(vm_config->platform.ipcs[j].interrupts, config);
        }
    }

    adjust_ptr(vm_config->children, config);
    for (size_t i = 0; i < vm_config->children_num; i++) {
        adjust_ptr(vm_config->children[i], config);
        config_vm_adjust_to_va(vm_config->children[i], config, phys);
    }

    config_arch_vm_adjust_to_va(vm_config, config, phys);
}

bool config_is_builtin() {
    extern uint8_t _config_start, _config_end;
    return &_config_start != &_config_end;
}

void config_adjust_to_va(struct config *config, uint64_t phys) {
    adjust_ptr(config->shmemlist, config);

    for (int i = 0; i < config->vmlist_size; i++) {
        adjust_ptr(config->vmlist[i], config);
        config_vm_adjust_to_va(config->vmlist[i], config, phys);
    }
}
