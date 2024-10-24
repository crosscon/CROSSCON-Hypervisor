/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <vm.h>
#include <vmm.h>
#include <string.h>
#include <mem.h>
#include <cache.h>
#include "inc/ipc.h"
#include "list.h"

#include <sdtz.h>
#include <sdgpos.h>
#include <sdsgx.h>
#include <vmstack.h>

enum emul_type {EMUL_MEM, EMUL_REG};
struct emul_node {
    node_t node;
    enum emul_type type;
    union {
        struct emul_mem emu_mem;
        struct emul_reg emu_reg;
    };
};

static void vm_master_init(struct vm* vm, const struct vm_config* config, vmid_t vm_id)
{
    vm->master = cpu.id;
    vm->config = config;
    vm->cpu_num = config->platform.cpu_num;
    vm->id = vm_id;

    cpu_sync_init(&vm->sync, vm->cpu_num);

    as_init(&vm->as, AS_VM, vm->id, NULL, config->colors);

    vm->type = config->type;

    list_init(&vm->emul_list);
    list_init(&vm->vcpu_list);
    objcache_init(&vm->emul_oc, sizeof(struct emul_node), SEC_HYP_VM, false);
    objcache_init(&vm->smc_oc, sizeof(struct hndl_smc_node), SEC_HYP_VM, false);
    objcache_init(&vm->hvc_oc, sizeof(struct hndl_hvc_node), SEC_HYP_VM, false);
    objcache_init(&vm->irq_oc, sizeof(struct hndl_irq_node), SEC_HYP_VM, false);
    objcache_init(&vm->mem_abort_oc, sizeof(struct hndl_mem_abort_node), SEC_HYP_VM, false);
}

static void vm_master_destroy(struct vm* vm)
{
    cpu_sync_init(&vm->sync, vm->cpu_num);

    objcache_destroy(&vm->emul_oc);

    as_destroy(&vm->as);
}

void vm_cpu_init(struct vm* vm)
{
    spin_lock(&vm->lock);
    vm->cpus |= (1UL << cpu.id);
    spin_unlock(&vm->lock);
}

void vm_add_vcpu(struct vm* vm, struct vcpu* vcpu)
{
    struct node_data* node = objcache_alloc(&partition->nodes);
    node->data = vcpu;
    list_push(&vm->vcpu_list, (node_t*) node);
}

struct vcpu* vm_get_vcpu(struct vm* vm, vcpuid_t vcpuid)
{
    list_foreach(vm->vcpu_list, struct node_data, node){
	struct vcpu* vcpu = node->data;
        if(vcpu->id == vcpuid){
            return vcpu;
        }
    }

    return NULL;
}


struct vcpu* vm_vcpu_init(struct vm* vm, const struct vm_config* vm_cfg)
{
    size_t n = NUM_PAGES(sizeof(struct vcpu));
    struct vcpu* vcpu = (struct vcpu*)mem_alloc_page(n, SEC_HYP_VM, false);
    if(vcpu == NULL){ ERROR("failed to allocate vcpu"); }
    memset(vcpu, 0, n * PAGE_SIZE);

    /* cpu.vcpu = vcpu; */
    vcpu->phys_id = cpu.id;
    vcpu->vm = vm;

    size_t count = 0, offset = 0;
    while (count < vm->cpu_num) {
        if (offset == cpu.id) {
            vcpu->id = count;
            break;
        }
        if ((1UL << offset) & vm->cpus) {
            count++;
        }
        offset++;
    }

    memset(vcpu->stack, 0, sizeof(vcpu->stack));
    vcpu->regs = (struct arch_regs*)(vcpu->stack + sizeof(vcpu->stack) -
                                     sizeof(*vcpu->regs));

    vcpu->state = VCPU_INACTIVE;
    vcpu->parent = NULL;

    vm_add_vcpu(vm, vcpu);

    vcpu_arch_init(vcpu, vm);
    vcpu_arch_reset(vcpu, vm->config->entry);

    /* vmstacking */
    list_init(&vcpu->vmstack_children);

    cpu_add_vcpu(vcpu);

    return vcpu;
}

struct vcpu* vm_vcpu_destroy(struct vm* vm, struct vcpu* vcpu)
{
    cpu_remove_vcpu(vcpu);

    list_rm(&vm->vcpu_list, (node_t*)vcpu);

    memset(vcpu->stack, 0, sizeof(vcpu->stack));

    size_t n = NUM_PAGES(sizeof(struct vcpu));
    memset(vcpu, 0, n * PAGE_SIZE);

    mem_free_vpage(&cpu.as, (vaddr_t)vcpu, n, true);

    return vcpu;
}


void vm_copy_img_to_rgn(struct vm* vm, const struct vm_config* config,
                               struct mem_region* reg)
{
    /* map original img address */
    size_t n_img = NUM_PAGES(config->image.size);
    struct ppages src_pa_img = mem_ppages_get(config->image.load_addr, n_img);
    vaddr_t src_va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL_VA, n_img);
    if (!mem_map(&cpu.as, src_va, &src_pa_img, n_img, PTE_HYP_FLAGS)) {
        ERROR("mem_map failed %s", __func__);
    }

    /* map new address */
    size_t offset = config->image.base_addr - reg->base;
    size_t dst_phys = reg->phys + offset;
    struct ppages dst_pp = mem_ppages_get(dst_phys, n_img);
    vaddr_t dst_va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL_VA, n_img);
    if (!mem_map(&cpu.as, dst_va, &dst_pp, n_img, PTE_HYP_FLAGS)) {
        ERROR("mem_map failed %s", __func__);
    }

    memcpy((void*)dst_va, (void*)src_va, n_img * PAGE_SIZE);
    cache_flush_range((vaddr_t)dst_va, n_img * PAGE_SIZE);
    /* TODO: unmap */
}

void vm_map_mem_region(struct vm* vm, struct mem_region* reg)
{
    INFO("VM %d adding memory region, VA 0x%lx size 0x%lx", vm->id, reg->base, reg->size);
    size_t n = NUM_PAGES(reg->size);
    vaddr_t va = mem_alloc_vpage(&vm->as, SEC_VM_ANY,
                    (vaddr_t)reg->base, n);
    if (va != (vaddr_t)reg->base) {
        ERROR("failed to allocate vm's dev address");
    }

    if (reg->place_phys) {
        struct ppages pa_reg = mem_ppages_get(reg->phys, n);
        mem_map(&vm->as, va, &pa_reg, n, PTE_VM_FLAGS);
    } else {
        mem_map(&vm->as, va, NULL, n, PTE_VM_FLAGS);
    }
}

static void vm_map_img_rgn_inplace(struct vm* vm, const struct vm_config* config,
                                   struct mem_region* reg)
{
    /* mem region pages before the img */
    size_t n_before = (config->image.base_addr - reg->base) / PAGE_SIZE;
    /* pages after the img */
    size_t n_aft = ((reg->base + reg->size) -
                    (config->image.base_addr + config->image.size)) /
                   PAGE_SIZE;
    /* mem region pages for img */
    size_t n_img = NUM_PAGES(config->image.size);

    size_t n_total = n_before + n_aft + n_img;

    /* map img in place */
    struct ppages pa_img = mem_ppages_get(config->image.load_addr, n_img);
    vaddr_t va = mem_alloc_vpage(&vm->as, SEC_VM_ANY,
                                    (vaddr_t)reg->base, n_total);

    /* map pages before img */
    mem_map(&vm->as, va, NULL, n_before, PTE_VM_FLAGS);

    if (all_clrs(vm->as.colors)) {
        /* map img in place */
        mem_map(&vm->as, va + n_before * PAGE_SIZE, &pa_img, n_img,
                PTE_VM_FLAGS);
        /* we are mapping in place, config is already reserved */
    } else {
        /* recolour img */
        mem_map_reclr(&vm->as, va + n_before * PAGE_SIZE, &pa_img, n_img,
                      PTE_VM_FLAGS);
        /* TODO: reserve phys mem? */
    }
    /* map pages after img */
    mem_map(&vm->as, va + (n_before + n_img) * PAGE_SIZE, NULL, n_aft,
            PTE_VM_FLAGS);
}

static void vm_install_image(struct vm* vm) {
    size_t img_num_pages = NUM_PAGES(vm->config->image.size);
    struct ppages img_ppages =
        mem_ppages_get(vm->config->image.load_addr, img_num_pages);
    vaddr_t src_va =
        mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL_VA, img_num_pages);
    mem_map(&cpu.as, src_va, &img_ppages, img_num_pages, PTE_HYP_FLAGS);
    vaddr_t dst_va = mem_map_cpy(&vm->as, &cpu.as, vm->config->image.base_addr,
                                NULL_VA, img_num_pages);
    memcpy((void*)dst_va, (void*)src_va, vm->config->image.size);
    cache_flush_range((vaddr_t)dst_va, vm->config->image.size);
    mem_free_vpage(&cpu.as, src_va, img_num_pages, false);
    mem_free_vpage(&cpu.as, dst_va, img_num_pages, false);
}

void vm_map_img_rgn(struct vm* vm, const struct vm_config* config,
                           struct mem_region* reg)
{
    if (reg->place_phys) {
        vm_copy_img_to_rgn(vm, config, reg);
        vm_map_mem_region(vm, reg);
    } else if(config->image.inplace) {
        vm_map_img_rgn_inplace(vm, config, reg);
    } else {
        vm_map_mem_region(vm, reg);
        vm_install_image(vm);
    }
}

static void vm_init_mem_regions(struct vm* vm, const struct vm_config* config)
{
    for (size_t i = 0; i < config->platform.region_num; i++) {
        struct mem_region* reg = &config->platform.regions[i];

        bool img_is_in_rgn = range_in_range(
            config->image.base_addr, config->image.size, reg->base, reg->size);
        if (img_is_in_rgn) {
            vm_map_img_rgn(vm, config, reg);
        } else {
            vm_map_mem_region(vm, reg);
        }
    }
}

static void vm_init_ipc(struct vm* vm, const struct vm_config* config)
{
    vm->ipc_num = config->platform.ipc_num;
    vm->ipcs = config->platform.ipcs;
    for (size_t i = 0; i < vm->ipc_num; i++) {
        struct ipc *ipc = &vm->ipcs[i];
        struct shmem *shmem = ipc_get_shmem(ipc->shmem_id);
        if(shmem == NULL) {
            WARNING("Invalid shmem id in configuration. Ignored.");
            continue;
        }
        INFO("VM %d adding IPC for shared memory %d at VA: 0x%lx  size: 0x%lx", vm->id, ipc->shmem_id, ipc->base, ipc->size);
        size_t size = ipc->size;
        if(ipc->size > shmem->size) {
            size = shmem->size;
            WARNING("Trying to map region to smaller shared memory. Truncated");
        }
        struct mem_region reg = {
            .base = ipc->base,
            .size = size,
            .place_phys = true,
            .phys = shmem->phys,
            .colors = shmem->colors
        };

        vm_map_mem_region(vm, &reg);
    }
}

static void vm_destroy_ipc(struct vm* vm)
{
    for (size_t i = 0; i < vm->ipc_num; i++) {
        struct ipc *ipc = &vm->ipcs[i];
        struct shmem *shmem = ipc_get_shmem(ipc->shmem_id);
	mem_free_vpage(&vm->as, ipc->base, shmem->size, true);
    }
}

static void vm_init_dev(struct vm* vm, const struct vm_config* vm_cfg)
{
    for (size_t i = 0; i < vm->config->platform.dev_num; i++) {
        struct dev_region* dev = &vm->config->platform.devs[i];
        INFO("VM %d adding MMIO region, VA: 0x%lx size: 0x%lx mapped at 0x%lx", vm->id, dev->va, dev->va, dev->pa);

        size_t n = ALIGN(dev->size, PAGE_SIZE) / PAGE_SIZE;

        vaddr_t va = mem_alloc_vpage(&vm->as, SEC_VM_ANY,
                                        (vaddr_t)dev->va, n);
        mem_map_dev(&vm->as, va, dev->pa, n);

        for (size_t j = 0; j < dev->interrupt_num; j++) {
            INFO("VM %d assigning interrupt %u", vm->id, dev->interrupts[j]);
            interrupts_vm_assign(vm, dev->interrupts[j]);
        }
    }

    /* iommu */
    if (iommu_vm_init(vm, vm->config)) {
        for (size_t i = 0; i < vm->config->platform.dev_num; i++) {
            struct dev_region* dev = &vm->config->platform.devs[i];
            if (dev->id) {
                if(!iommu_vm_add_device(vm, dev->id)){
                    ERROR("Failed to add device to iommu");
                }
            }
        }
    }
}

static void vm_destroy_dev(struct vm* vm, const struct vm_config* config)
{
    /* TODO */
}


/* TODO: this should be done in two steps:
 * 1: create array of memory regions corresponding to physical mappings and
 * include it this to the config file
 * 2: create the enclave vm dynamically using core features
 * 3: unmap physical memory
 * there's a performance tradeoff though */
static void vm_dynamic_donate(struct vm* newvm, struct config* config, uint64_t donor_ipa)
{
    struct vm_config* newvm_cfg = config->vmlist[0];

    /* struct mem_region img_rgn = { */
    /*     .phys = newvm_cfg->image.load_addr, */
    /*     .base = newvm_cfg->image.base_addr, */
    /*     .size = ALIGN(newvm_cfg->image.size, PAGE_SIZE), */
    /*     .place_phys = true, */
    /*     .colors = 0, */
    /* }; */
    /* vm_map_mem_region(newvm, &img_rgn); */

    /* mem_free_vpage(&cpu.vcpu->vm->as, donor_ipa, NUM_PAGES(config->config_size), */
    /*                false); */

    /* mem region 0 is special because it comes from the application */
    struct mem_region* reg = &newvm_cfg->platform.regions[0];

    uintptr_t newvm_mem_start = reg->base;
    uintptr_t newvm_mem_size = reg->size;

    INFO("Donating host VM %d memory VA 0x%x size 0x%x to VM %d", cpu.vcpu->vm->id, reg->base, reg->size, newvm->id);

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_nclv_ipa = 0;

    vaddr_t nclv_ipa = newvm_mem_start;
    vaddr_t host_ipa = donor_ipa + config->config_header_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(newvm_mem_size);
    size_t i = 1;
    while (i <= n) {
        bool last_page = (i == n);

        mem_guest_ipa_translate((void*)host_ipa, &pa);
        if (contiguous_pages == 0) {
            contiguous_pages = 1;
            base_cont_pa = pa;
            base_nclv_ipa = nclv_ipa;
            if (!last_page) {
                goto skip;
            }
        } else if (pa == (base_cont_pa + contiguous_pages * PAGE_SIZE)) {
            contiguous_pages++;
            if (!last_page) {
                goto skip;
            }
        }

        /* give memory to nclv VM */
        struct mem_region rgn = {
            .phys = base_cont_pa,
            .base = base_nclv_ipa,
            .size = contiguous_pages * PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(newvm, &rgn);

        contiguous_pages = 1;
        base_cont_pa = pa;
        base_nclv_ipa = nclv_ipa;
    skip:
        nclv_ipa += PAGE_SIZE;
        host_ipa += PAGE_SIZE;
        i++;
    }
    mem_free_vpage(&cpu.vcpu->vm->as, donor_ipa + config->config_header_size,
                   NUM_PAGES(newvm_mem_size), false);

    /* TODO: All memory should be given by the donor VM, this is temporary to
     * test MPK domains */
    /* we start at 1 because region 0 (including image) already mapped */
    for (size_t i = 1; i < newvm_cfg->platform.region_num; i++) {
        struct mem_region* reg = &newvm_cfg->platform.regions[i];
        vm_map_mem_region(newvm, reg);
    }
}


void vm_init_dynamic(struct vm* vm, struct config* config, uint64_t vm_addr, vmid_t vmid)
{
    INFO("Creating dynamic VM %d", vmid);
    vm_master_init(vm, config->vmlist[0], vmid);
    vm_cpu_init(vm);

    vm_vcpu_init(vm, config->vmlist[0]);
    vm_arch_init(vm, config->vmlist[0]);

    /* TODO: init dynamic from config not like this */
    vm_dynamic_donate(vm, config, vm_addr);

    vm_init_dev(vm, config->vmlist[0]);
    vm_init_ipc(vm, config->vmlist[0]);

    sdsgx_handler_setup(vm);

    vm->vmdyn_house_keeping.donor_va = vm_addr;
    vm->vmdyn_house_keeping.config = config;
    INFO("Dynamic VM %d created", vmid);
}

void vm_dynamic_reclaim(struct vcpu* host, struct vcpu* newvm)
{
    vmstack_push(newvm);

    struct config* config = newvm->vm->vmdyn_house_keeping.config;
    struct vm_config* newvm_cfg = config->vmlist[0];
    vaddr_t host_base_ipa = newvm->vm->vmdyn_house_keeping.donor_va;

    struct mem_region* reg = &newvm_cfg->platform.regions[0];

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_host_ipa = 0;
    vaddr_t base_hyp = 0;

    vaddr_t newvm_ipa = reg->base;
    vaddr_t host_ipa = host_base_ipa + config->config_header_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(reg->size);
    const vaddr_t hyp_va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL_VA, n);
    vaddr_t hyp_va_tmp = hyp_va;
    size_t i = 1;
    while (i <= n) {
        bool last_page = (i == n);

        mem_guest_ipa_translate((void*)newvm_ipa, &pa);
        if (contiguous_pages == 0) {
            contiguous_pages = 1;
            base_cont_pa = pa;
            base_host_ipa = host_ipa;
            base_hyp = hyp_va_tmp;
            if (!last_page) {
                goto skip;
            }
        } else if (pa == (base_cont_pa + contiguous_pages * PAGE_SIZE)) {
            contiguous_pages++;
            if (!last_page) {
                goto skip;
            }
        }

        /* clear the memory */
        struct ppages pp = mem_ppages_get(base_cont_pa, contiguous_pages);
        mem_map(&cpu.as, base_hyp, &pp, contiguous_pages, PTE_HYP_FLAGS);
        memset((void*)base_hyp, 0, contiguous_pages * PAGE_SIZE);
        /* TODO: must flush */

        /* TODO optimize undoing the mapping */
        /* give back memory to host VM */
        struct mem_region rgn = {
            .phys = base_cont_pa,
            .base = base_host_ipa,
            .size = contiguous_pages * PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(host->vm, &rgn);

        contiguous_pages = 1;
        base_cont_pa = pa;
        base_host_ipa = host_ipa;
        base_hyp = hyp_va_tmp;
    skip:
        newvm_ipa += PAGE_SIZE;
        host_ipa += PAGE_SIZE;
        hyp_va_tmp += PAGE_SIZE;
        i++;
    }
    /* remove page from enclave's AS */
    mem_free_vpage(&newvm->vm->as, reg->base, n, false);
    /* unmap memory from hypervisor */
    mem_free_vpage(&cpu.as, hyp_va, n, false);

    /* restore */
    host_ipa = host_base_ipa;
    /* we are done with everything, give the last piece of memory to the host */
    size_t hdr_sz = config->config_header_size;
    for (size_t i = 0; i < NUM_PAGES(hdr_sz); i++) {
        memset((void*)config, 0, hdr_sz);
        vaddr_t host_ipa = host_base_ipa + i * PAGE_SIZE;

        paddr_t pa;
        mem_translate(&cpu.as, (vaddr_t)config, &pa);
        struct mem_region rgn = {
            .phys = pa,
            .base = host_ipa,
            .size = PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(host->vm, &rgn);

        mem_free_vpage(&cpu.as, (vaddr_t)config, NUM_PAGES(hdr_sz), false);
    }
    vmstack_pop();
}


void vm_destroy_dynamic(struct vm* vm)
{
    INFO("Destroying dynamic VM %d", vm->id);
    vm_destroy_ipc(vm);
    vm_destroy_dev(vm, vm->config);

    /* TODO: This is not making much sense right now. We need to reclaim
     * resources from the vm, not from the cpu */
    vm_dynamic_reclaim(cpu.vcpu, vm_get_vcpu(vm, 0));

    vm_vcpu_destroy(vm, vm_get_vcpu(vm, 0));
    /* vm_arch_destroy(vm, vm->config); */

    vm_master_destroy(vm);
}

#include <sdtz.h>

struct vcpu* vm_init(struct vm* vm, const struct vm_config* config, bool master, vmid_t vm_id)
{
    /**
     * Before anything else, initialize vm structure.
     */
    if (master) {
        INFO("Initializing VM %d", vm_id);
        vm_master_init(vm, config, vm_id);
    }

    /*
     *  Initialize each core.
     */
    vm_cpu_init(vm);

    cpu_sync_barrier(&vm->sync);

    /*
     *  Initialize each virtual core.
     */
    struct vcpu* vcpu = vm_vcpu_init(vm, config);

    cpu_sync_barrier(&vm->sync);

    /**
     * Perform architecture dependent initializations. This includes,
     * for example, setting the page table pointer and other virtualization
     * extensions specifics.
     */
    vm_arch_init(vm, config);

    /**
     * Create the VM's address space according to configuration and where
     * its image was loaded.
     */
    if (master) {
        vm_init_mem_regions(vm, config);
        vm_init_dev(vm, config);
        vm_init_ipc(vm, config);
    }

    if(master){
        switch(vm->type){
            case 0:
                INFO("VM %d is sdGPOS (normal VM)", vm->id);
                break;
            case 1:
            case 2:
                INFO("VM %d is sdTZ (OP-TEE)", vm->id);
                break;
        }

        /* TODO: use linker table */
        sdtz_handler_setup(vm);
        sdgpos_handler_setup(vm);
        sdsgx_handler_setup(vm);
    }

    cpu_sync_barrier(&vm->sync);

    return vcpu;
}

void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu)
{
    struct emul_node* ptr = objcache_alloc(&vm->emul_oc);
    if (ptr != NULL) {
        ptr->type = EMUL_MEM;
        ptr->emu_mem = *emu;
        list_push(&vm->emul_list, (node_t*)ptr);
        // TODO: if we plan to grow the VM's PAS dynamically, after
        // inialization,
        // the pages for this emulation region must be reserved in the stage 2
        // page table.
    }
}

void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu)
{
    struct emul_node* ptr = objcache_alloc(&vm->emul_oc);
    if (ptr != NULL) {
        ptr->type = EMUL_REG;
        ptr->emu_reg = *emu;
        list_push(&vm->emul_list, (node_t*)ptr);
        // TODO: if we plan to grow the VM's PAS dynamically, after
        // inialization,
        // the pages for this emulation region must be reserved in the stage 2
        // page table.
    }

}

void vm_hndl_irq_add(struct vm* vm, struct hndl_irq* irqs)
{
    struct hndl_irq_node* ptr = objcache_alloc(&vm->irq_oc);
    if (ptr != NULL) {
        ptr->hndl_irq = *irqs;
        list_push(&vm->irq_list, (node_t*)ptr);
    }
}

void vm_hndl_smc_add(struct vm* vm, struct hndl_smc* smcs)
{
    struct hndl_smc_node* ptr = objcache_alloc(&vm->smc_oc);
    if (ptr != NULL) {
        ptr->hndl_smc = *smcs;
        list_push(&vm->smc_list, (node_t*)ptr);
    }
}

void vm_hndl_hvc_add(struct vm* vm, struct hndl_hvc* hvcs)
{
    struct hndl_hvc_node* ptr = objcache_alloc(&vm->hvc_oc);
    if (ptr != NULL) {
        ptr->hndl_hvc = *hvcs;
        list_push(&vm->hvc_list, (node_t*)ptr);
    }
}

void vm_hndl_mem_abort_add(struct vm* vm, struct hndl_mem_abort* mem_aborts)
{
    struct hndl_mem_abort_node* ptr = objcache_alloc(&vm->hvc_oc);
    if (ptr != NULL) {
        ptr->hndl_mem_abort = *mem_aborts;
        list_push(&vm->mem_abort_list, (node_t*)ptr);
    }
}


static inline emul_handler_t vm_emul_get(struct vm* vm, enum emul_type type, vaddr_t addr)
{
    emul_handler_t handler = NULL;
    list_foreach(vm->emul_list, struct emul_node, node)
    {
        if (node->type == EMUL_MEM) {
            struct emul_mem* emu = &node->emu_mem;
            if (addr >= emu->va_base && (addr < (emu->va_base + emu->size))) {
                handler = emu->handler;
                break;
            }
        } else {
            struct emul_reg *emu = &node->emu_reg;
            if(emu->addr == addr) {
                handler = emu->handler;
                break;
            }
        }
    }

    return handler;
}

emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr)
{
    return vm_emul_get(vm, EMUL_MEM, addr);
}

emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr)
{
    return vm_emul_get(vm, EMUL_REG, addr);
}

void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg)
{
    for (size_t i = 0, n = 0; n < vm->cpu_num - 1; i++) {
        if (((1U << i) & vm->cpus) && (i != cpu.id)) {
            n++;
            cpu_send_msg(i, msg);
        }
    }
}

__attribute__((weak)) cpumap_t vm_translate_to_pcpu_mask(struct vm* vm,
                                                         cpumap_t mask,
                                                         size_t len)
{
    cpumap_t pmask = 0;
    cpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) &&
            ((shift = vm_translate_to_pcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1ULL << shift);
        }
    }
    return pmask;
}

__attribute__((weak)) cpumap_t vm_translate_to_vcpu_mask(struct vm* vm,
                                                         cpumap_t mask,
                                                         size_t len)
{
    cpumap_t pmask = 0;
    vcpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) &&
            ((shift = vm_translate_to_vcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1ULL << shift);
        }
    }
    return pmask;
}

void vcpu_run(struct vcpu* vcpu)
{
    cpu.vcpu->active = true;
    vcpu_arch_run(vcpu);
}

struct vcpu* vcpu_get_child(struct vcpu* vcpu, int index)
{
    int i = 0;
    struct vcpu* child = NULL;
    list_foreach(vcpu->vmstack_children, struct node_data, node)
    {
        if (i++ == index) {
            child = node->data;
            break;
        }
    }
    return child;
}
