/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <ipc.h>

#include <cpu.h>
#include <vmm.h>
#include <hypercall.h>
#include <config.h>
#include <shmem.h>

enum { IPC_NOTIFY };

union ipc_msg_data {
    struct {
        uint32_t shmem_id;
        uint32_t event_id;
    };
    uint64_t raw;
};

static struct ipc* ipc_find_by_shmemid(struct vm* vm, size_t shmem_id)
{
    struct ipc* ipc_obj = NULL;

    for (size_t i = 0; i < vm->ipc_num; i++) {
        if (vm->ipcs[i].shmem_id == shmem_id) {
            ipc_obj = &vm->ipcs[i];
            break;
        }
    }

    return ipc_obj;
}

static void notify_local_vms(unsigned long shmem_id, unsigned long event_id);
static void notify_remote_vms(unsigned long shmem_id, unsigned long event_id);

static void ipc_handler(uint32_t event, uint64_t data)
{
    union ipc_msg_data ipc_data = { .raw = data };
    switch (event) {
        case IPC_NOTIFY:
            notify_local_vms(ipc_data.shmem_id, ipc_data.event_id);
            break;
        default:
            WARNING("Unknown IPC IPI event");
            break;
    }
}

CPU_MSG_HANDLER(ipc_handler, IPC_CPUMSG_ID)

static void notify_local_vms(unsigned long shmem_id, unsigned long event_id)
{
    struct vcpu* vcpu = NULL;
    list_foreach (cpu()->vcpu_list, node_t, node) {
        vcpu = CONTAINER_OF(struct vcpu, cpu_vcpu_list_node, node);
        if (vcpu == cpu()->vcpu) {
            continue;
        }

        struct ipc* ipc = ipc_find_by_shmemid(vcpu->vm, shmem_id);
        if (ipc) {
            if (ipc->master != cpu()->id) {
                continue;
            }

            if (event_id >= ipc->interrupt_num) {
                ERROR("ipc event out of range");
            }
            irqid_t irq_id = ipc->interrupts[event_id];
            vcpu_inject_irq(vcpu, irq_id);
        }
    }
}

static void notify_remote_vms(unsigned long shmem_id, unsigned long event_id)
{
    struct shmem* shmem = shmem_get(shmem_id);
    cpumap_t ipc_cpu_masters = shmem->cpu_masters & ~cpu()->vcpu->vm->cpus;
    union ipc_msg_data data = {
        .shmem_id = (uint32_t)shmem_id,
        .event_id = (uint32_t)event_id,
    };
    struct cpu_msg msg = { (uint32_t)IPC_CPUMSG_ID, IPC_NOTIFY, data.raw };

    for (size_t i = 0; i < platform.cpu_num; i++) {
        if (ipc_cpu_masters & (1ULL << i)) {
            cpu_send_msg(i, &msg);
        }
    }
}

static void notify_ipc(unsigned long shmem_id, unsigned long event_id)
{
    notify_local_vms(shmem_id, event_id);
    notify_remote_vms(shmem_id, event_id);
}

long int ipc_hypercall(void)
{
    unsigned long ipc_id = hypercall_get_arg(cpu()->vcpu, 0);
    unsigned long ipc_event = hypercall_get_arg(cpu()->vcpu, 1);

    long int ret = -HC_E_SUCCESS;

    struct shmem* shmem = NULL;
    bool valid_ipc_obj = ipc_id < cpu()->vcpu->vm->ipc_num;
    if (valid_ipc_obj) {
        shmem = shmem_get(cpu()->vcpu->vm->ipcs[ipc_id].shmem_id);
    }
    bool valid_shmem = shmem != NULL;

    if (valid_ipc_obj && valid_shmem) {
        unsigned long shmem_id = (uint32_t)cpu()->vcpu->vm->ipcs[ipc_id].shmem_id;
        unsigned long event_id = (uint32_t)ipc_event;

        notify_ipc(shmem_id, event_id);

    } else {
        ret = -HC_E_INVAL_ARGS;
    }

    return ret;
}
