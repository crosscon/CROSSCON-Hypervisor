/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <config.h>

static void config_adjust_child_vm_image_addr(struct vm_config *vm_cfg, paddr_t load_addr)
{
    if (!vm_cfg->image.separately_loaded) {
        vm_cfg->image.load_addr = (vm_cfg->image.load_addr - BAO_VAS_BASE) + load_addr;
    }
    for (size_t j = 0; j < vm_cfg->children_num; j++) {
        config_adjust_child_vm_image_addr(vm_cfg->children[j], load_addr);
    }
}

static void config_adjust_vm_image_addr(paddr_t load_addr)
{
    for (size_t i = 0; i < config.vmlist_size; i++) {
        struct vm_config* vm_cfg = config.vmlist[i];
        if (!vm_cfg->image.separately_loaded) {
            vm_cfg->image.load_addr = (vm_cfg->image.load_addr - BAO_VAS_BASE) + load_addr;
        }
        for (size_t j = 0; j < vm_cfg->children_num; j++) {
            config_adjust_child_vm_image_addr(vm_cfg->children[j], load_addr);
        }
    }
}

__attribute__((weak)) void config_mem_prot_init(paddr_t load_addr)
{
    UNUSED_ARG(load_addr);
}

void config_init(paddr_t load_addr)
{
    config_adjust_vm_image_addr(load_addr);
    config_mem_prot_init(load_addr);
}
