/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __VMM_H__
#define __VMM_H__

#include <crossconhyp.h>
#include <arch/vmm.h>
#include <vm.h>
#include <mem_prot/vmm.h>
#include <objpool.h>

struct vcpu_node {
    node_t node;
    void* data;
};

struct partition {
    spinlock_t lock;
    struct cpu_synctoken sync;
    uint64_t master;
    struct {
        struct vm* curr_vm;
        size_t ncpus;
    } init;
    struct vm* interrupts[MAX_INTERRUPT_LINES];
};
extern struct partition* parttn;
extern struct objpool nodes_pool;

void vmm_init(void);
void vmm_arch_init(void);

void vmm_io_init(void);

struct vm_install_info vmm_get_vm_install_info(struct vm_allocation* vm_alloc);
void vmm_vm_install(struct vm_install_info* install_info);

struct vm* vmm_init_dynamic(struct dynconfig* ptr_vm_config, uint64_t vm_addr);
void vmm_destroy_dynamic(struct vm* vm);

#endif /* __VMM_H__ */
