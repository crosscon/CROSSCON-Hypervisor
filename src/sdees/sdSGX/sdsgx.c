#include <arch/crossconhyp.h>
#include <dynconfig.h>
#include <cpu.h>
#include <types.h>
#include <sdsgx.h>

#include <stdint.h>
#include <vm.h>
#include <vmm.h>
#include <mem.h>
#include <tlb.h>
#include <string.h>
#include <vmstack.h>
#include <hypercall.h>
#include <arch/page_table.h>
#include <dynconfig.h>
#include <util.h>

/* #define MASK 3 */

enum {
    SDSGX_CREATE = 0,
    SDSGX_ECALL = 1,
    SDSGX_OCALL = 2,
    SDSGX_RESUME = 3,
    /* SDSGX_GOTO    = 4, */
    SDSGX_EXIT = 5,
    SDSGX_DELETE = 6,
    SDSGX_ADD_RGN = 7,
    SDSGX_INFO = 8,
    SDSGX_FAULT = 9,
};

static struct vcpu* sdsgx_get_nclv(struct vcpu* vcpu, size_t nclv_id)
{
    struct vcpu* child = NULL;
    list_foreach (vcpu->vmstack_children, struct node_data, node) {
        struct vcpu* tmp = NULL;
        tmp = node->data;
        if (tmp->nclv_data.id == nclv_id) {
            child = tmp;
            break;
        }
    }
    return child;
}

static struct dynconfig* sdsgx_get_cfg_from_host(struct vm* host, vaddr_t host_ipa)
{
    uint64_t paddr = 0;
    vaddr_t nclv_cfg_va = (vaddr_t)NULL;
    struct dynconfig* nclv_cfg;
    size_t cfg_size;
    bool pushed = 0;

    /* One page */
    if (cpu()->vcpu->vm != host) {
        pushed = true;
        struct vcpu* vcpu = cpu_get_vcpu(host->id);
        vmstack_push(vcpu);
    }
    mem_guest_ipa_translate(&host->as, host_ipa, &paddr);
    struct ppages dyn_cfg_pp = mem_ppages_get(paddr, 1);

    nclv_cfg_va =
        mem_alloc_map(&cpu()->as, SEC_HYP_GLOBAL, &dyn_cfg_pp, INVALID_VA, 1, PTE_HYP_FLAGS);
    if (nclv_cfg_va == INVALID_VA) {
        ERROR("Failed to allocate and map memory for host sdsgx config");
    }

    nclv_cfg = (struct dynconfig*)nclv_cfg_va;
    cfg_size = nclv_cfg->config_header_size;

    if (cfg_size > PAGE_SIZE) {
        /* Entire config minus the page we already mapped, page by page */
        size_t offset = PAGE_SIZE;
        for (size_t i = 1; i < NUM_PAGES(cfg_size); i++) {
            mem_guest_ipa_translate(&host->as, (host_ipa + offset), &paddr);
            struct ppages pp = mem_ppages_get(paddr, 1);
            nclv_cfg_va = mem_alloc_map(&cpu()->as, SEC_HYP_GLOBAL, &pp, nclv_cfg_va + offset, 1,
                PTE_HYP_FLAGS);
            if (nclv_cfg_va == INVALID_VA) {
                ERROR("Failed to allocate and map memory for host sdsgx config");
            }
            offset += PAGE_SIZE;
        }
    }
    /* Assumes the last page of the config does not map anything other than the config */
    mem_unmap(&host->as, host_ipa, NUM_PAGES(cfg_size), false);
    dynconfig_init(nclv_cfg, paddr);

    if (pushed) {
        vmstack_pop();
    }

    return nclv_cfg;
}

static void sdsgx_create(uint64_t host_ipa)
{
    struct dynconfig* nclv_cfg = sdsgx_get_cfg_from_host(cpu()->vcpu->vm, host_ipa);

    /* Create enclave */
    /* CROSSCON TODO CHECKS */
    struct vm* nclv_vm = vmm_init_dynamic(nclv_cfg, host_ipa);
    /* return the enclave id to the creator */
    vcpu_writereg(cpu()->vcpu, 1, nclv_vm->id);
    cpu()->vcpu->nclv_data.initialized = false;

    /* init */
    struct vcpu* nclv_vcpu = cpu_get_vcpu(nclv_vm->id);
    nclv_vcpu->nclv_data.id = nclv_vm->id;
    vcpu_writereg(nclv_vcpu, 0, 0);
    vmstack_push(nclv_vcpu);
    nclv_vcpu->nclv_data.initialized = false;
}

static void sdsgx_add_rgn(uint64_t enclave_id, uint64_t donor_ipa, uint64_t nclv_va)
{
    /* CROSSCON TODO: handle multiple child */
    struct vcpu* child = NULL;
    uint64_t pa = 0;

    if ((child = sdsgx_get_nclv(cpu()->vcpu, enclave_id)) == NULL) {
        /* CROSSCON TODO HANDLE */
        return;
    }
    mem_guest_ipa_translate(&cpu()->vcpu->vm->as, donor_ipa, &pa);

    struct ppages pp = mem_ppages_get(pa, 1);
    mem_alloc_map(&child->vm->as, SEC_VM_ANY, &pp, ALIGN_FLOOR((vaddr_t)nclv_va, PAGE_SIZE), 1,
        PTE_VM_FLAGS);

    vcpu_writereg(cpu()->vcpu, 0, 0);
}

static void sdsgx_delete(uint64_t enclave_id)
{
    struct vcpu* nclv = NULL;

    /* CROSSCON TODO: search for enclave vm in host vm and then retrieve vcpu */
    if ((nclv = sdsgx_get_nclv(cpu()->vcpu, enclave_id)) == NULL) {
        ERROR("non host invoked enclaved destruction");
    }

    vmm_destroy_dynamic(nclv->vm);

    vcpu_writereg(cpu()->vcpu, 0, 0);
}

static void sdsgx_ecall(uint64_t enclave_id, uint64_t args_addr, uint64_t sp_el0)
{
    int64_t res = HC_E_SUCCESS;
    struct vcpu* child = NULL;
    if ((child = sdsgx_get_nclv(cpu()->vcpu, enclave_id)) != NULL) {
        /* CROSSCON TODO separate architecture specific details. only works for Arm */
        /* child->arch.sysregs.vm.sp_el0 = sp_el0; */
        vmstack_push(child);
        vcpu_writereg(cpu()->vcpu, 1, args_addr);
        vcpu_writereg(cpu()->vcpu, 2, sp_el0);
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
    }
}

static void sdsgx_ocall(uint64_t idx, uint64_t ms)
{
    UNUSED_ARG(idx);
    UNUSED_ARG(ms);
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;

    enclave = vmstack_pop();
    if (enclave != NULL) {
        vcpu_writereg(cpu()->vcpu, 0, SDSGX_OCALL);
        vcpu_writereg(cpu()->vcpu, 1, vcpu_readreg(enclave, 1)); /* SP to be updated */
        vcpu_writereg(cpu()->vcpu, 2, vcpu_readreg(enclave, 2)); /* calloc size */
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
    }
}

static void sdsgx_resume(uint64_t enclave_id)
{
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;
    if ((enclave = sdsgx_get_nclv(cpu()->vcpu, enclave_id)) != NULL) {
        vmstack_push(enclave);
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
    }
}

static void sdsgx_exit(void)
{
    /* CROSSCON TODO: detect initialization successfull return */
    if (!cpu()->vcpu->nclv_data.initialized) {
        cpu()->vcpu->nclv_data.initialized = true;
    }

    /* go back to parent's context */
    vmstack_pop();

    /* return sucessfull to the host */
    vcpu_writereg(cpu()->vcpu, 0, 0);
}

extern uint64_t irqs;
extern uint64_t enclv_aborts;
static int64_t sdsgx_handle_hypercall(struct vcpu* vcpu, uint64_t fid)
{
    int64_t res = HC_E_SUCCESS;
    static unsigned int n_calls = 0;
    static unsigned int o_calls = 0;
    static unsigned int n_resumes = 0;

    uint64_t arg0 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(0));
    uint64_t arg1 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(1));
    uint64_t arg2 = vcpu_readreg(vcpu, HYPCALL_IN_ARG_REG(2));

    switch (fid) {
        case SDSGX_CREATE:
            sdsgx_create(arg1);
            break;

        case SDSGX_RESUME:
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

        case SDSGX_DELETE:
            sdsgx_delete(arg0);
            break;

        case SDSGX_ADD_RGN:
            sdsgx_add_rgn(arg0, arg1, arg2);
            break;
        case SDSGX_INFO:
            vcpu_writereg(cpu()->vcpu, 1, enclv_aborts);
            vcpu_writereg(cpu()->vcpu, 2, n_resumes);
            vcpu_writereg(cpu()->vcpu, 3, irqs);
            vcpu_writereg(cpu()->vcpu, 4, n_calls);
            vcpu_writereg(cpu()->vcpu, 5, o_calls);
            vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
            enclv_aborts = 0;
            n_calls = 0;
            o_calls = 0;
            n_resumes = 0;
            irqs = 0;
            enclv_aborts = 0;
            break;

        default:
            ERROR("Unknown command %d from vm %u", fid, cpu()->vcpu->vm->id);
            res = -HC_E_FAILURE;
            vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
    }

    // CROSSCON TODO check if needed
    if (cpu()->vcpu->state == VCPU_OFF) {
        cpu_standby();
    }

    return res;
}

uint64_t enclv_aborts = 0;
static int64_t sdsgx_handle_abort(struct vcpu* vcpu, uint64_t addr)
{
    int64_t res = HC_E_SUCCESS;
    struct vcpu* enclave = NULL;

    if (vcpu->vm->type != 3) {
        return 0;
    }

    enclv_aborts++;
    /* CROSSCON TODO: validate address space */

    enclave = vmstack_pop();
    if (enclave != NULL) {
        vcpu_writereg(cpu()->vcpu, 0, SDSGX_FAULT);
        vcpu_writereg(cpu()->vcpu, 1, enclave->vm->id);
        vcpu_writereg(cpu()->vcpu, 2, addr);
    } else {
        res = -HC_E_INVAL_ARGS;
        vcpu_writereg(cpu()->vcpu, 0, (unsigned long)res);
    }
    return 0;
}

#include <interrupts.h>
extern uint64_t interrupt_owner[MAX_INTERRUPT_LINES];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

uint64_t irqs = 0;
static void sdsgx_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    UNUSED_ARG(int_id);
    if (vcpu != cpu()->vcpu && vcpu->state == VCPU_STACKED) {
        /* TODO VM TYPES */
        if (cpu()->vcpu->vm->type == 3) { /* currently running enclave */
            if (cpu()->vcpu->nclv_data.initialized == false) {
                return;
            }
            vmstack_pop(); /* transition to normal world */
            irqs++;
            /* inform that enclave was interrupted */
            vcpu_writereg(cpu()->vcpu, 0, 1);
        }
    }
}

#include <vmm.h>
static struct hndl_hvc hvc = {
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdsgx_handle_hypercall,
};

static struct hndl_irq irq = {
    /* CROSSCON TODO: obtain this from config file */
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = { 27, 33, 72, 73, 74, 75, 76, 77, 78, 79 },
    .handler = sdsgx_handle_interrupt,
};

static struct hndl_mem_abort mem_abort = {
    .handler = sdsgx_handle_abort,
};

int64_t sdsgx_handler_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_hvc_add(vm, &hvc);
    vm_hndl_irq_add(vm, &irq);

    /* CROSSCON TODO */
    if (vm->type == 3) {
        vm_hndl_mem_abort_add(vm, &mem_abort);
    }

    return ret;
}
