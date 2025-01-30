/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <string.h>
#include <mem.h>
#include <cache.h>
#include <config.h>
#include <dynconfig.h>
#include <shmem.h>
#include <objpool.h>
#include <sdtz.h>
#include <sdgpos.h>
#include <sdsgx.h>
#include <vmstack.h>

extern uint8_t _hypercall_start, _start;

enum emul_type {EMUL_MEM, EMUL_REG};
struct emul_node {
    node_t node;
    enum emul_type type;
    union {
        struct emul_mem emu_mem;
        struct emul_reg emu_reg;
    };
};

/* Number of sdTEES */
OBJPOOL_ALLOC(irq_oc, struct hndl_irq_node, 10);
OBJPOOL_ALLOC(hvc_oc, struct hndl_hvc_node, 10);
OBJPOOL_ALLOC(smc_oc, struct hndl_smc_node, 10);
OBJPOOL_ALLOC(mem_abort_oc, struct hndl_mem_abort_node, 3);

static void vm_master_init(struct vm* vm, const struct vm_config* vm_config, vmid_t vm_id)
{
    vm->master = cpu()->id;
    vm->config = vm_config;
    vm->cpu_num = vm_config->platform.cpu_num;
    vm->id = vm_id;

    cpu_sync_init(&vm->sync, vm->cpu_num);

    vm_mem_prot_init(vm, vm_config);

    /* CROSSCON TODO remove as_init(&vm->as, AS_VM, vm->id, NULL, vm_config->colors); */

    vm->type = vm_config->type;

}

static void vm_master_destroy(struct vm* vm)
{
    cpu_sync_init(&vm->sync, vm->cpu_num);

    // objpool_destroy(&vm->emul_oc);
    // FREE this vms objects within the pool

    as_destroy(&vm->as);
}

static void vm_cpu_init(struct vm* vm)
{
    spin_lock(&vm->lock);
    vm->cpus |= (1UL << cpu()->id);
    spin_unlock(&vm->lock);
}

static vcpuid_t vm_calc_vcpu_id(struct vm* vm)
{
    vcpuid_t vcpu_id = 0;
    for (size_t i = 0; i < cpu()->id; i++) {
        if (!!bit_get(vm->cpus, i)) {
            vcpu_id++;
        }
    }
    return vcpu_id;
}

static struct vcpu* vm_vcpu_init(struct vm* vm, const struct vm_config* vm_config)
{
    vcpuid_t vcpu_id = vm_calc_vcpu_id(vm);
    struct vcpu* vcpu = vm_get_vcpu(vm, vcpu_id);

    vcpu->id = vcpu_id;
    vcpu->phys_id = cpu()->id;
    vcpu->vm = vm;
    vcpu->active = true;

    vcpu->blocked_count = 0;

    memset(vcpu->stack, 0, sizeof(vcpu->stack));

    vcpu->state = VCPU_INACTIVE;
    vcpu->parent = NULL;

    vcpu_arch_init(vcpu, vm);
    vcpu_arch_reset(vcpu, vm_config->entry);

     /* vmstacking */
    list_init(&vcpu->vmstack_children);

    cpu_add_vcpu(vcpu);

    return vcpu;
}

struct vcpu* vm_vcpu_destroy(struct vm* vm, struct vcpu* vcpu);

struct vcpu* vm_vcpu_destroy(struct vm* vm, struct vcpu* vcpu)
{
    UNUSED_ARG(vm);
    cpu_remove_vcpu(vcpu);

    /* CROSSCON TODO */
    WARNING("TODO: Must free vcpu array\n");
    // list_rm(&vm->vcpu_list, (node_t*)vcpu);

    memset(vcpu->stack, 0, sizeof(vcpu->stack));

    size_t n = NUM_PAGES(sizeof(struct vcpu));
    memset(vcpu, 0, n * PAGE_SIZE);

    mem_unmap(&cpu()->as, (vaddr_t)vcpu, n, true);

    return vcpu;
}

static void vm_map_mem_region(struct vm* vm, struct vm_mem_region* reg)
{
    size_t n = NUM_PAGES(reg->size);

    struct ppages pa_reg;
    struct ppages* pa_ptr = NULL;
    if (reg->place_phys) {
        pa_reg = mem_ppages_get(reg->phys, n);
        pa_reg.colors = reg->colors;
        pa_ptr = &pa_reg;
    } else {
        pa_ptr = NULL;
    }

    vaddr_t va = mem_alloc_map(&vm->as, SEC_VM_ANY, pa_ptr, (vaddr_t)reg->base, n, PTE_VM_FLAGS);
    if (va != (vaddr_t)reg->base) {
        ERROR("failed to allocate vm's region at 0x%lx", reg->base);
    }
}

static void vm_map_img_rgn_inplace(struct vm* vm, const struct vm_config* vm_config,
    struct vm_mem_region* reg)
{
    vaddr_t img_base = vm_config->image.base_addr;
    size_t img_size = vm_config->image.size;
    /* mem region pages before the img */
    size_t n_before = NUM_PAGES(img_base - reg->base);
    /* pages after the img */
    size_t n_aft = NUM_PAGES((reg->base + reg->size) - (img_base + img_size));
    /* mem region pages for img */
    size_t n_img = NUM_PAGES(img_size);

    /* map img in place */
    struct ppages pa_img = mem_ppages_get(vm_config->image.load_addr, n_img);

    mem_alloc_map(&vm->as, SEC_VM_ANY, NULL, (vaddr_t)reg->base, n_before, PTE_VM_FLAGS);
    if (all_clrs(vm->as.colors)) {
        /* map img in place */
        mem_alloc_map(&vm->as, SEC_VM_ANY, &pa_img, img_base, n_img, PTE_VM_FLAGS);
        /* we are mapping in place, config is already reserved */
    } else {
        /* recolour img */
        mem_map_reclr(&vm->as, img_base, &pa_img, n_img, PTE_VM_FLAGS);
    }
    /* map pages after img */
    mem_alloc_map(&vm->as, SEC_VM_ANY, NULL, img_base + NUM_PAGES(img_size) * PAGE_SIZE, n_aft,
        PTE_VM_FLAGS);
}

static void vm_install_image(struct vm* vm, struct vm_mem_region* reg)
{
    if (reg->place_phys) {
        paddr_t img_base = (paddr_t)vm->config->image.base_addr;
        paddr_t img_load_pa = vm->config->image.load_addr;
        size_t img_sz = vm->config->image.size;

        if (img_base == img_load_pa) {
            // The image is already correctly installed. Our work is done.
            return;
        }

        if (range_overlap_range(img_base, img_sz, img_load_pa, img_sz)) {
            // We impose an image load region cannot overlap its runtime region. This both
            // simplifies the copying procedure as well as avoids limitations of mpu-based memory
            // management which does not allow overlapping mappings on the same address space.
            ERROR("failed installing vm image. Image load region overlaps with"
                  " image runtime region");
        }
    }

    size_t img_num_pages = NUM_PAGES(vm->config->image.size);
    struct ppages img_ppages = mem_ppages_get(vm->config->image.load_addr, img_num_pages);
    vaddr_t src_va = mem_alloc_map(&cpu()->as, SEC_HYP_GLOBAL, &img_ppages, INVALID_VA,
        img_num_pages, PTE_HYP_FLAGS);
    vaddr_t dst_va =
        mem_map_cpy(&vm->as, &cpu()->as, vm->config->image.base_addr, INVALID_VA, img_num_pages);
    memcpy((void*)dst_va, (void*)src_va, vm->config->image.size);
    cache_flush_range((vaddr_t)dst_va, vm->config->image.size);
    mem_unmap(&cpu()->as, src_va, img_num_pages, false);
    mem_unmap(&cpu()->as, dst_va, img_num_pages, false);
}

static void vm_map_img_rgn(struct vm* vm, const struct vm_config* vm_config,
    struct vm_mem_region* reg)
{
    if (!reg->place_phys && vm_config->image.inplace) {
        vm_map_img_rgn_inplace(vm, vm_config, reg);
    } else {
        vm_map_mem_region(vm, reg);
        vm_install_image(vm, reg);
    }
}

static void vm_init_mem_regions(struct vm* vm, const struct vm_config* vm_config)
{
    for (size_t i = 0; i < vm_config->platform.region_num; i++) {
        struct vm_mem_region* reg = &vm_config->platform.regions[i];
        bool img_is_in_rgn =
            range_in_range(vm_config->image.base_addr, vm_config->image.size, reg->base, reg->size);
        if (img_is_in_rgn) {
            vm_map_img_rgn(vm, vm_config, reg);
        } else {
            vm_map_mem_region(vm, reg);
        }
    }
}

static void vm_init_ipc(struct vm* vm, const struct vm_config* vm_config)
{
    vm->ipc_num = vm_config->platform.ipc_num;
    vm->ipcs = vm_config->platform.ipcs;
    for (size_t i = 0; i < vm_config->platform.ipc_num; i++) {
        struct ipc* ipc = &vm_config->platform.ipcs[i];
        struct shmem* shmem = shmem_get(ipc->shmem_id);
        if (shmem == NULL) {
            WARNING("Invalid shmem id in configuration. Ignored.\n");
            continue;
        }
        INFO("VM %d adding IPC for shared memory %d at VA: 0x%lx  size: 0x%lx\n", vm->id, ipc->shmem_id, ipc->base, ipc->size);
        size_t size = ipc->size;
        if (ipc->size > shmem->size) {
            size = shmem->size;
            WARNING("Trying to map region to smaller shared memory. Truncated\n");
        }

        spin_lock(&shmem->lock);
        shmem->cpu_masters |= (1UL << cpu()->id);
        ipc->master = cpu()->id;
        spin_unlock(&shmem->lock);

        struct vm_mem_region reg = {
            .base = ipc->base,
            .size = size,
            .place_phys = true,
            .phys = shmem->phys,
            .colors = shmem->colors,
        };

        vm_map_mem_region(vm, &reg);

        for (size_t j = 0; j < ipc->interrupt_num; j++) {
            if (!interrupts_vm_assign(vm, ipc->interrupts[j])) {
                ERROR("Failed to assign interrupt id %d", ipc->interrupts[j]);
            }
        }
    }

#ifdef MEM_NON_UNIFIED
    if (vm->ipc_num) {
        size_t num_pages = NUM_PAGES((size_t)(&_start - &_hypercall_start));
        // size_t num_pages = 1;
        struct ppages ppages = mem_ppages_get((paddr_t)&_hypercall_start, num_pages);
        // struct ppages ppages = mem_ppages_get((paddr_t)0x40, num_pages);
        vaddr_t va = mem_alloc_map(&vm->as, SEC_HYP_HC, &ppages, (paddr_t)&_hypercall_start,
            num_pages, PTE_VM_HC_FLAGS);
        // vaddr_t va = mem_alloc_map(&vm->as, SEC_HYP_HC, &ppages, (paddr_t)0x40, num_pages,
        // PTE_VM_HC_FLAGS); mem_alloc_map(&cpu()->as, SEC_HYP_IMAGE, &ppages, (paddr_t)0x40,
        // num_pages, PTE_HYP_FLAGS_CODE);
        mem_alloc_map(&cpu()->as, SEC_HYP_IMAGE, &ppages, (paddr_t)&_hypercall_start, num_pages,
            PTE_HYP_FLAGS_CODE);

        if (va == INVALID_VA) {
            ERROR("couldn't install hypercall region at 0x%lx", &_hypercall_start);
        }
    }
#endif
}

static void vm_destroy_ipc(struct vm* vm)
{
    UNUSED_ARG(vm);
    /* for (size_t i = 0; i < vm->ipc_num; i++) { */
    /*     struct ipc *ipc = &vm->ipcs[i]; */
    /*     struct shmem *shmem = shmem_get(ipc->shmem_id); */
	/* mem_unmap(&vm->as, ipc->base, shmem->size, true); */
    /* } */
}

static void vm_init_dev(struct vm* vm, const struct vm_config* vm_config)
{
    for (size_t i = 0; i < vm_config->platform.dev_num; i++) {
        struct vm_dev_region* dev = &vm_config->platform.devs[i];
        INFO("VM %d adding MMIO region, VA: 0x%lx size: 0x%lx mapped at 0x%lx\n", vm->id, dev->va, dev->va, dev->pa);

        size_t n = ALIGN(dev->size, PAGE_SIZE) / PAGE_SIZE;

        if (dev->va != INVALID_VA) {
            mem_alloc_map_dev(&vm->as, SEC_VM_ANY, (vaddr_t)dev->va, dev->pa, n);
        }

        for (size_t j = 0; j < dev->interrupt_num; j++) {
            INFO("VM %d assigning interrupt %u\n", vm->id, dev->interrupts[j]);
            if (!interrupts_vm_assign(vm, dev->interrupts[j])) {
                ERROR("Failed to assign interrupt id %d", dev->interrupts[j]);
            }
        }
    }

    if (io_vm_init(vm, vm_config)) {
        for (size_t i = 0; i < vm_config->platform.dev_num; i++) {
            struct vm_dev_region* dev = &vm_config->platform.devs[i];
            if (dev->id) {
                if (!io_vm_add_device(vm, dev->id)) {
                    ERROR("Failed to add device to iommu");
                }
            }
        }
    }
}

static void vm_destroy_dev(struct vm* vm, const struct dynconfig* dynconfig)
{
    UNUSED_ARG(vm);
    UNUSED_ARG(dynconfig);
    /* CROSSCON TODO */
    WARNING("%s is not implemented\n", __func__);
}

static void vm_init_remio_dev(struct vm* vm, struct remio_dev* remio_dev)
{
    struct shmem* shmem = shmem_get(remio_dev->shmem.shmem_id);
    if (shmem == NULL) {
        ERROR("Invalid shmem id (%d) in the Remote I/O device (%d) configuration",
            remio_dev->shmem.shmem_id, remio_dev->bind_key);
    }
    size_t shmem_size = remio_dev->shmem.size;
    if (shmem_size > shmem->size) {
        shmem_size = shmem->size;
        WARNING("Trying to map region to smaller shared memory. Truncated\n");
    }
    spin_lock(&shmem->lock);
    shmem->cpu_masters |= (1UL << cpu()->id);
    spin_unlock(&shmem->lock);

    struct vm_mem_region reg = {
        .base = remio_dev->shmem.base,
        .size = shmem_size,
        .place_phys = true,
        .phys = shmem->phys,
        .colors = shmem->colors,
    };

    vm_map_mem_region(vm, &reg);

    if (remio_dev->type == REMIO_DEV_FRONTEND) {
        struct emul_mem* emu = &remio_dev->emul;
        emu->va_base = remio_dev->va;
        emu->size = remio_dev->size;
        emu->handler = remio_mmio_emul_handler;
        vm_emul_add_mem(vm, emu);
    }
}

/* CROSSCON TODO: this should be done in two steps:
 * 1: create array of memory regions corresponding to physical mappings and
 * include it this to the config file
 * 2: create the enclave vm dynamically using core features
 * 3: unmap physical memory
 * there's a performance tradeoff though */
struct config;
static void vm_dyn_host_donate(struct vm* host_vm, struct vm* dyn_vm, struct vm_config* dyn_vm_cfg, uint64_t donor_ipa)
{
    const struct dynconfig* dynconfig = dyn_vm->vmdyn_house_keeping.dynconfig;

    /* struct vm_mem_region img_rgn = { */
    /*     .phys = dyn_vm_cfg->image.load_addr, */
    /*     .base = dyn_vm_cfg->image.base_addr, */
    /*     .size = ALIGN(dyn_vm_cfg->image.size, PAGE_SIZE), */
    /*     .place_phys = true, */
    /*     .colors = 0, /1* CROSSCON TODO: support dynamic VM colors *1/ */
    /* }; */
    /* vm_map_mem_region(dyn_vm, &img_rgn); */

    /* mem_unmap(&host_vm->as, donor_ipa, NUM_PAGES(dyn_vm_cfg->image.size), false); */

    /* mem region 0 is special because it comes from the application */
    /* CROSSCON TODO: Support multiple regions */
    struct vm_mem_region* reg = &dyn_vm_cfg->platform.regions[0];

    uintptr_t dyn_vm_mem_start = reg->base;
    uintptr_t dyn_vm_mem_size = reg->size;

    INFO("Donating host VM %d memory VA 0x%x size 0x%x to VM %d\n", host_vm->id, reg->base, reg->size, dyn_vm->id);

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_nclv_ipa = 0;

    vaddr_t nclv_ipa = dyn_vm_mem_start;
    vaddr_t host_ipa = donor_ipa + dynconfig->config_header_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(dyn_vm_mem_size);
    size_t i = 1;
    while (i <= n) {
        bool last_page = (i == n);

        mem_guest_ipa_translate(&host_vm->as, host_ipa, &pa);
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
        struct vm_mem_region rgn = {
            .phys = base_cont_pa,
            .base = base_nclv_ipa,
            .size = contiguous_pages * PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(dyn_vm, &rgn);

        contiguous_pages = 1;
        base_cont_pa = pa;
        base_nclv_ipa = nclv_ipa;
    skip:
        nclv_ipa += PAGE_SIZE;
        host_ipa += PAGE_SIZE;
        i++;
    }
    mem_unmap(&host_vm->as, donor_ipa + dynconfig->config_header_size,
                   NUM_PAGES(dyn_vm_mem_size), false);

    /* CROSSCON TODO: All memory should be given by the donor VM, this is temporary to
     * test MPK domains */
    /* we start at 1 because region 0 (including image) already mapped */
    for (size_t j = 1; j < dyn_vm_cfg->platform.region_num; j++) {
        struct vm_mem_region* new_reg = &dyn_vm_cfg->platform.regions[j];
        vm_map_mem_region(dyn_vm, new_reg);
    }
}


static void vm_dynamic_reclaim(struct vm* host_vm, struct vm* dyn_vm)
{
    vmstack_push(cpu_get_vcpu(dyn_vm->id));

    struct dynconfig* enclv_config = dyn_vm->vmdyn_house_keeping.dynconfig;
    const struct vm_config* dyn_vm_cfg = dyn_vm->config;
    vaddr_t host_base_ipa = dyn_vm->vmdyn_house_keeping.donor_va;

    struct vm_mem_region* reg = &dyn_vm_cfg->platform.regions[0];

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_host_ipa = 0;

    vaddr_t dyn_vm_ipa = reg->base;
    vaddr_t host_ipa = host_base_ipa + enclv_config->config_header_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(reg->size);
    size_t i = 1;
    while (i <= n) {
        bool last_page = (i == n);

        mem_guest_ipa_translate(&dyn_vm->as, dyn_vm_ipa, &pa);
        if (contiguous_pages == 0) {
            contiguous_pages = 1;
            base_cont_pa = pa;
            base_host_ipa = host_ipa;
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
        vaddr_t va = mem_alloc_map(&cpu()->as, SEC_HYP_GLOBAL, &pp, INVALID_VA, contiguous_pages, PTE_HYP_FLAGS);
        memset((void*)va, 0, contiguous_pages * PAGE_SIZE);
        /* CROSSCON TODO: Optimize flush */
        cache_flush_range(va, contiguous_pages * PAGE_SIZE);
        mem_unmap(&cpu()->as, va, pp.num_pages, true);

        /* give back memory to host VM */
        struct vm_mem_region rgn = {
            .phys = base_cont_pa,
            .base = base_host_ipa,
            .size = contiguous_pages * PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(host_vm, &rgn);

        contiguous_pages = 1;
        base_cont_pa = pa;
        base_host_ipa = host_ipa;
    skip:
        dyn_vm_ipa += PAGE_SIZE;
        host_ipa += PAGE_SIZE;
        i++;
    }
    /* remove page from enclave's AS */
    mem_unmap(&dyn_vm->as, reg->base, n, false);

    /* restore */
    host_ipa = host_base_ipa;
    /* we are done with everything, give the last piece of memory to the host */
    size_t hdr_sz = enclv_config->config_header_size;
    for (size_t j = 0; j < NUM_PAGES(hdr_sz); j++) {
        memset((void*)enclv_config, 0, hdr_sz);
        host_ipa = host_base_ipa + j * PAGE_SIZE;

        mem_guest_ipa_translate(&cpu()->as, (vaddr_t)enclv_config, &pa);
        struct vm_mem_region rgn = {
            .phys = pa,
            .base = host_ipa,
            .size = PAGE_SIZE,
            .place_phys = true,
            .colors = 0,
        };
        vm_map_mem_region(host_vm, &rgn);

        mem_unmap(&cpu()->as, (vaddr_t)enclv_config, NUM_PAGES(hdr_sz), false);
    }
    vmstack_pop();
}


void vm_destroy_dynamic(struct vm* vm)
{
    INFO("Destroying dynamic VM %d\n", vm->id);
    vm_destroy_ipc(vm);
    /* ERROR("fetch the correct config"); */
    vm_destroy_dev(vm, vm->vmdyn_house_keeping.dynconfig);

    /* CROSSCON TODO: This is not making much sense right now. We need to reclaim
     * resources from the vm, not from the cpu */
    vm_dynamic_reclaim(cpu()->vcpu->vm, vm);

    vm_vcpu_destroy(vm, vm_get_vcpu(vm, 0));
    /* CROSSCONM TODO */
    /* vm_arch_destroy(vm, vm->config); */

    vm_master_destroy(vm);
}

static void vm_init_remio(struct vm* vm, const struct vm_config* vm_config)
{
    if (vm_config->platform.remio_dev_num == 0) {
        return;
    }

    vm->remio_dev_num = vm_config->platform.remio_dev_num;
    vm->remio_devs = vm_config->platform.remio_devs;

    for (size_t i = 0; i < vm_config->platform.remio_dev_num; i++) {
        struct remio_dev* remio_dev = &vm_config->platform.remio_devs[i];
        vm_init_remio_dev(vm, remio_dev);
    }
    remio_assign_vm_cpus(vm);
}

static struct vm* vm_allocation_init(struct vm_allocation* vm_alloc)
{
    struct vm* vm = vm_alloc->vm;
    vm->vcpus = vm_alloc->vcpus;
    return vm;
}

struct vm* vm_init_dynamic(struct vm_allocation* vm_alloc, struct vm_config *vm_cfg, uint64_t vm_addr, vmid_t vmid, struct dynconfig* dyn_config)
{
    INFO("Creating dynamic VM %d\n", vmid);
    struct vm* dyn_vm = vm_allocation_init(vm_alloc);

    vm_master_init(dyn_vm, vm_cfg, vmid);
    dyn_vm->vmdyn_house_keeping.dynconfig = dyn_config;
    vm_cpu_init(dyn_vm);

    vm_vcpu_init(dyn_vm, vm_cfg);
    vm_arch_init(dyn_vm, vm_cfg);

    struct vm *host_vm = cpu()->vcpu->vm;
    vm_dyn_host_donate(host_vm, dyn_vm, vm_cfg, vm_addr);

    vm_init_dev(dyn_vm, vm_cfg);
    vm_init_ipc(dyn_vm, vm_cfg);

    sdsgx_handler_setup(dyn_vm);

    dyn_vm->vmdyn_house_keeping.donor_va = vm_addr;
    // CROSSCON TODO do this outside: vm->vmdyn_house_keeping.config = dyn_config;
    INFO("Dynamic VM %d created\n", vmid);

    return dyn_vm;
}

struct vm* vm_init(struct vm_allocation* vm_alloc, const struct vm_config* vm_config, bool master,
    vmid_t vm_id)
{
    struct vm* vm = vm_allocation_init(vm_alloc);

    /**
     * Before anything else, initialize vm structure.
     */
    if (master) {
        INFO("Initializing VM %d\n", vm_id);
        vm_master_init(vm, vm_config, vm_id);
    }

    /*
     *  Initialize each core.
     */
    vm_cpu_init(vm);

    cpu_sync_barrier(&vm->sync);

    /*
     *  Initialize each virtual core.
     */
    struct vcpu* vcpu = vm_vcpu_init(vm, vm_config);
    UNUSED_ARG(vcpu);

    cpu_sync_barrier(&vm->sync);

    /**
     * Perform architecture dependent initializations. This includes, for example, setting the page
     * table pointer and other virtualization extensions specifics.
     */
    vm_arch_init(vm, vm_config);

    /**
     * Create the VM's address space according to configuration and where its image was loaded.
     */
    if (master) {
        vm_init_mem_regions(vm, vm_config);
        vm_init_dev(vm, vm_config);
        vm_init_ipc(vm, vm_config);
        vm_init_remio(vm, vm_config);
    }

    if(master){
        switch(vm->type){
            case 0:
                INFO("VM %d is sdGPOS (normal VM)\n", vm->id);
                break;
            case 1:
            case 2:
                INFO("VM %d is sdTZ (OP-TEE)\n", vm->id);
                break;
            default:
                ERROR("VM %d type invalid");
        }

        /* CROSSCON TODO: use linker table */
        sdtz_handler_setup(vm);
        sdgpos_handler_setup(vm);
        sdsgx_handler_setup(vm);
    }

    cpu_sync_and_clear_msgs(&vm->sync);

    return vm;
}

void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu)
{
    list_push(&vm->emul_mem_list, &emu->node);
}

void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu)
{
    list_push(&vm->emul_reg_list, &emu->node);
}

emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr)
{
    emul_handler_t handler = NULL;
    list_foreach (vm->emul_mem_list, struct emul_mem, emu) {
        if (addr >= emu->va_base && (addr < (emu->va_base + emu->size))) {
            handler = emu->handler;
            break;
        }
    }

    return handler;
}

emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr)
{
    emul_handler_t handler = NULL;
    list_foreach (vm->emul_reg_list, struct emul_reg, emu) {
        if (emu->addr == addr) {
            handler = emu->handler;
            break;
        }
    }

    return handler;
}

void vm_hndl_irq_add(struct vm* vm, struct hndl_irq* irqs)
{
    struct hndl_irq_node* ptr = objpool_alloc(&irq_oc);
    if (ptr != NULL) {
        ptr->hndl_irq = *irqs;
        list_push(&vm->irq_list, (node_t*)ptr);
    }
}

void vm_hndl_smc_add(struct vm* vm, struct hndl_smc* smcs)
{
    struct hndl_smc_node* ptr = objpool_alloc(&smc_oc);
    if (ptr != NULL) {
        ptr->hndl_smc = *smcs;
        list_push(&vm->smc_list, (node_t*)ptr);
    }
}

void vm_hndl_hvc_add(struct vm* vm, struct hndl_hvc* hvcs)
{
    struct hndl_hvc_node* ptr = objpool_alloc(&hvc_oc);
    if (ptr != NULL) {
        ptr->hndl_hvc = *hvcs;
        list_push(&vm->hvc_list, (node_t*)ptr);
    }
}

void vm_hndl_mem_abort_add(struct vm* vm, struct hndl_mem_abort* mem_aborts)
{
    struct hndl_mem_abort_node* ptr = objpool_alloc(&mem_abort_oc);
    if (ptr != NULL) {
        ptr->hndl_mem_abort = *mem_aborts;
        list_push(&vm->mem_abort_list, (node_t*)ptr);
    }
}

void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg)
{
    for (size_t i = 0, n = 0; n < vm->cpu_num - 1; i++) {
        if (((1U << i) & vm->cpus) && (i != cpu()->id)) {
            n++;
            cpu_send_msg(i, msg);
        }
    }
}

__attribute__((weak)) cpumap_t vm_translate_to_pcpu_mask(struct vm* vm, cpumap_t mask, size_t len)
{
    cpumap_t pmask = 0;
    cpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) && ((shift = vm_translate_to_pcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1UL << shift);
        }
    }
    return pmask;
}

__attribute__((weak)) cpumap_t vm_translate_to_vcpu_mask(struct vm* vm, cpumap_t mask, size_t len)
{
    cpumap_t pmask = 0;
    vcpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) && ((shift = vm_translate_to_vcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1UL << shift);
        }
    }
    return pmask;
}

void vcpu_run(struct vcpu* vcpu)
{
    if (vcpu_arch_is_on(vcpu)) {
        if (vcpu->active) {
            vcpu_arch_entry();
        } else {
            cpu_standby();
        }
    } else {
        cpu_powerdown();
    }
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

void vcpu_context_switch(void)
{
    if (cpu()->vcpu != NULL) {
        vcpu_save_state(cpu()->vcpu);
    }
    vcpu_restore_state(cpu()->next_vcpu);
    cpu()->vcpu = cpu()->next_vcpu;
}
