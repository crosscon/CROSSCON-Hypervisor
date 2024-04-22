#include <sdsgx.h>

#include <stdint.h>
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
    SDSGX_CREATE  = 0,
    SDSGX_ECALL   = 1,
    SDSGX_OCALL   = 2,
    SDSGX_RESUME  = 3,
    /* SDSGX_GOTO    = 4, */
    SDSGX_EXIT    = 5,
    SDSGX_DELETE  = 6,
    SDSGX_ADD_RGN = 7,
    SDSGX_INFO    = 8,
    SDSGX_FAULT   = 9,
};

struct config* sdsgx_get_cfg_from_host(struct vm* host, vaddr_t host_ipa)
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

    if (cfg_size > PAGE_SIZE) {
        /* Entire config minus the page we already mapped, page by page */
        if (mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, nclv_cfg_va + PAGE_SIZE,
                            NUM_PAGES(nclv_cfg->config_size) - 1) == NULL_VA) {
            ERROR("Mapping config %s", __func__);
        }
        for (int i = 1; i < NUM_PAGES(cfg_size); i++) {
            size_t offset = i * PAGE_SIZE;
            mem_guest_ipa_translate((void*)(host_ipa + offset), &paddr);
            struct ppages pp = mem_ppages_get(paddr, 1);
            if (!mem_map(&cpu.as, nclv_cfg_va + offset, &pp, 1,
                         PTE_HYP_FLAGS)) {
                ERROR("mem_map failed %s", __func__);
            }
        }
    }
    /* Assumes the last of the page of the config does not map anything other
     * than the config */
    mem_free_vpage(&host->as, host_ipa, NUM_PAGES(cfg_size), false);
    config_adjust_to_va(nclv_cfg, paddr);

    return nclv_cfg;
}

void sdsgx_create(uint64_t host_ipa)
{
    struct config* nclv_cfg = sdsgx_get_cfg_from_host(cpu.vcpu->vm, host_ipa);

    /* Create enclave */
    struct vm* enclave = vmm_init_dynamic(nclv_cfg, host_ipa);
    /* return the enclave id to the creator */
    vcpu_writereg(cpu.vcpu, 1, enclave->id);
    cpu.vcpu->nclv_data.initialized = false;

    /* init */
    /* TODO use parent vm.vcpu.id*/
    struct vcpu* enclave_vcpu = vm_get_vcpu(enclave, 0);
    vmstack_push(enclave_vcpu);
    enclave_vcpu->nclv_data.initialized = false;
    vcpu_writereg(cpu.vcpu, 0, 0);
}

void sdsgx_add_rgn(uint64_t enclave_id, uint64_t donor_ipa, uint64_t enclave_va)
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
    if (!va) {
        ERROR("mem_alloc_vpage failed %s", __func__);
    }
    if (!mem_map(&child->vm->as, va, &ppages, 1, PTE_VM_FLAGS)) {
        ERROR("mem_map failed %s", __func__);
    }

    vcpu_writereg(cpu.vcpu, 0, 0);
}

void sdsgx_delete(uint64_t enclave_id, uint64_t arg1)
{
    /* TODO: handle multiple child */

    struct vcpu* nclv = NULL;

    if ((nclv = vcpu_get_child(cpu.vcpu, 0)) == NULL) {
        ERROR("non host invoked enclaved destruction");
    }

    vmm_destroy_dynamic(nclv->vm);

    vcpu_writereg(cpu.vcpu, 0, 0);
}

void sdsgx_ecall(uint64_t enclave_id, uint64_t args_addr, uint64_t sp_el0)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* child = NULL;
    if ((child = vcpu_get_child(cpu.vcpu, 0)) != NULL) {
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

void sdsgx_ocall(uint64_t idx, uint64_t ms)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;

    enclave = vmstack_pop();
    if (enclave != NULL) {
        vcpu_writereg(cpu.vcpu, 0, SDSGX_OCALL);
        vcpu_writereg(cpu.vcpu, 1, enclave->regs->x[1]); /* SP to be updated */
        vcpu_writereg(cpu.vcpu, 2, enclave->regs->x[2]); /* calloc size */
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu.vcpu, 0, res);
    }
}

void sdsgx_resume(uint64_t enclave_id)
{
    /* TODO: handle multiple child */
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;
    if ((enclave = vcpu_get_child(cpu.vcpu, 0)) != NULL) {
        vmstack_push(enclave);
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu.vcpu, 0, res);
    }
}

void sdsgx_exit()
{
    /* this is just a hack */
    if (!cpu.vcpu->nclv_data.initialized) {
        cpu.vcpu->nclv_data.initialized = true;
    }

    vmstack_pop();
    vcpu_writereg(cpu.vcpu, 0, 0);
}

extern uint64_t irqs;
extern uint64_t enclv_aborts;
int64_t sdsgx_handle_hypercall(struct vcpu* vcpu, uint64_t fid)
{
    int64_t res = HC_E_SUCCESS;
    static unsigned int n_calls = 0;
    static unsigned int o_calls = 0;
    static unsigned int n_resumes = 0;

    uint64_t arg0 = vcpu->regs->x[1];
    uint64_t arg1 = vcpu->regs->x[2];
    uint64_t arg2 = vcpu->regs->x[3];

    switch (fid) {
        case SDSGX_CREATE:
            sdsgx_create(arg1);
            break;

        case SDSGX_RESUME:
            /* TODO: handle multiple child */
            n_resumes++;
            sdsgx_resume(arg0);
            break;

        case SDSGX_ECALL:
            n_calls++;
            sdsgx_ecall(arg0, arg1, arg2);
            break;

        case SDSGX_OCALL:
            o_calls++;
            sdsgx_ocall(arg0, arg1);
            break;

        case SDSGX_EXIT:
            sdsgx_exit();
            res = 0;
            break;

        case SDSGX_DELETE:  // ver alloc
            /* TODO: handle multiple child */
            sdsgx_delete(arg0, arg1);
            break;

        case SDSGX_ADD_RGN:
            sdsgx_add_rgn(arg0, arg1, arg2);
            break;
        case SDSGX_INFO:
            vcpu_writereg(cpu.vcpu, 1, enclv_aborts);
            vcpu_writereg(cpu.vcpu, 2, n_resumes);
            vcpu_writereg(cpu.vcpu, 3, irqs);
            vcpu_writereg(cpu.vcpu, 4, n_calls);
            vcpu_writereg(cpu.vcpu, 5, o_calls);
            vcpu_writereg(cpu.vcpu, 0, res);
            enclv_aborts = 0;
            n_calls = 0;
            o_calls = 0;
            n_resumes = 0;
            irqs = 0;
            enclv_aborts = 0;
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

uint64_t enclv_aborts = 0;
int64_t sdsgx_handle_abort(struct vcpu* vcpu, uint64_t addr)
{
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;
    enclv_aborts++;
    /* TODO: validate address space */

    enclave = vmstack_pop();
    if (enclave != NULL) {
        vcpu_writereg(cpu.vcpu, 0, SDSGX_FAULT);
        vcpu_writereg(cpu.vcpu, 1, enclave->vm->id);
        vcpu_writereg(cpu.vcpu, 2, addr);
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu.vcpu, 0, res);
    }
    return 0;
}

#include <interrupts.h>
extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

uint64_t irqs = 0;
void sdsgx_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    if (vcpu != cpu.vcpu && vcpu->state == VCPU_STACKED) {
        if (cpu.vcpu->vm->type == 3) { /* currently running enclave */
            if (cpu.vcpu->nclv_data.initialized == false) return;
            vmstack_pop(); /* transition to normal world */
            irqs++;
            /* signal that enclave was interrupted */
            vcpu_writereg(cpu.vcpu, 0, 1);
        }
    }
}

#include <vmm.h>
static struct hndl_hvc hvc = {
    /* TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdsgx_handle_hypercall,
};

static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    /* TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = {27, 33, 72, 73, 74, 75, 76, 77, 78, 79},
    .handler = sdsgx_handle_interrupt,
};

static struct hndl_mem_abort mem_abort = {
    .handler = sdsgx_handle_abort,
};

int64_t sdsgx_handler_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) return -1;

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_hvc_add(vm, &hvc);
    vm_hndl_irq_add(vm, &irq);

    /* TODO */
    if(vm->id >= 3)
        vm_hndl_mem_abort_add(vm, &mem_abort);

    return ret;
}
