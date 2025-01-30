/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include "arch/spinlock.h"
#include "objpool.h"
#include <vmm.h>
#include <vm.h>
#include <config.h>
#include <dynconfig.h>
#include <cpu.h>
#include <spinlock.h>
#include <fences.h>
#include <string.h>
#include <shmem.h>
#include <mem.h>
#include <config_defs.h>
#include <vmstack.h>

/* CROSSCON TODO this over-allocates */
struct partition partition[CONFIG_PARTITION_NUM];

OBJPOOL_ALLOC(nodes_pool, struct node_data, 10 /* CROSSCON TODO */);

static struct vm_assignment {
    spinlock_t lock;
    struct cpu_synctoken root_sync;
    bool master;
    size_t ncpus;
    cpumap_t cpus;
    struct vm_allocation vm_alloc;
    struct vm_install_info vm_install_info;
    volatile bool install_info_ready;
    size_t partition_id;
    struct vm_config *vm_config;
} vm_assign[CONFIG_VM_NUM];

/* needed for dynamic VMs */
static vmid_t vmm_alloc_vmid(void)
{
    static uint64_t id = CONFIG_VM_NUM;
    static spinlock_t lock = SPINLOCK_INITVAL;

    uint64_t vmid;
    spin_lock(&lock);
    vmid = id++;
    spin_unlock(&lock);

    return vmid;
}

static void vmm_init_child_vm_assign(struct vm_config* vm_cfg, size_t *idx, size_t part_id)
{
    if(vm_cfg == NULL)
        ERROR("%s", __func__);

    vmid_t vm_id = *idx;
    vm_cfg->vm_id = vm_id;
    vm_assign[vm_id].partition_id = part_id;
    vm_assign[vm_id].vm_config = vm_cfg;

    *idx = (*idx) + 1;
    for (size_t i = 0; i < vm_cfg->children_num; i++) { //como esta funçao é recursiva, ela é executada children_num vezes?
        struct vm_config *tmp_vm_cfg = vm_cfg->children[i];
        vmm_init_child_vm_assign(tmp_vm_cfg, idx, part_id);
    }
}

static void vmm_init_assign(void)
{
    size_t idx = 0;
    for (size_t p = 0; p < CONFIG_PARTITION_NUM; p++) {
        vmm_init_child_vm_assign(config.vmlist[p], &idx, p);
    }
}
static inline bool vmm_assign_pcpu_aff(vmid_t vm_id, struct vm_config *vm_cfg, bool *master)
{
    bool assigned = false;
    if (!vm_assign[vm_id].master) {
        vm_assign[vm_id].master = true;
        vm_assign[vm_id].ncpus++;
        vm_assign[vm_id].cpus |= (1UL << cpu()->id);
        *master = true;
        assigned = true;
    } else if (vm_assign[vm_id].ncpus < vm_cfg->platform.cpu_num) {
        assigned = true;
        vm_assign[vm_id].ncpus++;
        vm_assign[vm_id].cpus |= (1UL << cpu()->id);
    }
    return assigned;
}

static inline bool vmm_assign_pcpu(vmid_t vm_id, struct vm_config *vm_cfg, bool *master)
{
    bool assigned = false;
    if (vm_assign[vm_id].ncpus < vm_cfg->platform.cpu_num) {
        if (!vm_assign[vm_id].master) {
            vm_assign[vm_id].master = true;
            vm_assign[vm_id].ncpus++;
            *master = true;
            assigned = true;
            vm_assign[vm_id].cpus |= (1UL << cpu()->id);
        } else {
            assigned = true;
            vm_assign[vm_id].ncpus++;
            vm_assign[vm_id].cpus |= (1UL << cpu()->id);
        }
    }

    return assigned;
}

static bool vmm_assign_vcpu(bool* master, vmid_t* vm_id)
{
    bool assigned = false;
    *master = false;
    /* Assign cpus according to partition vm affinity. */
    for (size_t i = 0; i < config.vmlist_size && !assigned; i++) {
        if (config.vmlist[i]->cpu_affinity & (1UL << cpu()->id)) {
            vmid_t tmp_vm_id = config.vmlist[i]->vm_id;
            spin_lock(&vm_assign[tmp_vm_id].lock);
            if (!vm_assign[tmp_vm_id].master) {
                vm_assign[tmp_vm_id].master = true;
                vm_assign[tmp_vm_id].ncpus++;
                vm_assign[tmp_vm_id].cpus |= (1UL << cpu()->id);
                *master = true;
                partition[tmp_vm_id].master = cpu()->id;
                assigned = true;
                *vm_id = tmp_vm_id;
            } else if (vm_assign[tmp_vm_id].ncpus < config.vmlist[tmp_vm_id]->platform.cpu_num) {
                assigned = true;
                vm_assign[tmp_vm_id].ncpus++;
                vm_assign[tmp_vm_id].cpus |= (1UL << cpu()->id);
                *vm_id = tmp_vm_id;
            }
            spin_unlock(&vm_assign[tmp_vm_id].lock);
        }
    }

    cpu_sync_barrier(&cpu_glb_sync);

    /* Assign remaining cpus not assigned by affinity. */
    if (assigned == false) {
        for (size_t i = 0; i < config.vmlist_size && !assigned; i++) {
            vmid_t tmp_vm_id = config.vmlist[i]->vm_id;
            spin_lock(&vm_assign[tmp_vm_id].lock);
            if (vm_assign[tmp_vm_id].ncpus < config.vmlist[i]->platform.cpu_num) {
                if (!vm_assign[tmp_vm_id].master) {
                    vm_assign[tmp_vm_id].master = true;
                    vm_assign[tmp_vm_id].ncpus++;
                    *master = true;
                    partition[tmp_vm_id].master = cpu()->id;
                    assigned = true;
                    vm_assign[tmp_vm_id].cpus |= (1UL << cpu()->id);
                    *vm_id = i;
                } else {
                    assigned = true;
                    vm_assign[tmp_vm_id].ncpus++;
                    vm_assign[tmp_vm_id].cpus |= (1UL << cpu()->id);
                    *vm_id = i;
                }
            }
            spin_unlock(&vm_assign[tmp_vm_id].lock);
        }
    }

    return assigned;
}

static uint64_t vmm_alloc_vmid(void);

static bool vmm_alloc_vm(struct vm_allocation* vm_alloc, struct vm_config* vm_config)
{
    /**
     * We know that we will allocate a block aligned to the PAGE_SIZE, which is guaranteed to
     * fulfill the alignment of all types. However, to guarantee the alignment of all fields, when
     * we calculate the size of a field in the vm_allocation struct, we must align the previous
     * total size calculated until that point, to the alignment of the type of the next field.
     */

    size_t total_size = sizeof(struct vm);
    size_t vcpus_offset = ALIGN(total_size, _Alignof(struct vcpu));
    total_size = vcpus_offset + (vm_config->platform.cpu_num * sizeof(struct vcpu));
    total_size = ALIGN(total_size, PAGE_SIZE);

    void* allocation = mem_alloc_page(NUM_PAGES(total_size), SEC_HYP_VM, MEM_ALIGN_NOT_REQ);
    if (allocation == NULL) {
        return false;
    }
    memset((void*)allocation, 0, total_size);

    vm_alloc->base = (vaddr_t)allocation;
    vm_alloc->size = total_size;
    vm_alloc->vm = (struct vm*)vm_alloc->base;
    vm_alloc->vcpus = (struct vcpu*)(vm_alloc->base + vcpus_offset);

    return true;
}

static void vmm_free_vm(struct vm* vm)
{
    /* CROSSCON TODO take into account vcpus as well */
    size_t n = NUM_PAGES(sizeof(struct vm));
    memset((void*)vm, 0, n * PAGE_SIZE);
    mem_unmap(&cpu()->as, (vaddr_t)vm, n, true);
}

static struct vm_allocation* vmm_alloc_install_vm(vmid_t vm_id, bool master, struct vm_config* vm_cfg)
{
    struct vm_allocation* vm_alloc = &vm_assign[vm_id].vm_alloc;
    struct vm_config* vm_config = vm_cfg;
    if (master) {
        if (!vmm_alloc_vm(vm_alloc, vm_config)) {
            ERROR("Failed to allocate vm internal structures");
        }
        vm_assign[vm_id].vm_install_info = vmm_get_vm_install_info(vm_alloc);
        fence_ord_write();
        vm_assign[vm_id].install_info_ready = true;
    } else {
        while (!vm_assign[vm_id].install_info_ready) { }
        fence_ord_read();
        vmm_vm_install(&vm_assign[vm_id].vm_install_info);
    }

    return vm_alloc;
}

static bool vmm_partition_cpu_assign_aff(struct partition* cur_parttn, struct vm_config *vm_cfg, bool *master)
{
    spin_lock(&cur_parttn->lock);
    bool assigned = false;

    bool all_cpus_init = (cur_parttn->init.ncpus == vm_cfg->platform.cpu_num);
    bool pcpu_is_in_aff = (1ULL << cpu()->id) & vm_cfg->cpu_affinity;
    if (!all_cpus_init && pcpu_is_in_aff) {
        if (cur_parttn->init.ncpus == 0){
            /* if we are the first in the affinity we are master */
            *master = true;
        } else {
            /* master is the master for this vm */
        }

        cur_parttn->init.ncpus++;
        assigned = true;
    }
    spin_unlock(&cur_parttn->lock);
    return assigned;
}

static bool vmm_partition_cpu_assign(struct partition* cur_parttn, struct vm_config *vm_cfg, bool *master)
{
    spin_lock(&cur_parttn->lock);
    bool assigned = false;
    if (cur_parttn->init.ncpus < vm_cfg->platform.cpu_num) {
        if (cur_parttn->init.ncpus == 0) {
             *master = true;
        }
        cur_parttn->init.ncpus++;
        assigned = true;
    }
    spin_unlock(&cur_parttn->lock);
    return assigned;
}

static struct vm* vmm_create_vms(struct vm_config* vm_config, struct vcpu* parent, bool master)
{
    vmid_t cur_vm_id = vm_config->vm_id;
    size_t partition_id = vm_assign[cur_vm_id].partition_id;
    struct partition *cur_parttn = &partition[partition_id];
    struct vm_allocation* vm_alloc = vmm_alloc_install_vm(cur_vm_id, master, vm_config);

    /* CROSSCON TODO maybe just use master */
    if (cpu()->id == cur_parttn->master) {
        vm_alloc = vmm_alloc_install_vm(cur_vm_id, master, vm_config);
        /* partition init state for this vm */
        cur_parttn->init.curr_vm = vm_alloc->vm;
        cur_parttn->init.ncpus = 0;
    }

    if (parent) {
        /* wait for vm to be allocated */
        cpu_sync_barrier(&parent->vm->sync);
    } else {
        /* wait for vm to be allocated */
        cpu_sync_barrier(&cur_parttn->sync);
    }

    bool assigned = false;
    bool vm_master = false;

    assigned = vmm_partition_cpu_assign_aff(cur_parttn, vm_config, &vm_master);

    if (parent) {
        cpu_sync_barrier(&parent->vm->sync);
    } else {
        cpu_sync_barrier(&cur_parttn->sync);
    }

    if(!assigned){
        assigned = vmm_partition_cpu_assign(cur_parttn, vm_config, &vm_master);
    }


    struct vm *vm = NULL;
    if(assigned){
        vm = vm_init(vm_alloc, vm_config, master, vm_config->vm_id);
        struct vcpu *vcpu = cpu_get_vcpu(vm->id);

        for(size_t i = 0; i < vm_config->children_num; i++){
            struct vm_config* child_config = vm_config->children[i];
            //CROSSCON TODO: do this without recursion
            struct vm* child_vm = vmm_create_vms(child_config, vcpu, master);
            if(child_vm != NULL){
                struct vcpu *child_vcpu = cpu_get_vcpu(child_vm->id);
                struct node_data* node = objpool_alloc(&nodes_pool);
                node->data = child_vcpu;
                INFO("VM %u is parent of VM %u\n", vcpu->vm->id, child_vcpu->vm->id);
                list_push(&vcpu->vmstack_children, (node_t*)node);
            }
            cpu_sync_barrier(&vm->sync);
        }
    }

    return vm;
}


struct vm* vmm_init_dynamic(struct dynconfig* dyn_config, uint64_t vm_addr)
{
    /* CROSSCON TODO: support multicore dynamic VMs */
    vmid_t vmid = vmm_alloc_vmid();
    struct vm_config *vm_cfg = &dyn_config->vm_cfg;
    struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vmid, true, vm_cfg);
    struct vm *dyn_vm = vm_init_dynamic(vm_alloc, vm_cfg, vm_addr, vmid, dyn_config);

    /* CROSSCON TODO */
    struct node_data* node = objpool_alloc(&nodes_pool);
    struct vcpu* child = cpu_get_vcpu(dyn_vm->id);
    node->data = child;
    list_push(&cpu()->vcpu->vmstack_children, (node_t*)node);

    return dyn_vm;
}

void vmm_destroy_dynamic(struct vm *vm)
{
    list_foreach(cpu()->vcpu->vmstack_children, struct node_data, node){
	struct vcpu* child = node->data;
	if(child->vm == vm){
            /* CROSSCON TODO remove recursively */
	    list_rm(&cpu()->vcpu->vmstack_children, (node_t*)node);
	    objpool_free(&nodes_pool, node);
	}
    }

    vm_destroy_dynamic(vm);
    vmm_free_vm(vm);
}

static struct vcpu* vmm_create_vm(struct vm_config* vm_config, vmid_t vm_id, bool master,
    struct vcpu* root_vcpu)
{
    struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vm_config, vm_id, master);
    struct vcpu* tmp_root = NULL;

    vm_alloc->root_vcpu = root_vcpu;

    struct vcpu* vcpu = vm_init(vm_alloc, &vm_assign[vm_id].root_sync, vm_config, master, vm_id);
    for (size_t i = 0; i < vm_config->children_num; i++) {
        vmid_t child_vmid = vmm_config_to_vmid(vm_config->children[i]);
        if (vm_assign[child_vmid].cpus & (1ULL << cpu()->id)) {
            if (!root_vcpu) {
                tmp_root = vcpu;
            } else {
                tmp_root = root_vcpu;
            }

            struct vcpu* child_vcpu =
                vmm_create_vm(vm_config->children[i], child_vmid, master, tmp_root);

            INFO("VM %u is parent of VM %u\n", vcpu->vm->id, child_vcpu->vm->id);
            list_push(&vcpu->vmstack_children, &child_vcpu->vmstack_child_node);
        }
    }

    return vcpu;
}

void vmm_init()
{
    vmm_arch_init();
    vmm_io_init();
    shmem_init();
    remio_init();

    if (cpu_is_master()) {
        for (size_t i = 0; i < CONFIG_VM_NUM; i++) {
            vm_assign[i].lock = SPINLOCK_INITVAL;
            cpu_sync_init(&vm_assign[i].root_sync, config.vmlist[i].platform.cpu_num);
            objpool_init(&nodes_pool);
            vmm_allocate_vmids();
        }
    }

    cpu_sync_barrier(&cpu_glb_sync);

    bool master = false;
    vmid_t vm_id = INVALID_VMID;
    if (vmm_assign_vcpu(&master, &vm_id)) {
        // struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vm_id, master);
        // struct vm_config* vm_config = &config.vmlist[vm_id];
        // struct vm* vm = vm_init(vm_alloc, &vm_assign[vm_id].root_sync, vm_config, master, vm_id);
        vmm_create_vm(vm_config_by_id_table[vm_id], vm_id, master, NULL);
        cpu_sync_barrier(&vm->sync);
        vmstack_push(cpu()->vcpu);
        vcpu_run(cpu()->vcpu);
    } else {
        cpu_powerdown();
    }
}
