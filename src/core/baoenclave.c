#include <baoenclave.h>

#include <vm.h>
#include <vmm.h>
#include <mem.h>
#include <tlb.h>
#include <string.h>
#include <vmstack.h>
#include <hypercall.h>

#define MASK 3

struct config* config_ptr;

vaddr_t alloc_baoenclave_struct(uint64_t physical_address, size_t size)
{
    struct ppages ppages = mem_ppages_get(physical_address, size);
    vaddr_t va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, (vaddr_t)NULL, size);
    if (!mem_map(&cpu.as, va, &ppages, size, PTE_HYP_FLAGS)) {
        ERROR("mem_map failed %s", __func__);
    }

    return va;
}

void alloc_baoenclave(void* vm_ptr, uint64_t donor_va)
{
    struct vm* vm = vm_ptr;
    struct vm_config* vm_cfg = config_ptr->vmlist[0];

    struct mem_region img_reg;
    struct mem_region mem_reg;

    uint64_t physical_address = 0;

    uint64_t mem_pos = 0;

    img_reg.phys = vm_cfg->image.load_addr;
    img_reg.base = vm_cfg->image.base_addr;
    img_reg.size = ALIGN(vm_cfg->image.size, PAGE_SIZE);
    img_reg.place_phys = true;
    img_reg.colors = 0;

    vm_copy_img_to_rgn(vm_ptr, vm_cfg, &img_reg);
    vm_map_mem_region(vm_ptr, &img_reg);

    mem_free_vpage(&cpu.vcpu->vm->as, donor_va,
                   NUM_PAGES(config_ptr->config_size), false);

    struct mem_region* reg = &vm_cfg->platform.regions[0];

    /* we already mapped the image from base to image.size */
    uintptr_t vm_mem_after_img = reg->base + ALIGN(vm_cfg->image.size, PAGE_SIZE);

    /* leftover memory we need to map (discounting image size) */
    uintptr_t leftover_to_map_vm = reg->size - ALIGN(vm_cfg->image.size, PAGE_SIZE);;

    /* could be optimized if(physical_address), guardando */
    for (size_t i = 0; i < NUM_PAGES(leftover_to_map_vm); i++) {
        mem_pos =
            (size_t)(donor_va + (i * PAGE_SIZE) + config_ptr->config_size);
        mem_guest_ipa_translate((void*)mem_pos, &physical_address);

        mem_reg.phys = physical_address;
        mem_reg.base = vm_mem_after_img + (i * PAGE_SIZE);
        mem_reg.size = PAGE_SIZE;
        mem_reg.place_phys = true;
        mem_reg.colors = 0;

        vm_map_mem_region(vm, &mem_reg);
        mem_free_vpage(&cpu.vcpu->vm->as, mem_pos, 1, false);
    }

    tlb_vm_inv_all(cpu.vcpu->vm->id);
    tlb_vm_inv_all(vm->id);
}

enum {
    BAOENCLAVE_CREATE  = 0,
    BAOENCLAVE_CALL    = 1,
    BAOENCLAVE_RESUME  = 2,
    BAOENCLAVE_GOTO    = 3,
    BAOENCLAVE_EXIT    = 4,
    BAOENCLAVE_DELETE  = 5,
    BAOENCLAVE_ADD_RGN = 6
};

int64_t baoenclave_dynamic_hypercall(uint64_t fid, uint64_t arg0, uint64_t arg1,
                                     uint64_t arg2)
{
    int64_t res = HC_E_SUCCESS;

    size_t c_size;
    size_t full_size;
    uint64_t physical_address = 0;

    struct vcpu* child = NULL;
    vaddr_t va = (vaddr_t)NULL;

    //uint64_t tmp = MRS(DAIF);

    switch (fid) {
        case BAOENCLAVE_CREATE:
            mem_guest_ipa_translate((void*)arg1, &physical_address);

            /* One page */
            va = alloc_baoenclave_struct(physical_address, 1);
            config_ptr = (struct config *)va;
            c_size = config_ptr->config_size;

            /* Free the page */
            mem_free_vpage(&cpu.as, va, 1, true);

            /* Entire config */
            va = alloc_baoenclave_struct(physical_address, NUM_PAGES(c_size));
            config_ptr = (struct config *)va;

            config_adjust_to_va(config_ptr, physical_address);

            /* Create enclave */
            struct vm *enclave = vmm_init_dynamic(config_ptr, arg1);
		/* return the enclave id to the creator */
	    vcpu_writereg(cpu.vcpu, 1, enclave->id);
	    cpu.vcpu->nclv_data.initialized = false;

	    /* init */
	    /* TODO use parent vm.vcpu.id*/
	    struct vcpu *enclave_vcpu = vm_get_vcpu(enclave, 0);
	    vmstack_push(enclave_vcpu);
	    enclave_vcpu->nclv_data.initialized = false;

            INFO("Enclave Created");
	    vcpu_writereg(cpu.vcpu, 0, 0);
            break;
        case BAOENCLAVE_RESUME:
            if((child = vcpu_get_child(cpu.vcpu, 0)) != NULL){
                vmstack_push(child);
            } else {
                res = -HC_E_INVAL_ARGS;
		vcpu_writereg(cpu.vcpu, 0, res);
            }
            break;
	case BAOENCLAVE_CALL:
            if((child = vcpu_get_child(cpu.vcpu, 0)) != NULL){
                vmstack_push(child);
		vcpu_writereg(cpu.vcpu, 1, arg1);
            } else {
                res = -HC_E_INVAL_ARGS;
		vcpu_writereg(cpu.vcpu, 0, res);
            }
            break;
        case BAOENCLAVE_EXIT:
	    /* this is just a hack */
	    cpu.vcpu->nclv_data.initialized = true;
            vmstack_pop();
	    res = 0;
            break;
        case BAOENCLAVE_DELETE: //ver alloc

            /* optimizar com o adress space vttbr e assim*/
            if ((child = vcpu_get_child(cpu.vcpu, 0)) != NULL) {
                vmstack_push(child);
            }

            mem_guest_ipa_translate(
                (void*)config_ptr->vmlist[0]->platform.regions->base,
                &physical_address);


            full_size = //config_ptr->vmlist[0]->platform.ipcs->size +
                        config_ptr->vmlist[0]->platform.regions->size +
                        config_ptr->config_header_size;

            /* Clear memory */
            va =
                alloc_baoenclave_struct(physical_address, NUM_PAGES(full_size));
            memset((void*)va, 0, full_size);
            mem_free_vpage(&cpu.as, va, NUM_PAGES(full_size), true);

            vmstack_pop();

            /* Map in primary VM again, mapear pagina a pagina */
            if (!mem_map(&cpu.vcpu->vm->as,
                        (vaddr_t)arg1 /*+ config_ptr->config_header_size*/, NULL,
                        NUM_PAGES(full_size), PTE_VM_FLAGS)) {
                ERROR("mem_map failed %s", __func__);
            }

            INFO("Enclave Destroyed");
	    vcpu_writereg(cpu.vcpu, 0, 0);
            break;
        case BAOENCLAVE_ADD_RGN:
            if ((child = vcpu_get_child(cpu.vcpu, 0)) == NULL) {
		res = -HC_E_FAILURE;
		break;
            }
            mem_guest_ipa_translate((void*)arg1, &physical_address);
	    struct ppages ppages = mem_ppages_get(physical_address, 1);
	    vaddr_t va = mem_alloc_vpage(&child->vm->as, SEC_VM_ANY, (vaddr_t)arg2, 1);
	    if(!va) {
		ERROR("mem_alloc_vpage failed %s", __func__);
	    }
	    if (!mem_map(&child->vm->as, va, &ppages, 1, PTE_VM_FLAGS)) {
		ERROR("mem_map failed %s", __func__);
	    }
	    vcpu_writereg(cpu.vcpu, 0, 0);

	    break;
        default:
	    ERROR("Uknown command %d from vm %u", fid, cpu.vcpu->vm->id);
            res = -HC_E_FAILURE;
	    vcpu_writereg(cpu.vcpu, 0, res);
    }

    if (vcpu_is_off(cpu.vcpu)) {
        cpu_idle();
    }

    return res;
}
