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

#include <vmm.h>
#include <vm.h>
#include <config.h>
#include <cpu.h>
#include <iommu.h>
#include <spinlock.h>
#include <fences.h>
#include <string.h>
#include <ipc.h>
#include <vmstack.h>

struct config* vm_config_ptr;

struct partition* const partition = (struct partition*)BAO_VM_BASE;

extern int cpu_different;

static void* vmm_alloc_vm_struct()
{
    size_t vm_npages = ALIGN(sizeof(vm_t), PAGE_SIZE) / PAGE_SIZE;
    void* va = mem_alloc_vpage(&cpu.as, SEC_HYP_VM, NULL, vm_npages);
    mem_map(&cpu.as, va, NULL, vm_npages, PTE_HYP_FLAGS);
    memset(va, 0, vm_npages * PAGE_SIZE);
    return va;
}

uint64_t vmm_alloc_vmid()
{
    static uint64_t id = 0;
    static spinlock_t lock = SPINLOCK_INITVAL;

    uint64_t vmid;
    spin_lock(&lock);
    vmid = ++id;  // no vmid 0
    spin_unlock(&lock);

    return vmid;
}

static vcpu_t* vmm_create_vms(vm_config_t* config, vcpu_t* parent)
{
    if (cpu.id == partition->master) {
        partition->init.curr_vm = vmm_alloc_vm_struct();
        partition->init.ncpus = 0;
    }

    if (parent) {
        cpu_sync_barrier(&parent->vm->sync);
    } else {
        cpu_sync_barrier(&partition->sync);
    }

    vm_t* vm = partition->init.curr_vm;
    vcpu_t* vcpu = NULL;

    bool assigned = false;
    bool master = false;

    spin_lock(&partition->lock);
    if ((partition->init.ncpus < config->platform.cpu_num) &&
        (1ULL << cpu.id) & config->cpu_affinity) {
        if (partition->init.ncpus == 0) master = true;
        partition->init.ncpus++;
        assigned = true;
    }
    spin_unlock(&partition->lock);

    if (parent) {
        cpu_sync_barrier(&parent->vm->sync);
    } else {
        cpu_sync_barrier(&partition->sync);
    }

    spin_lock(&partition->lock);
    if (!assigned && (partition->init.ncpus < config->platform.cpu_num)) {
        if (partition->init.ncpus == 0) master = true;
        partition->init.ncpus++;
        assigned = true;
    }
    spin_unlock(&partition->lock);

    if (assigned) {
        vcpu = vm_init(vm, config, master);
        // for (int i = 0; i < config->children_num; i++) {
        //    vm_config_t* child_config = config->children[i];
        //    vcpu_t* child = vmm_create_vms(
        //        child_config, vcpu);  // TODO: do this without recursion
        //    if (child != NULL) {
        //        node_data_t* node = objcache_alloc(&partition->nodes);
        //        node->data = child;
        //        list_append(&vcpu->children, (node_t*)node);
        //    }
        cpu_sync_barrier(&vm->sync);
        //}
    }

    return vcpu;
}

void vmm_init_dynamic(struct config* ptr_vm_config, uint64_t vm_addr)
{
    vm_t* vm = vmm_alloc_vm_struct();
    vm_config_t* enclave_config = ptr_vm_config->vmlist[0];

    vcpu_t* enclave = vm_init_dynamic(vm, enclave_config, vm_addr);
    
    node_data_t* node = objcache_alloc(&partition->nodes);
    node->data = enclave;
    list_append(&cpu.vcpu->children, (node_t*)node);

    cpu_different = cpu.id;
}

void vmm_init()
{
    if(vm_config_ptr->vmlist_size == 0){
        if(cpu.id == CPU_MASTER)
            INFO("No virtual machines to run.");
        cpu_idle();
    } 
    
    vmm_arch_init();

    volatile static struct vm_assignment {
        spinlock_t lock;
        bool master;
        size_t ncpus;
        cpumap_t cpus;
        pte_t vm_shared_table;
    } * vm_assign;

    size_t vmass_npages = 0;
    if (cpu.id == CPU_MASTER) {
        iommu_init();

        vmass_npages =
            ALIGN(sizeof(struct vm_assignment) * vm_config_ptr->vmlist_size,
                  PAGE_SIZE) /
            PAGE_SIZE;
        vm_assign = mem_alloc_page(vmass_npages, SEC_HYP_GLOBAL, false);
        if (vm_assign == NULL) ERROR("cant allocate vm assignemnt pages");
        memset((void*)vm_assign, 0, vmass_npages * PAGE_SIZE);
    }

    cpu_sync_barrier(&cpu_glb_sync);

    bool master = false;
    bool assigned = false;
    vmid_t vm_id = 0;
    struct vm_config *vm_config = NULL;

    /**
     * Assign cpus according to vm affinity.
     */
    for (size_t i = 0; i < vm_config_ptr->vmlist_size && !assigned; i++) {
        if (vm_config_ptr->vmlist[i].cpu_affinity & (1UL << cpu.id)) {
            spin_lock(&vm_assign[i].lock);
            if (!vm_assign[i].master) {
                vm_assign[i].master = true;
                vm_assign[i].ncpus++;
                vm_assign[i].cpus |= (1UL << cpu.id);
                master = true;
                assigned = true;
                vm_id = i;
            } else if (vm_assign[i].ncpus <
                       vm_config_ptr->vmlist[i].platform.cpu_num) {
                assigned = true;
                vm_assign[i].ncpus++;
                vm_assign[i].cpus |= (1UL << cpu.id);
                vm_id = i;
            }
            spin_unlock(&vm_assign[i].lock);
        }
    }

    cpu_sync_barrier(&cpu_glb_sync);

    /**
     * Assign remaining cpus not assigned by affinity.
     */
    if (assigned == false) {
        for (size_t i = 0; i < vm_config_ptr->vmlist_size && !assigned; i++) {
            spin_lock(&vm_assign[i].lock);
            if (vm_assign[i].ncpus <
                vm_config_ptr->vmlist[i].platform.cpu_num) {
                if (!vm_assign[i].master) {
                    vm_assign[i].master = true;
                    vm_assign[i].ncpus++;
                    master = true;
                    assigned = true;
                    vm_assign[i].cpus |= (1UL << cpu.id);
                    vm_id = i;
                } else {
                    assigned = true;
                    vm_assign[i].ncpus++;
                    vm_assign[i].cpus |= (1UL << cpu.id);
                    vm_id = i;
                }
            }
            spin_unlock(&vm_assign[i].lock);
        }
    }

    cpu_sync_barrier(&cpu_glb_sync);

    if (assigned) {
        vm_config = &vm_config_ptr->vmlist[vm_id];
        if (master) {
            //size_t vm_npages = NUM_PAGES(sizeof(struct vm));
            /* TODO */
            size_t vm_npages = NUM_PAGES(sizeof(struct partition));
            vaddr_t va = mem_alloc_vpage(&cpu.as, SEC_HYP_VM,
                                            (vaddr_t)BAO_VM_BASE,
                                            vm_npages);
            mem_map(&cpu.as, va, NULL, vm_npages, PTE_HYP_FLAGS);
            memset((void*)va, 0, vm_npages * PAGE_SIZE);
            vcpu_sync_init(&partition->sync, vm_assign[vm_id].ncpus);
            partition->master = cpu.id;
            objcache_init(&partition->nodes, sizeof(node_data_t), SEC_HYP_VM,
                          true);
            fence_ord_write();
            vm_assign[vm_id].vm_shared_table =
                *pt_get_pte(&cpu.as.pt, 0, (vaddr_t)BAO_VM_BASE);
        } else {
            while (vm_assign[vm_id].vm_shared_table == 0);
            pte_t* pte = pt_get_pte(&cpu.as.pt, 0, (vaddr_t)BAO_VM_BASE);
            *pte = vm_assign[vm_id].vm_shared_table;
            fence_sync_write();
        }
    }

    cpu_sync_barrier(&cpu_glb_sync);

    if (cpu.id == CPU_MASTER) {
        mem_free_vpage(&cpu.as, (vaddr_t)vm_assign, vmass_npages, true);
    }

    ipc_init(vm_config, master);

    if (assigned) {
/*
        vm_init((struct vm*)BAO_VM_BASE, vm_config, master, vm_id);
        vcpu_run(cpu.vcpu);
*/
	/* TODO */
        vcpu_t* root = vmm_create_vms(vm_config_ptr->vmlist[vm_id], NULL);
        vmstack_push(root);
        cpu_sync_barrier(&partition->sync);
        vcpu_run(root);
    } else {
        cpu_idle();
    }
}
