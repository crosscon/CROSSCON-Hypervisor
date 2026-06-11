#include <virtTEE_V.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include <crossconhyp.h>
#include <types.h>
#include <vm.h>
#include <vmm.h>
#include <arch/virtTEE_V.h>

static long mtower_handle_nw(struct vcpu* ree_vcpu)
{
    long ret = -HC_E_FAILURE;
    if (vmstack_pop() != NULL) {
        tee_arch_interrupt_disable();
        virtteev_copy_args(cpu()->vcpu, ree_vcpu, 3);
        ret = HC_E_SUCCESS;
    }
    return ret;
}

static long mtower2_handle_nw(struct vcpu* ree_vcpu, uint32_t fid)
{
    long ret = -HC_E_FAILURE;

    struct vcpu* tee_vcpu = vcpu_get_child(ree_vcpu, FID_GET_CHILD_IDX(fid));

    if (tee_vcpu != NULL) {
        switch (ID_TO_FUNCID(fid)) {
            case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
            case TEEHC_FUNCID_RETURN_ON_DONE:
                virtteev_copy_args(tee_vcpu, cpu()->vcpu, 3);
                vmstack_push(tee_vcpu);
                tee_arch_interrupt_enable();
                break;
            case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                vmstack_push(tee_vcpu);
                break;
            default:
                ERROR("unknown tee call %0lx by vm %d", fid, cpu()->vcpu->vm->id);
        }
        ret = HC_E_SUCCESS;
    }

    return ret;
}

static long mtower_handle_sw(struct vcpu* mtower_vcpu, uint32_t fid)
{
    long ret = -HC_E_FAILURE;

    struct vcpu* ree_vcpu = vcpu_get_child(mtower_vcpu, FID_GET_CHILD_IDX(fid));

    if (ree_vcpu != NULL) {
        switch (ID_TO_FUNCID(fid)) {
            case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
            case TEEHC_FUNCID_RETURN_ON_DONE:
                virtteev_copy_args(ree_vcpu, cpu()->vcpu, 1);
                vmstack_push(ree_vcpu);
                tee_arch_interrupt_enable();
                break;
            case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                vmstack_push(ree_vcpu);
                break;
            default:
                ERROR("unknown tee call %0lx by vm %d", fid, cpu()->vcpu->vm->id);
        }
        ret = HC_E_SUCCESS;
    }

    return ret;
}

static long mtower2_handle_sw(struct vcpu* mtower_vcpu)
{
    long ret = -HC_E_FAILURE;
    if (vmstack_pop() != NULL) {
        tee_arch_interrupt_disable();
        virtteev_copy_args(cpu()->vcpu, mtower_vcpu, 1);
        ret = HC_E_SUCCESS;
    }
    return ret;
}

long virtteev_handler(struct vcpu* vcpu, uint32_t fid)
{
    long ret = -HC_E_FAILURE;

    if (vcpu->vm->type == 0) {
        /* normal world */
        if (fid & FID_HIERARCHY_BIT){    /* fid bit 28 indicates if invoke to parent or child */
            ret = mtower2_handle_nw(vcpu, fid);     /* FreeRTOS + child TEE */
        }else{
            ret = mtower_handle_nw(vcpu);           /* FreeRTOS + parent TEE */
        }
    } else {
        /* secure world */
        if (cpu()->vcpu->vm->type == 1) {       
            /* limitation for 3 generations of VMs: grandparent, parent & child */
            if (vcpu_get_child(vcpu, 0) != NULL){
                ret = mtower_handle_sw(vcpu, fid);  /* host secure world (grandparent) */
            }else{
                ret = mtower2_handle_sw(vcpu);  /* guest secure world (child) */
            }
        }
    }

    return ret;
}

static void virtteev_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    if (vcpu != cpu()->vcpu && vcpu->state == VCPU_INACTIVE) {
        if (cpu()->vcpu->vm->type == 1) {
            /* CROSSCON TODO: handle TEE preemption/context switch if required */
        }
    }
    interrupts_vm_inject(vcpu, int_id);
}

static int32_t virtteev_handle_abort(struct vcpu* vcpu, long unsigned addr)
{
    UNUSED_ARG(addr);
    int32_t res = HC_E_SUCCESS;

    if (vcpu->vm->type == 1) {
        struct vcpu* ree_vcpu = vcpu_get_child(vcpu, 0);
        if (ree_vcpu != NULL) {
            vmstack_push(ree_vcpu);
        }
        INFO("VM %d performed illegal access at 0x%x. Disabling.\n", vcpu->vm->id, addr);
        tee_arch_interrupt_enable();
    }

    /* CROSSCON TODO: arch specific */
    vcpu_writereg(cpu()->vcpu, 10, 0x7);
    return res;
}

static struct hndl_irq irq = {
    /* CROSSCON TODO: obtain this from config file */
    /* CROSSCON TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = { 27, 33, 72, 73, 74, 75, 76, 77, 78, 79 },
    .handler = virtteev_handle_interrupt,
};

static struct hndl_mem_abort mem_abort = {
    .handler = virtteev_handle_abort,
};

int32_t virtteev_handler_setup(struct vm* vm){

    int32_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    virtteev_arch_handler_setup(vm);
    
    vm_hndl_irq_add(vm, &irq);
    vm_hndl_mem_abort_add(vm, &mem_abort);

    return ret;
}
