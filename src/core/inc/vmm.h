/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __VMM_H__
#define __VMM_H__

#include <bao.h>
#include <cpu.h>
#include <vm.h>
#include <list.h>
#include <objcache.h>
#include <interrupts.h>

struct vcpu_node{
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
    struct objcache nodes;
    struct vm* interrupts[MAX_INTERRUPTS];
} extern* const partition;

void vmm_init();
struct vm* vmm_init_dynamic(struct config*, uint64_t);
void vmm_destroy_dynamic(struct vm *vm);
void vmm_arch_init();
uint64_t vmm_alloc_vmid();

#endif /* __VMM_H__ */
