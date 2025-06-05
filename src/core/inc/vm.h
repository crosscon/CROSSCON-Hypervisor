/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __VM_H__
#define __VM_H__

#include <crossconhyp.h>
#include <arch/vm.h>

#include <mem.h>
#include <cpu.h>
#include <spinlock.h>
#include <emul.h>
#include <interrupts.h>
#include <bitmap.h>
#include <io.h>
#include <ipc.h>
#include <remio.h>

struct vm_mem_region {
    paddr_t base;
    size_t size;
    colormap_t colors;
    bool place_phys;
    paddr_t phys;
};

struct vm_dev_region {
    paddr_t pa;
    vaddr_t va;
    size_t size;
    size_t interrupt_num;
    irqid_t* interrupts;
    deviceid_t id; /* bus master id for iommu effects */
};

struct vm_platform {
    size_t cpu_num;

    size_t region_num;
    struct vm_mem_region* regions;

    size_t ipc_num;
    struct ipc* ipcs;

    size_t dev_num;
    struct vm_dev_region* devs;

    size_t remio_dev_num;
    struct remio_dev* remio_devs;

    // /**
    //  * In MPU-based platforms which might also support virtual memory
    //  * (i.e. aarch64 cortex-r) the hypervisor sets up the VM using an MPU by
    //  * default. If the user wants this VM to use the MMU they must set the
    //  * config mmu parameter to true;
    //  */
    bool mmu;

    struct arch_vm_platform arch;
};

struct vm {
    vmid_t id;

    const struct vm_config* config;

    spinlock_t lock;
    struct cpu_synctoken sync;
    cpuid_t master;

    struct vcpu* vcpus;
    size_t cpu_num;
    cpumap_t cpus;

    size_t type;

    struct addr_space as;

    struct vm_arch arch;

    struct list emul_mem_list;
    struct list emul_reg_list;

    struct list irq_list;

    struct list hvc_list;

    struct list smc_list;

    struct list mem_abort_list;

    struct vm_io io;

    BITMAP_ALLOC(interrupt_bitmap, MAX_GUEST_INTERRUPTS);

    size_t ipc_num;
    struct ipc* ipcs;

    size_t remio_dev_num;
    struct remio_dev* remio_devs;

    struct {
        vaddr_t donor_va;
        struct dynconfig* dynconfig;
    } vmdyn_house_keeping;
};

struct vcpu {
    node_t sched_node;
    node_t vmstack_node;
    node_t list_node;
    node_t vmstack_child_node;

    struct arch_regs regs;
    struct vcpu_arch arch;

    vcpuid_t id;
    cpuid_t phys_id;
    bool active;
    unsigned long first_run;
    enum { VCPU_OFF, VCPU_INACTIVE, VCPU_ACTIVE, VCPU_STACKED } state;

    spinlock_t blocked_count_lock;
    long int blocked_count;

    struct vm* vm;
    struct list vcpu_stack_lst; /* vm stack management */
    struct vcpu* root_vcpu;
    struct list vmstack_children;
    struct vcpu* parent;
    struct {
        bool initialized;
        size_t id;
    } nclv_data;
    uint8_t stack[STACK_SIZE] __attribute__((aligned(PAGE_SIZE)));
};

struct vm_allocation {
    vaddr_t base;
    size_t size;
    struct vcpu* root_vcpu;
    struct vm* vm;
    struct vcpu* vcpus;
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

typedef int64_t (*smc_handler_t)(struct vcpu* vcpu, unsigned long smc);
struct hndl_smc {
    size_t start;
    size_t end;
    smc_handler_t handler;
};

struct hndl_smc_node {
    node_t node;
    struct hndl_smc hndl_smc;
};

typedef long (*hvc_handler_t)(struct vcpu* vcpu, uint64_t hvc);
struct hndl_hvc {
    size_t start;
    size_t end;
    hvc_handler_t handler;
};
struct hndl_hvc_node {
    node_t node;
    struct hndl_hvc hndl_hvc;
};

typedef int64_t (*mem_abort_handler_t)(struct vcpu* vcpu, unsigned long addr);
struct hndl_mem_abort {
    mem_abort_handler_t handler;
};
struct hndl_mem_abort_node {
    node_t node;
    struct hndl_mem_abort hndl_mem_abort;
};

#ifndef GENERATING_DEFS
struct vm* vm_init(struct vm_allocation* vm_alloc, struct cpu_synctoken* vm_init_sync,
    const struct vm_config* config, bool master, vmid_t vm_id);
struct vm* vm_init_dynamic(struct vm_allocation*, struct vm_config*, uint64_t, vmid_t vmid, struct dynconfig* dyn_config);
void vm_destroy_dynamic(struct vm* vm);
void vm_start(struct vm* vm, vaddr_t entry);
void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu);
void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu);
emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr);
emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr);
void vcpu_init(struct vcpu* vcpu, struct vm* vm, vaddr_t entry);
void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg);
cpumap_t vm_translate_to_pcpu_mask(struct vm* vm, cpumap_t mask, size_t len);
cpumap_t vm_translate_to_vcpu_mask(struct vm* vm, cpumap_t mask, size_t len);
struct vcpu* vcpu_get_child(struct vcpu* vcpu, int index);

static inline struct vcpu* vm_get_vcpu(struct vm* vm, vcpuid_t vcpuid)
{
    if (vcpuid < vm->cpu_num) {
        return &vm->vcpus[vcpuid];
    }
    return NULL;
}

static inline cpuid_t vm_translate_to_pcpuid(struct vm* vm, vcpuid_t vcpuid)
{
    struct vcpu* vcpu = vm_get_vcpu(vm, vcpuid);

    if (vcpu == NULL) {
        return INVALID_CPUID;
    } else {
        return vcpu->phys_id;
    }
}

static inline vcpuid_t vm_translate_to_vcpuid(struct vm* vm, cpuid_t pcpuid)
{
    if (vm->cpus & (1UL << pcpuid)) {
        return (cpuid_t)bit_count(vm->cpus & BIT_MASK(0, pcpuid)) - 1;
    } else {
        return INVALID_CPUID;
    }
}

static inline bool vm_has_interrupt(struct vm* vm, irqid_t int_id)
{
    return !!bitmap_get(vm->interrupt_bitmap, int_id);
}

static inline void vcpu_inject_hw_irq(struct vcpu* vcpu, irqid_t id)
{
    vcpu_arch_inject_hw_irq(vcpu, id);
}

static inline void vcpu_inject_irq(struct vcpu* vcpu, irqid_t id)
{
    vcpu_arch_inject_irq(vcpu, id);
}

static inline void vcpu_block(struct vcpu* vcpu)
{
    // TODO check for overflows
    vcpu->blocked_count += 1;
}

static inline void vcpu_unblock(struct vcpu* vcpu)
{
    if (vcpu->blocked_count > 0) {
        vcpu->blocked_count -= 1;
    }
}

static inline bool vcpu_is_blocked(struct vcpu* vcpu)
{
    return vcpu->blocked_count > 0;
}

static inline void vcpu_kill(struct vcpu* vcpu)
{
    vcpu->blocked_count = -1;
}

static inline bool vcpu_is_dead(struct vcpu* vcpu)
{
    return vcpu->blocked_count < 0;
}

static inline struct vcpu* vcpu_current(void)
{
    return cpu()->vcpu;
}

static inline struct vcpu* vcpu_next(void)
{
    return cpu()->vcpu;
}

static inline struct vcpu* vcpu_set_next(struct vcpu* vcpu)
{
    return cpu()->next_vcpu = vcpu;
}

void vcpu_context_switch(void);

/* ------------------------------------------------------------*/

void vm_mem_prot_init(struct vm* vm, const struct vm_config* config);

/* ------------------------------------------------------------*/

void vm_arch_init(struct vm* vm, const struct vm_config* config);
void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm);
void vcpu_run(struct vcpu* vcpu);
unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg);
void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val);
unsigned long vcpu_readpc(struct vcpu* vcpu);
void vcpu_writepc(struct vcpu* vcpu, unsigned long pc);
void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry);
bool vcpu_arch_is_on(struct vcpu* vcpu);
void vm_arch_allow_mmio_access(struct vm* vm, struct vm_dev_region* dev);
void vcpu_save_state(struct vcpu* vcpu);
void vcpu_restore_state(struct vcpu* vcpu);

void vm_hndl_irq_add(struct vm* vm, struct hndl_irq* irqs);

void vm_hndl_smc_add(struct vm* vm, struct hndl_smc* smcs);

void vm_hndl_hvc_add(struct vm* vm, struct hndl_hvc* hvcs);

void vm_hndl_mem_abort_add(struct vm* vm, struct hndl_mem_abort* mem_aborts);
void vcpu_context_switch(void);
#endif /* GENERATING_DEFS */

#endif /* __VM_H__ */
