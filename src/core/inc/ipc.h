/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef IPC_H
#define IPC_H

#include <crossconhyp.h>
#include <mem.h>
#include <vm.h>

struct ipc {
    paddr_t base;
    size_t size;
    size_t shmem_id;
    cpuid_t master;
    size_t interrupt_num;
    irqid_t* interrupts;
};

struct vm_config;

long int ipc_hypercall(struct vcpu* vcpu);

#endif /* IPC_H */
