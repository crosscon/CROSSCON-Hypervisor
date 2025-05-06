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

enum ipc_cpu_origin {
    ORIGIN_THIS_CPU,
    ORIGIN_OTHER_CPU
};

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

static void notify_local_vms(struct vcpu* vcpu, unsigned long shmem_id, unsigned long event_id, enum ipc_cpu_origin origin)
{
    struct vcpu* vcpu_tmp = NULL;
    list_foreach (cpu()->vcpu_lst, node_t, node) {
        vcpu_tmp = CONTAINER_OF(struct vcpu, list_node, node);
        if (origin == ORIGIN_THIS_CPU && vcpu_tmp == vcpu) {
            continue;
        }

        struct ipc* ipc = ipc_find_by_shmemid(vcpu_tmp->vm, shmem_id);
        if (ipc) {
            if (ipc->master != cpu()->id) {
                continue;
            }

            if (event_id >= ipc->interrupt_num) {
                ERROR("ipc event out of range");
            }
            irqid_t irq_id = ipc->interrupts[event_id];
            vcpu_inject_irq(vcpu_tmp, irq_id);
        }
    }
}

static void ipc_handler(uint32_t event, uint64_t data)
{
    union ipc_msg_data ipc_data = { .raw = data };
    switch (event) {
        case IPC_NOTIFY:
            notify_local_vms(cpu()->vcpu, ipc_data.shmem_id, ipc_data.event_id, ORIGIN_OTHER_CPU);
            break;
        default:

            WARNING("Unknown IPC IPI event\n");
            break;
    }
}

CPU_MSG_HANDLER(ipc_handler, IPC_CPUMSG_ID)

static void notify_remote_vms(struct vcpu* vcpu, unsigned long shmem_id, unsigned long event_id)
{
    struct shmem* shmem = shmem_get(shmem_id);
    cpumap_t ipc_cpu_masters = shmem->cpu_masters & ~vcpu->vm->cpus;
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

static void notify_ipc(struct vcpu* vcpu, unsigned long shmem_id, unsigned long event_id, enum ipc_cpu_origin origin)
{
    notify_local_vms(vcpu, shmem_id, event_id, origin);
    notify_remote_vms(vcpu, shmem_id, event_id);
}

long int ipc_hypercall(struct vcpu* vcpu)
{
    unsigned long ipc_id = hypercall_get_arg(vcpu, 0);
    unsigned long ipc_event = hypercall_get_arg(vcpu, 1);

    long int ret = -HC_E_SUCCESS;

    struct shmem* shmem = NULL;
    bool valid_ipc_obj = ipc_id < vcpu->vm->ipc_num;
    if (valid_ipc_obj) {
        shmem = shmem_get(vcpu->vm->ipcs[ipc_id].shmem_id);
    }
    bool valid_shmem = shmem != NULL;

    if (valid_ipc_obj && valid_shmem) {
        unsigned long shmem_id = (uint32_t)vcpu->vm->ipcs[ipc_id].shmem_id;
        unsigned long event_id = (uint32_t)ipc_event;

        notify_ipc(vcpu, shmem_id, event_id, ORIGIN_THIS_CPU);

    } else {
        ret = -HC_E_INVAL_ARGS;
    }

    return ret;
}
