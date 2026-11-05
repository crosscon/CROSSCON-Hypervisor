/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <config.h>
#include <shmem.h>

static void config_validate_vm_ipc(const struct vm_config* vm)
{
    for (size_t i = 0; i < vm->platform.ipc_num; i++) {
        const struct ipc* ipc = &vm->platform.ipcs[i];
        if (ipc->shmem_id >= config.shmemlist_size) {
            ERROR("Invalid IPC shmem_id %lu at VM entry 0x%lx",
                (unsigned long)ipc->shmem_id, (unsigned long)vm->entry);
        }
        if (ipc->base != config.shmemlist[ipc->shmem_id].base) {
            ERROR("IPC base addr must be the same as its shmem base addr.");
        }
    }
    for (size_t i = 0; i < vm->children_num; i++) {
        config_validate_vm_ipc(vm->children[i]);
    }
}

void config_mem_prot_init(void)
{
    for (size_t i = 0; i < config.vmlist_size; i++) {
        for (size_t j = 0; j < config.vmlist[i]->platform.region_num; j++) {
            /**
             * On MPU systems all VM regions must be physical regions with 1-to-1 mapping.
             */
            config.vmlist[i]->platform.regions[j].place_phys = true;
            vaddr_t region_base = config.vmlist[i]->platform.regions[j].base;
            config.vmlist[i]->platform.regions[j].phys = region_base;
        }

        config_validate_vm_ipc(config.vmlist[i]);
    }

    for (size_t i = 0; i < config.shmemlist_size; i++) {
        /**
         * On MPU systems all shared memory regions must be physical regions with 1-to-1 mapping.
         */
        config.shmemlist[i].place_phys = true;
    }
}
