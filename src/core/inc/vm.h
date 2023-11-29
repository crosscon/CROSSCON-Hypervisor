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

#ifndef __VM_H__
#define __VM_H__

#include <bao.h>
#include <arch/vm.h>

#include <platform.h>
#include <mem.h>
#include <cpu.h>
#include <spinlock.h>
#include <config.h>
#include <emul.h>
#include <objcache.h>
#include <interrupts.h>
#include <bitmap.h>
#include <iommu.h>
#include <ipc.h>
#include <vmm.h>

struct vm {
    node_t node;
    vmid_t id;

    const struct vm_config* config;

    spinlock_t lock;
    struct cpu_synctoken sync;
    cpuid_t master;

    struct list vcpu_list;
    size_t cpu_num;
    cpumap_t cpus;

    struct addr_space as;

    struct vm_arch arch;

    /* TODO: OCs should be global */
    struct list emul_list;
    struct objcache emul_oc;

    struct list irq_list;
    struct objcache irq_oc;

    struct list hvc_list;
    struct objcache hvc_oc;

    struct list smc_list;
    struct objcache smc_oc;

    struct list mem_abort_list;
    struct objcache mem_abort_oc;

    struct iommu_vm iommu;

    BITMAP_ALLOC(interrupt_bitmap, MAX_INTERRUPTS);

    size_t ipc_num;
    struct ipc *ipcs;

    struct {
	vaddr_t donor_va;
	struct config* config;
    } enclave_house_keeping;
};

struct vcpu {
    node_t node;

    struct arch_regs* regs;
    struct vcpu_arch arch;

    vcpuid_t id;
    cpuid_t phys_id;
    bool active;
    enum { VCPU_OFF, VCPU_INACTIVE, VCPU_ACTIVE, VCPU_STACKED } state;

    struct vm* vm;
    struct list vmstack_children;
    struct vcpu* parent;
    struct {
	bool initialized;
    }nclv_data;

    uint8_t stack[STACK_SIZE] __attribute__((aligned(STACK_SIZE)));
};

typedef void (*sdirq_handler_t)(struct vcpu* vcpu, irqid_t int_id);
struct hndl_irq {
    size_t num;
    uint64_t irqs[159];
    sdirq_handler_t handler;
};
struct hndl_irq_node {
    node_t node;
    struct hndl_irq hndl_irq;
};

typedef int64_t (*smc_handler_t)(struct vcpu* vcpu, uint64_t smc);
struct hndl_smc {
    size_t start;
    size_t end;
    smc_handler_t handler;
};

struct hndl_smc_node {
    node_t node;
    struct hndl_smc hndl_smc;
};

typedef int64_t (*hvc_handler_t)(struct vcpu* vcpu, uint64_t hvc);
struct hndl_hvc {
    size_t start;
    size_t end;
    hvc_handler_t handler;
};
struct hndl_hvc_node {
    node_t node;
    struct hndl_hvc hndl_hvc;
};

typedef int64_t (*mem_abort_handler_t)(struct vcpu* vcpu, uint64_t addr);
struct hndl_mem_abort {
    mem_abort_handler_t handler;
};
struct hndl_mem_abort_node {
    node_t node;
    struct hndl_mem_abort hndl_mem_abort;
};

extern struct vm vm;
extern struct config* vm_config_ptr;

struct vcpu* vm_init(struct vm* vm, const struct vm_config* config, bool master, vmid_t vm_id);
void vm_init_dynamic(struct vm*, struct config*, uint64_t, vmid_t vmid);
void vm_destroy_dynamic(struct vm* vm);
void vm_start(struct vm* vm, vaddr_t entry);
struct vcpu* vm_get_vcpu(struct vm* vm, vcpuid_t vcpuid);
void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu);
void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu);
emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr);
emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr);
void vcpu_init(struct vcpu* vcpu, struct vm* vm, vaddr_t entry);
void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg);
cpumap_t vm_translate_to_pcpu_mask(struct vm* vm, cpumap_t mask, size_t len);
cpumap_t vm_translate_to_vcpu_mask(struct vm* vm, cpumap_t mask, size_t len);
struct vcpu* vcpu_get_child(struct vcpu* vcpu, int index);

static inline cpuid_t vm_translate_to_pcpuid(struct vm* vm, vcpuid_t vcpuid)
{
    ssize_t i = bitmap_find_nth((bitmap_t*)&vm->cpus, sizeof(vm->cpus) * 8,
                           vcpuid + 1, 0, true);
    if(i < 0) {
        return INVALID_CPUID;
    } else {
        return (cpuid_t)i;
    }
}

static inline vcpuid_t vm_translate_to_vcpuid(struct vm* vm, cpuid_t pcpuid)
{
    if (vm->cpus & (1UL << pcpuid)) {
        return (cpuid_t)bitmap_count((bitmap_t*)&vm->cpus, 0, pcpuid, true);
    } else {
        return INVALID_CPUID;
    }
}

static inline bool vm_has_interrupt(struct vm* vm, irqid_t int_id)
{
    return !!bitmap_get(vm->interrupt_bitmap, int_id);
}

static inline void vcpu_inject_hw_irq(struct vcpu *vcpu, irqid_t id)
{
    vcpu_arch_inject_hw_irq(vcpu, id);
}

static inline void vcpu_inject_irq(struct vcpu *vcpu, irqid_t id)
{
    vcpu_arch_inject_irq(vcpu, id);
}

/* ------------------------------------------------------------*/

void vm_arch_init(struct vm* vm, const struct vm_config* config);
void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm);
int vcpu_is_off(struct vcpu* vcpu);
void vcpu_run(struct vcpu* vcpu);
unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg);
void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val);
unsigned long vcpu_readpc(struct vcpu* vcpu);
void vcpu_writepc(struct vcpu* vcpu, unsigned long pc);
void vcpu_arch_run(struct vcpu* vcpu);
void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry);
void vcpu_save_state(struct vcpu* vcpu);
void vcpu_restore_state(struct vcpu* vcpu);

void vm_map_img_rgn(struct vm* vm, const struct vm_config* config,
                    struct mem_region* reg);

void vm_copy_img_to_rgn(struct vm* vm, const struct vm_config* config,
                        struct mem_region* reg);

void vm_map_mem_region(struct vm* vm, struct mem_region* reg);

void vm_hndl_irq_add(struct vm* vm, struct hndl_irq* irqs);

void vm_hndl_smc_add(struct vm* vm, struct hndl_smc* smcs);

void vm_hndl_hvc_add(struct vm* vm, struct hndl_hvc* hvcs);

void vm_hndl_mem_abort_add(struct vm* vm, struct hndl_mem_abort* mem_aborts);

#endif /* __VM_H__ */
