#include <baoenclave.h>

#include <vm.h>
#include <vmm.h>
#include <mem.h>
#include <tlb.h>
#include <string.h>
#include <vmstack.h>
#include <hypercall.h>
#include "arch/page_table.h"
#include "config.h"
#include "util.h"

#define MASK 3

enum {
    BAOENCLAVE_CREATE  = 0,
    BAOENCLAVE_ECALL   = 1,
    BAOENCLAVE_OCALL   = 2,
    BAOENCLAVE_RESUME  = 3,
    BAOENCLAVE_GOTO    = 4,
    BAOENCLAVE_EXIT    = 5,
    BAOENCLAVE_DELETE  = 6,
    BAOENCLAVE_ADD_RGN = 7,
    BAOENCLAVE_INFO    = 8,
    BAOENCLAVE_FAULT   = 9,
};

void baoenclave_donate(struct vm *nclv,  struct config *config, uint64_t donor_ipa)
{
    struct vm_config* nclv_cfg = config->vmlist[0];

    struct mem_region img_rgn = {
	.phys = nclv_cfg->image.load_addr,
	.base = nclv_cfg->image.base_addr,
	.size = ALIGN(nclv_cfg->image.size, PAGE_SIZE),
	.place_phys = true,
	.colors = 0,
    };
    vm_map_mem_region(nclv, &img_rgn);

    mem_free_vpage(&cpu.vcpu->vm->as,
		    donor_ipa,
		    NUM_PAGES(config->config_size),
		    false);

    /* TODO We should check enclaves integrity at this point */

    /* mem region 0 is special because it comes from the application */
    struct mem_region* reg = &nclv_cfg->platform.regions[0];

    /* we already mapped the image from base to image.size */
    uintptr_t nclv_mem_after_img = reg->base + ALIGN(nclv_cfg->image.size, PAGE_SIZE);

    /* leftover memory we need to map (discounting image size) */
    uintptr_t leftover_to_map_vm = reg->size - ALIGN(nclv_cfg->image.size, PAGE_SIZE);

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_nclv_ipa = 0;

    vaddr_t nclv_ipa = nclv_mem_after_img;
    vaddr_t host_ipa = donor_ipa + config->config_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(leftover_to_map_vm);
    size_t i = 1;
    while(i <= n) {
	bool last_page = (i == n);

	mem_guest_ipa_translate((void*)host_ipa, &pa);
	if(contiguous_pages == 0){
	    contiguous_pages = 1;
	    base_cont_pa = pa;
	    base_nclv_ipa = nclv_ipa;
	    if(!last_page){
		goto skip;
	    }
	} else if(pa == (base_cont_pa + contiguous_pages * PAGE_SIZE)){
	    contiguous_pages++;
	    if(!last_page){
		goto skip;
	    }
	}

	/* give memory to nclv VM */
	struct mem_region rgn = {
	    .phys       = base_cont_pa,
	    .base       = base_nclv_ipa,
	    .size       = contiguous_pages * PAGE_SIZE,
	    .place_phys = true,
	    .colors     = 0,
	};
	vm_map_mem_region(nclv, &rgn);

	contiguous_pages = 1;
	base_cont_pa = pa;
	base_nclv_ipa = nclv_ipa;
skip:
	nclv_ipa += PAGE_SIZE;
	host_ipa += PAGE_SIZE;
	i++;
    }
    mem_free_vpage(&cpu.vcpu->vm->as,
	    donor_ipa + config->config_size,
	    NUM_PAGES(leftover_to_map_vm),
	    false);

    /* TODO: All memory should be given by the donor VM, this is temporary to
     * test MPK domains */
    /* we start at 1 because region 0 (including image) already mapped */
    for (size_t i = 1; i < nclv_cfg->platform.region_num; i++) {
	struct mem_region* reg = &nclv_cfg->platform.regions[i];
	vm_map_mem_region(nclv, reg);
    }
}

struct config* baoenclave_get_cfg_from_host(struct vm* host, vaddr_t host_ipa)
{
    uint64_t paddr = 0;
    vaddr_t nclv_cfg_va = (vaddr_t)NULL;
    struct config* nclv_cfg;
    size_t cfg_size;
    /* TODO: handle multiple child */

    /* One page */
    mem_guest_ipa_translate((void*)(host_ipa), &paddr);
    nclv_cfg_va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, (vaddr_t)NULL, 1);
    struct ppages pp = mem_ppages_get(paddr, 1);
    if (!mem_map(&cpu.as, (vaddr_t)nclv_cfg_va, &pp, 1, PTE_HYP_FLAGS)) {
	ERROR("mem_map failed %s", __func__);
    }
    nclv_cfg = (struct config*)nclv_cfg_va;
    cfg_size = nclv_cfg->config_header_size;

    if(cfg_size > PAGE_SIZE){
	/* Entire config minus the page we already mapped, page by page */
	if(mem_alloc_vpage(&cpu.as,
		    SEC_HYP_GLOBAL,
		    nclv_cfg_va+PAGE_SIZE,
		    NUM_PAGES(nclv_cfg->config_size)-1) == NULL_VA){
	    ERROR("Mapping config %s", __func__);
	}
	for(int i = 1; i < NUM_PAGES(cfg_size); i++){
	    size_t offset = i * PAGE_SIZE;
	    mem_guest_ipa_translate((void*)(host_ipa + offset), &paddr);
	    struct ppages pp = mem_ppages_get(paddr, 1);
	    if (!mem_map(&cpu.as, nclv_cfg_va + offset, &pp, 1, PTE_HYP_FLAGS)) {
		ERROR("mem_map failed %s", __func__);
	    }
	}
    }
    /* Assumes the laste of the page of the config does not map anything other
     * than the config */
    mem_free_vpage(&host->as, host_ipa, NUM_PAGES(cfg_size), false);
    config_adjust_to_va(nclv_cfg, paddr);

    return nclv_cfg;
}

void baoenclave_create(uint64_t host_ipa)
{
    struct config *nclv_cfg = baoenclave_get_cfg_from_host(cpu.vcpu->vm, host_ipa);

    /* Create enclave */
    struct vm *enclave = vmm_init_dynamic(nclv_cfg, host_ipa);
    /* return the enclave id to the creator */
    vcpu_writereg(cpu.vcpu, 1, enclave->id);
    cpu.vcpu->nclv_data.initialized = false;

    /* init */
    /* TODO use parent vm.vcpu.id*/
    struct vcpu *enclave_vcpu = vm_get_vcpu(enclave, 0);
    vmstack_push(enclave_vcpu);
    enclave_vcpu->nclv_data.initialized = false;
    vcpu_writereg(cpu.vcpu, 0, 0);
}

void baoenclave_add_rgn(uint64_t enclave_id, uint64_t donor_ipa, uint64_t enclave_va)
{
    /* TODO: handle multiple child */
    struct vcpu* child = NULL;
    uint64_t physical_address = 0;
    vaddr_t va = (vaddr_t)NULL;

    /* TODO: handle multiple child */
    if ((child = vcpu_get_child(cpu.vcpu, 0)) == NULL) {
	/* TODO HANDLE */
	return;
    }
    mem_guest_ipa_translate((void*)donor_ipa, &physical_address);
    struct ppages ppages = mem_ppages_get(physical_address, 1);
    va = mem_alloc_vpage(&child->vm->as, SEC_VM_ANY, (vaddr_t)enclave_va, 1);
    if(!va) {
	ERROR("mem_alloc_vpage failed %s", __func__);
    }
    if (!mem_map(&child->vm->as, va, &ppages, 1, PTE_VM_FLAGS)) {
	ERROR("mem_map failed %s", __func__);
    }

    vcpu_writereg(cpu.vcpu, 0, 0);
}

void baoenclave_reclaim(struct vcpu* host, struct vcpu* nclv)
{
    vmstack_push(nclv);

    struct config *config = nclv->vm->enclave_house_keeping.config;
    struct vm_config* nclv_cfg = config->vmlist[0];
    vaddr_t host_base_nclv_ipa = nclv->vm->enclave_house_keeping.donor_va;

    struct mem_region* reg = &nclv_cfg->platform.regions[0];

    size_t contiguous_pages = 0;
    size_t base_cont_pa = 0;
    vaddr_t base_host_ipa = 0;
    vaddr_t base_hyp = 0;

    vaddr_t nclv_ipa = reg->base;
    vaddr_t host_ipa = host_base_nclv_ipa + config->config_header_size;
    paddr_t pa;
    const size_t n = NUM_PAGES(reg->size);
    const vaddr_t hyp_va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL_VA, n);
    vaddr_t hyp_va_tmp = hyp_va;
    size_t i = 1;
    while(i <= n) {
	bool last_page = (i == n);

	mem_guest_ipa_translate((void*)nclv_ipa, &pa);
	if(contiguous_pages == 0){
	    contiguous_pages = 1;
	    base_cont_pa = pa;
	    base_host_ipa = host_ipa;
	    base_hyp = hyp_va_tmp;
	    if(!last_page){
		goto skip;
	    }
	} else if(pa == (base_cont_pa + contiguous_pages * PAGE_SIZE)){
	    contiguous_pages++;
	    if(!last_page){
		goto skip;
	    }
	}

	/* clear the memory */
	struct ppages pp = mem_ppages_get(base_cont_pa, contiguous_pages);
	mem_map(&cpu.as, base_hyp, &pp, contiguous_pages, PTE_HYP_FLAGS);
	memset((void*)base_hyp, 0, contiguous_pages * PAGE_SIZE);

	/* TODO optimize undoing the mapping */
	/* give back memory to host VM */
	struct mem_region rgn = {
	    .phys       = base_cont_pa,
	    .base       = base_host_ipa,
	    .size       = contiguous_pages * PAGE_SIZE,
	    .place_phys = true,
	    .colors     = 0,
	};
	vm_map_mem_region(host->vm, &rgn);

	contiguous_pages = 1;
	base_cont_pa = pa;
	base_host_ipa = host_ipa;
	base_hyp = hyp_va_tmp;
skip:
	nclv_ipa += PAGE_SIZE;
	host_ipa += PAGE_SIZE;
	hyp_va_tmp += PAGE_SIZE;
	i++;
    }
    /* remove page from enclave's AS */
    mem_free_vpage(&nclv->vm->as, reg->base, n, false);
    /* unmap memory from hypervisor */
    mem_free_vpage(&cpu.as, hyp_va, n, false);

    /* restore */
    host_ipa = host_base_nclv_ipa;
    /* we are done with everything, give the last piece of memory to the host */
    size_t hdr_sz = config->config_header_size;
    for (size_t i = 0; i < NUM_PAGES(hdr_sz); i++) {
	memset((void*)config, 0, hdr_sz);
	vaddr_t host_ipa = host_base_nclv_ipa + i*PAGE_SIZE;

	paddr_t pa;
	mem_translate(&cpu.as, (vaddr_t)config, &pa);
	struct mem_region rgn = {
	    .phys       = pa,
	    .base       = host_ipa,
	    .size       = PAGE_SIZE,
	    .place_phys = true,
	    .colors     = 0,
	};
	vm_map_mem_region(host->vm, &rgn);

	mem_free_vpage(&cpu.as, (vaddr_t)config, NUM_PAGES(hdr_sz), false);
    }
    vmstack_pop();
}

void baoenclave_delete(uint64_t enclave_id, uint64_t arg1)
{
    /* TODO: handle multiple child */

    struct vcpu* nclv = NULL;

    if ((nclv = vcpu_get_child(cpu.vcpu, 0)) == NULL) {
	ERROR("non host invoked enclaved destruction");
    }

    vmm_destroy_dynamic(nclv->vm);

    vcpu_writereg(cpu.vcpu, 0, 0);
}

void baoenclave_ecall(uint64_t enclave_id, uint64_t args_addr, uint64_t sp_el0)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* child = NULL;
    if((child = vcpu_get_child(cpu.vcpu, 0)) != NULL){
	/* TODO separate architecture specific details. only works for Arm */
	/* child->arch.sysregs.vm.sp_el0 = sp_el0; */
	vmstack_push(child);
	vcpu_writereg(cpu.vcpu, 1, args_addr);
	vcpu_writereg(cpu.vcpu, 2, sp_el0);
    } else {
	res = -HC_E_INVAL_ARGS;
	vcpu_writereg(cpu.vcpu, 0, res);
    }
}


void baoenclave_ocall(uint64_t idx, uint64_t ms)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;

    enclave = vmstack_pop();
    if(enclave != NULL){
	vcpu_writereg(cpu.vcpu, 0, BAOENCLAVE_OCALL);
	vcpu_writereg(cpu.vcpu, 1, enclave->regs->x[1]); /* SP to be updated */
	vcpu_writereg(cpu.vcpu, 2, enclave->regs->x[2]); /* calloc size */
    } else {
	res = -HC_E_INVAL_ARGS;
	vcpu_writereg(cpu.vcpu, 0, res);
    }
}

void baoenclave_resume(uint64_t enclave_id)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;
    if((enclave = vcpu_get_child(cpu.vcpu, 0)) != NULL){
	/* vcpu_writereg(enclave, 2, cpu.vcpu->regs->x[2]); /1* calloc size *1/ */
	vmstack_push(enclave);
    } else {
	res = -HC_E_INVAL_ARGS;
	vcpu_writereg(cpu.vcpu, 0, res);
    }
}

void baoenclave_exit()
{
    /* this is just a hack */
    if (!cpu.vcpu->nclv_data.initialized){
	cpu.vcpu->nclv_data.initialized = true;
    }

    vmstack_pop();
    vcpu_writereg(cpu.vcpu, 0, 0);
}

int64_t baoenclave_dynamic_hypercall(uint64_t fid, uint64_t arg0, uint64_t arg1,
                                     uint64_t arg2)
{
    int64_t res = HC_E_SUCCESS;
    static unsigned int n_calls = 0;
    static unsigned int n_resumes = 0;

    switch (fid) {
        case BAOENCLAVE_CREATE:
	    baoenclave_create(arg1);
            break;

        case BAOENCLAVE_RESUME:
	    /* TODO: handle multiple child */
	    n_resumes++;
	    baoenclave_resume(arg0);
            break;

	case BAOENCLAVE_ECALL:
	    n_calls++;
	    baoenclave_ecall(arg0, arg1, arg2);
            break;

	case BAOENCLAVE_OCALL:
	    n_calls++;
	    baoenclave_ocall(arg0, arg1);
            break;

        case BAOENCLAVE_EXIT:
	    /* TODO: handle multiple child */
	    baoenclave_exit();
	    res = 0;
            break;

        case BAOENCLAVE_DELETE: //ver alloc
	    /* TODO: handle multiple child */
	    baoenclave_delete(arg0, arg1);
            break;

        case BAOENCLAVE_ADD_RGN:
	    baoenclave_add_rgn(arg0, arg1, arg2);
	    break;
	case BAOENCLAVE_INFO:
	    vcpu_writereg(cpu.vcpu, 1, n_calls);
	    vcpu_writereg(cpu.vcpu, 2, n_resumes);
	    vcpu_writereg(cpu.vcpu, 0, res);
	    n_calls = 0;
	    n_resumes = 0;
	    break;

        default:
	    ERROR("Unknown command %d from vm %u", fid, cpu.vcpu->vm->id);
            res = -HC_E_FAILURE;
	    vcpu_writereg(cpu.vcpu, 0, res);
    }

    if (vcpu_is_off(cpu.vcpu)) {
        cpu_idle();
    }

    return res;
}

int baoenclave_handle_abort(unsigned long addr)
{
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;
    /* TODO: validate address space */

    enclave = vmstack_pop();
    if(enclave != NULL){
	vcpu_writereg(cpu.vcpu, 0, BAOENCLAVE_FAULT);
	vcpu_writereg(cpu.vcpu, 1, enclave->vm->id);
	vcpu_writereg(cpu.vcpu, 2, addr);
    } else {
	res = -HC_E_INVAL_ARGS;
	vcpu_writereg(cpu.vcpu, 0, res);
    }
    return 0;
}
