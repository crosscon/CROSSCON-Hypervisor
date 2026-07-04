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
#if (IRQC != AIA)
#include <arch/sysregs.h>
#endif


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
} vm_assign[CONFIG_VM_NUM];

struct vm_config* vm_config_by_id_table[CONFIG_VM_NUM];

/* needed for dynamic VMs */
static vmid_t vmm_alloc_vmid(void)
{
    static vmid_t id = CONFIG_VM_NUM;
    static spinlock_t lock = SPINLOCK_INITVAL;

    vmid_t vmid;
    spin_lock(&lock);
    vmid = id++;
    spin_unlock(&lock);

    return vmid;
}

static vmid_t vmm_config_to_vmid(struct vm_config* vm_config)
{
    vmid_t vm_id = INVALID_VMID;

    for (size_t i = 0; i < CONFIG_VM_NUM; i++) {
        if (vm_config == vm_config_by_id_table[i]) {
            vm_id = i;
            break;
        }
    }

    return vm_id;
}

static size_t max_vcpu_per_cpu(void)
{
    size_t vcpu_num = 0;
    size_t exlusive_cpu_num = 0;

    for (size_t i = 0; i < config.vmlist_size; i++) {
        vcpu_num += config.vmlist[i]->platform.cpu_num;
        if (config.vmlist[i]->cpu_exclusivity) {
            exlusive_cpu_num += config.vmlist[i]->platform.cpu_num;
        }
    }

    size_t shared_cpu_num = platform.cpu_num - exlusive_cpu_num;
    size_t non_exclusive_vcpu_num = vcpu_num - exlusive_cpu_num;
    size_t max_vcpu = (non_exclusive_vcpu_num / shared_cpu_num) +
        ((non_exclusive_vcpu_num % shared_cpu_num) > 0 ? 1 : 0);

    return max_vcpu;
}

static void vmm_assign_child_vcpus(struct vm_config* vm_config)
{
    vmid_t parent_vmid = vmm_config_to_vmid(vm_config);
    cpumap_t parent_cpus = vm_assign[parent_vmid].cpus;

    for (size_t i = 0; i < vm_config->children_num; i++) {
        struct vm_config* child_config = vm_config->children[i];
        size_t parent_num_cpus = vm_config->platform.cpu_num;
        size_t child_num_cpus = child_config->platform.cpu_num;
        if (child_num_cpus > parent_num_cpus) {
            ERROR("Trying to assign more CPUs to a child VM than to its parent");
        }
        vmid_t child_vmid = vmm_config_to_vmid(child_config);

        size_t child_cpu_affinity = 0;
        if (child_config->cpu_affinity == 0) {
            child_cpu_affinity = parent_cpus;
        } else {
            child_cpu_affinity = child_config->cpu_affinity;
        }
        for (size_t j = 0; (j < PLAT_CPU_NUM) && (vm_assign[child_vmid].ncpus < child_num_cpus);
             j++) {
            if (child_cpu_affinity & parent_cpus & (1UL << j)) {
                vm_assign[child_vmid].cpus |= (1UL << j);
                vm_assign[child_vmid].ncpus += 1;
            }
        }
        if (vm_assign[child_vmid].ncpus < child_num_cpus) {
            ERROR("could not assign vcpus to vm");
        }

        for (size_t j = 0; j < child_config->children_num; j++) {
            vmm_assign_child_vcpus(child_config);
        }
    }
}

static bool vmm_assign_vcpus(void)
{
    size_t max_vcpus = max_vcpu_per_cpu();
    cpumap_t exclusive_cpus = 0;
    size_t cpu_vcpu_count[PLAT_CPU_NUM] = { 0 };

    static const struct {
        bool find_exclusive;
        bool assign_affinity;
    } cpu_search_params[4] = {
        { true, true },
        { true, false },
        { false, true },
        { false, false },
    };

    for (size_t k = 0; k < 4; k++) {
        for (size_t i = 0; i < config.vmlist_size; i++) {
            struct vm_config* vm_config = config.vmlist[i];
            vmid_t vm_id = vmm_config_to_vmid(vm_config);
            struct vm_assignment* vm_assignment = &vm_assign[vm_id];
            size_t vm_cpu_num = vm_config->platform.cpu_num;

            if (cpu_search_params[k].find_exclusive && !vm_config->cpu_exclusivity) {
                continue;
            }

            // For each physical cpu try to assign it one of this VM's vcpu
            for (size_t j = 0; (j < PLAT_CPU_NUM) && (vm_assignment->ncpus < vm_cpu_num); j++) {
                // If this cpu was already assigned a vCPU in the same VM, skip it
                if (bit_get(vm_assignment->cpus, j)) {
                    continue;
                }

                // If we are looking for exlucsive cpus and this was already assigned, skip it
                if (cpu_search_params[k].find_exclusive && (cpu_vcpu_count[j] > 0)) {
                    continue;
                }

                // If this cpu was already exclusively assigned, skip it
                if (bit_get(exclusive_cpus, j)) {
                    continue;
                }

                // If this cpu has no affinity to the VM and was already assigned the maximum
                // number of vcpus allowed, skip it
                if (!cpu_search_params[k].assign_affinity && cpu_vcpu_count[j] >= max_vcpus) {
                    continue;
                }

                if (!cpu_search_params[k].assign_affinity || bit_get(vm_config->cpu_affinity, j)) {
                    vm_assignment->cpus |= 1UL << j;
                    vm_assignment->ncpus += 1;
                    cpu_vcpu_count[j] += 1;
                    if (cpu_search_params[k].find_exclusive) {
                        exclusive_cpus |= 1UL << j;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < config.vmlist_size; i++) {
        vmm_assign_child_vcpus(config.vmlist[i]);
    }

    return true;
}

// static bool vmm_assign_vcpu(bool* master, vmid_t* vm_id)
// {
//     bool assigned = false;
//     *master = false;
//     /* Assign cpus according to vm affinity. */
//     for (size_t i = 0; i < config.vmlist_size && !assigned; i++) {
//         if (config.vmlist[i]->cpu_affinity & (1UL << cpu()->id)) {
//             spin_lock(&vm_assign[i].lock);
//             if (!vm_assign[i].master) {
//                 vm_assign[i].master = true;
//                 vm_assign[i].ncpus++;
//                 vm_assign[i].cpus |= (1UL << cpu()->id);
//                 *master = true;
//                 assigned = true;
//                 *vm_id = i;
//                 //TODO: não testei esta funcionalidade com o afinility 
//                 for (size_t j = 0; j < config.vmlist_size; j++) { //acrescentei aqui para apenas 1 cpus fazer esta atribuição
//                     vmm_assign_child_vcpus(config.vmlist[j]);
//                 }
//             } else if (vm_assign[i].ncpus < config.vmlist[i]->platform.cpu_num) {
//                 assigned = true;
//                 vm_assign[i].ncpus++;
//                 vm_assign[i].cpus |= (1UL << cpu()->id);
//                 *vm_id = i;
//             }
//             spin_unlock(&vm_assign[i].lock);
//         }
//     }

//     cpu_sync_barrier(&cpu_glb_sync);

//     /* Assign remaining cpus not assigned by affinity. */
//     if (assigned == false) {
//         for (size_t i = 0; i < config.vmlist_size && !assigned; i++) {
//             spin_lock(&vm_assign[i].lock);
//             if (vm_assign[i].ncpus < config.vmlist[i]->platform.cpu_num) {
//                 if (!vm_assign[i].master) {
//                     vm_assign[i].master = true;
//                     vm_assign[i].ncpus++;
//                     *master = true;
//                     assigned = true;
//                     vm_assign[i].cpus |= (1UL << cpu()->id);
//                     *vm_id = i;
//                     for (size_t j = 0; j < config.vmlist_size; j++) { //acrescentei aqui para apenas 1 cpus fazer esta atribuição
//                         vmm_assign_child_vcpus(config.vmlist[j]);
//                     }
//                 } else {
//                     assigned = true;
//                     vm_assign[i].ncpus++;
//                     vm_assign[i].cpus |= (1UL << cpu()->id);
//                     *vm_id = i;
//                 }
//             }
//             spin_unlock(&vm_assign[i].lock);
//         }
//     }

//     return assigned;
// }

static void vmm_allocate_vmid_rec(struct vm_config* vm_config)
{
    static vmid_t next_vmid = 0;
    vmid_t vm_id = next_vmid++;
    vm_config_by_id_table[vm_id] = vm_config;
    for (size_t i = 0; i < vm_config->children_num; i++) {
        vmm_allocate_vmid_rec(vm_config->children[i]);
    }
}

static void vmm_allocate_vmids(void)
{
    for (size_t i = 0; i < config.vmlist_size; i++) {
        vmm_allocate_vmid_rec(config.vmlist[i]);
    }
}

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

static struct vm_allocation* vmm_alloc_install_vm(struct vm_config* vm_config, vmid_t vm_id,
    bool master)
{
    struct vm_allocation* vm_alloc = &vm_assign[vm_id].vm_alloc;
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
void vmm_create_vm(struct vm_config* vm_config, vmid_t vm_id, bool master,  struct cpu_synctoken* vm_init_sync, struct vcpu* root_vcpu);
void vmm_create_vm(struct vm_config* vm_config, vmid_t vm_id, bool master,  struct cpu_synctoken* vm_init_sync, struct vcpu* root_vcpu)
{
    struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vm_config, vm_id, master);
    struct vcpu* tmp_root = NULL;

    vm_alloc->root_vcpu = root_vcpu;
    
    //struct vm* vm = vm_init(vm_alloc, vm_init_sync, vm_config, master, vm_id);
    //cpu_sync_barrier(&vm->sync); //ao inves de vm->sync coloquei uma vm_alloc->vm->sync para poder retornar o valor de vcpu da função vm_init
    struct vcpu* vcpu = vm_init(vm_alloc, vm_init_sync, vm_config, master, vm_id);
    cpu_sync_barrier(&vm_alloc->vm->sync);


    for (size_t i = 0; i < vm_config->children_num; i++) {
        vmid_t child_vmid = vmm_config_to_vmid(vm_config->children[i]);
        if (vm_assign[child_vmid].cpus & (1ULL << cpu()->id)) {
            if (!root_vcpu) {
                tmp_root = vcpu; 
            } else {
                tmp_root = root_vcpu;
            }

            vmm_create_vm(vm_config->children[i], child_vmid, master, &vm_assign[child_vmid].root_sync, tmp_root);

            INFO("VM %u is parent of VM %u\n", vm_id, child_vmid);
            struct vcpu* child_vcpu = cpu_get_vcpu_by_vmid(child_vmid);
            struct vcpu* parent_vcpu = cpu_get_vcpu_by_vmid(vm_id);
            list_push(&parent_vcpu->vmstack_children, &child_vcpu->vmstack_child_node);
        }
    }

    //return vcpu;
}

static bool vmm_get_next_assigned_root_vm(vmid_t* vm_id, bool* master)
{
    bool assigned = false;
    *master = false;

    for (size_t i = 0; i < config.vmlist_size; i++) {
        vmid_t vmid = vmm_config_to_vmid(config.vmlist[i]);

        if (vm_assign[vmid].cpus & (1ULL << cpu()->id)) {
            spin_lock(&vm_assign[vmid].lock);
            vm_assign[vmid].cpus &= ~(cpumap_t)(1ULL << cpu()->id);
            if (!vm_assign[vmid].master) {
                vm_assign[vmid].master = true;
                *master = true;
            }
            spin_unlock(&vm_assign[vmid].lock);

            *vm_id = vmid;
            assigned = true;
            break;
        }
    }

    return assigned;
}

struct vm* vmm_init_dynamic(struct dynconfig* dyn_config, uint64_t vm_addr)
{
    /* CROSSCON TODO: support multicore dynamic VMs */
    vmid_t vmid = vmm_alloc_vmid();
    struct vm_config* vm_cfg = &dyn_config->vm_cfg;
    struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vm_cfg, vmid, true);
    struct vm* dyn_vm = vm_init_dynamic(vm_alloc, vm_cfg, vm_addr, vmid, dyn_config);

    /* CROSSCON TODO */
    struct vcpu* child_vcpu = cpu_get_vcpu_by_vmid(dyn_vm->id);

    list_push(&cpu()->vcpu->vmstack_children, &child_vcpu->vmstack_child_node);

    return dyn_vm;
}

static void vmm_free_vm(struct vm* vm)
{
    /* CROSSCON TODO take into account vcpus as well */
    size_t n = NUM_PAGES(sizeof(struct vm));
    memset((void*)vm, 0, n * PAGE_SIZE);
    mem_unmap(&cpu()->as, (vaddr_t)vm, n, true);
}

void vmm_destroy_dynamic(struct vm* vm)
{
    list_foreach (&cpu()->vcpu->vmstack_children, node_t, node) {
        struct vcpu* vcpu_child = CONTAINER_OF(struct vcpu, vmstack_child_node, node);
        if (vcpu_child->vm == vm) {
            /* CROSSCON TODO remove recursively */
            list_rm(&cpu()->vcpu->vmstack_children, &vcpu_child->vmstack_child_node);
        }
    }

    vm_destroy_dynamic(vm);
    vmm_free_vm(vm);
}


void vmm_init()
{
    vmm_arch_init();
    vmm_io_init();
    shmem_init();
    remio_init();

    if (cpu_is_master()) {
            for (size_t j = 0; j < config.vmlist_size; j++) {
                vm_assign[j].lock = SPINLOCK_INITVAL;
                cpu_sync_init(&vm_assign[j].root_sync, config.vmlist[j]->platform.cpu_num);
                for (size_t i = 0; i < config.vmlist[j]->children_num; i++) {
                    cpu_sync_init(&vm_assign[j+i+1].root_sync, config.vmlist[j]->children[i]->platform.cpu_num);
                    //added because of the grandchildren of the vm
                    for (size_t z = 0; z < config.vmlist[j]->children[i]->children_num; z++) {
                        cpu_sync_init(&vm_assign[j+i+z+2].root_sync, config.vmlist[j]->children[i]->children[z]->platform.cpu_num);
                    }
                }
            }
            objpool_init(&nodes_pool);
            vmm_allocate_vmids();
            vmm_assign_vcpus();
    }

    cpu_sync_barrier(&cpu_glb_sync);

    bool master = false;
    vmid_t vm_id = INVALID_VMID;
    while (vmm_get_next_assigned_root_vm(&vm_id, &master)) {
        
        //isto tem de ficar comentado e dentro de vmm_create_vm para que os childs tenham uma função recursiva de criação de vms
        // struct vm_allocation* vm_alloc = vmm_alloc_install_vm(vm_id, master);
        // struct vm_config* vm_config = &config.vmlist[vm_id];
        // struct vm* vm = vm_init(vm_alloc, &vm_assign[vm_id].root_sync, vm_config, master, vm_id);

        vmm_create_vm(vm_config_by_id_table[vm_id], vm_id, master, &vm_assign[vm_id].root_sync, NULL);
        //cpu_sync_barrier(&vm->sync);
        
        cpu()->next_vcpu = cpu_get_vcpu_by_vmid(vm_id);
        vmstack_push(cpu()->next_vcpu);
        list_push(&cpu()->vcpu_sched_lst, &cpu()->next_vcpu->sched_node);

    }
}
