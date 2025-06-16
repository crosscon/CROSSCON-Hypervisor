#include <sdtzm.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include <crossconhyp.h>
#include <types.h>
#include <vm.h>
#include <vmm.h>
#include <arch/sdtzm.h>

static int optee_crash = 0;

static long mtower_handle_nw(struct vcpu* ree_vcpu)
{
    long ret = -HC_E_FAILURE;
    if (vmstack_pop() != NULL) {
        tee_arch_interrupt_disable();
        sdtzm_copy_args(cpu()->vcpu, ree_vcpu, 3);
        /* CROSSCON TODO: more generic stepping */
        /* in arm steeping is done here, but in RISC-V it is done outside */
        tee_step(cpu()->vcpu);
        ret = HC_E_SUCCESS;
    }
    return ret;
}

static long mtower_handle_sw(struct vcpu* mtower_vcpu, uint64_t fid)
{
    long ret = -HC_E_FAILURE;
    struct vcpu* ree_vcpu = vcpu_get_child(mtower_vcpu, 0);
    if (ree_vcpu != NULL) {
        /* There is bulshit when copying regsiters */
        switch (ID_TO_FUNCID(fid)) {
            case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
            case TEEHC_FUNCID_RETURN_ON_DONE:
                sdtzm_copy_args(ree_vcpu, cpu()->vcpu, 1);
                vmstack_push(ree_vcpu);
                tee_arch_interrupt_enable();
                break;
                /* TODO */
            /* case TEEHC_FUNCID_RETURN_CALL_DONE: */
            /*     if (vcpu_readreg(cpu()->vcpu, 1) == 0xffff0004) { */
            /*         /1* interrupted *1/ */
            /*         /1* CROSSCON TODO Not sure if needed *1/ */
            /*         sdtzm_copy_args_call_done(ree_vcpu, cpu()->vcpu, 4); */
            /*     } else { */
            /*         sdtzm_copy_args_call_done(ree_vcpu, cpu()->vcpu, 6); */
            /*     } */
            /*     vmstack_push(ree_vcpu); */
            /*     tee_arch_interrupt_enable(); */
            /*     break; */
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

long sdtzm_handler(struct vcpu* vcpu, uint64_t fid)
{
    long ret = -HC_E_FAILURE;

    if (vcpu->vm->type == 0) {
        /* normal world */
        ret = mtower_handle_nw(vcpu);
    } else {
        /* secure world */
        /* CROSSCON TODO: get parent */
        if (cpu()->vcpu->vm->type == 1) {        /* host secure world */
            ret = mtower_handle_sw(vcpu, fid);
        }
    }

    return ret;
}

extern uint64_t interrupt_owner[MAX_INTERRUPT_LINES];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

static void sdtzm_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    UNUSED_ARG(int_id);
    /* CROSSCON TODO: check current active handler */

    if (vcpu != cpu()->vcpu && vcpu->state == VCPU_INACTIVE) {
        if (cpu()->vcpu->vm->type == 1) {
            /* CROSSCON TODO */
            /* interrupts_vm_inject(cpu()->vcpu, 40); */
        }
    }
}

static int64_t sdtzm_handle_abort(struct vcpu* vcpu, long unsigned addr)
{
    UNUSED_ARG(addr);
    int64_t res = HC_E_SUCCESS;

    if (vcpu->vm->type == 1) {
        struct vcpu* ree_vcpu = vcpu_get_child(vcpu, 0);
        if (ree_vcpu != NULL) {
            vmstack_push(ree_vcpu);
        }
        optee_crash = 1;
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
    .handler = sdtzm_handle_interrupt,
};

static struct hndl_mem_abort mem_abort = {
    .handler = sdtzm_handle_abort,
};

int64_t sdtzm_handler_setup(struct vm* vm)
{
    int64_t ret = 0;

    if (vm == NULL) {
        return -1;
    }

    sdtzm_arch_handler_setup(vm);

    /* CROSSCON TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_irq_add(vm, &irq);
    vm_hndl_mem_abort_add(vm, &mem_abort);

    return ret;
}
