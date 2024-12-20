#include <sdtz.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include "crossconhyp.h"
#include "types.h"
#include "vm.h"
#include "vmm.h"
#include <arch/sdtz.h>

static int optee_crash = 0;
static int optee2_crash = 0;

static inline void sdtz_copy_args(struct vcpu *vcpu_dst, struct vcpu *vcpu_src,
        size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid));
    }
}

static inline void sdtz_copy_args_call_done(struct vcpu *vcpu_dst, struct vcpu
        *vcpu_src, size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid+1));
    }
}

int64_t optee_handle_nw(struct vcpu* ree_vcpu)
{
    int64_t ret = -HC_E_FAILURE;
    if (vmstack_pop() != NULL) {
        tee_arch_interrupt_disable();
        sdtz_copy_args(cpu.vcpu, ree_vcpu, 7);
        /* TODO: more generic stepping */
        /* in arm steeping is done here, but in RISC-V it is done outside */
        tee_step(cpu.vcpu);
        ret = HC_E_SUCCESS;
    }
    return ret;
}

int64_t optee2_handle_nw(struct vcpu* ree_vcpu)
{
    int64_t ret = -HC_E_FAILURE;
    struct vcpu* optee_vcpu = vcpu_get_child(ree_vcpu, 0);
    if(optee_vcpu == NULL){
        ret = HC_E_SUCCESS;
        vcpu_writereg(ree_vcpu, 0, -1);
        return ret;
    }

    tee_arch_interrupt_disable();
    if(optee_vcpu->vm->type == 2){
        vmstack_push(optee_vcpu);
        sdtz_copy_args(cpu.vcpu, ree_vcpu, 7);
        /* TODO: more generic stepping */
        /* in arm steeping is done here, but in RISC-V it is done outside */
        tee_step(cpu.vcpu);
        ret = HC_E_SUCCESS;
    }
    return ret;
}


int64_t optee_handle_sw(struct vcpu* optee_vcpu, uint64_t fid)
{
    int64_t ret = -HC_E_FAILURE;
    struct vcpu *ree_vcpu = vcpu_get_child(optee_vcpu, 0);
    if (ree_vcpu != NULL) {
        /* There is bulshit when copying regsiters */
        switch (ID_TO_FUNCID(fid)) {
            case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
            case TEEHC_FUNCID_RETURN_ON_DONE:
                sdtz_copy_args(ree_vcpu, cpu.vcpu, 1);
                vmstack_push(ree_vcpu);
                tee_arch_interrupt_enable();
                break;
            case TEEHC_FUNCID_RETURN_CALL_DONE:
                if(vcpu_readreg(cpu.vcpu, 1) == 0xffff0004){
                    /* interrupted */
                    /* TODO Not sure if needed */
                    sdtz_copy_args_call_done(ree_vcpu, cpu.vcpu, 4);
                } else
                    sdtz_copy_args_call_done(ree_vcpu, cpu.vcpu, 6);
                vmstack_push(ree_vcpu);
                tee_arch_interrupt_enable();
                break;
            case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                vmstack_push(ree_vcpu);
                struct vcpu *guest_vcpu = vcpu_get_child(ree_vcpu, 0);
                if(guest_vcpu != NULL)
                    vmstack_push(guest_vcpu);

                break;
            default:
                ERROR("unknown tee call %0lx by vm %d", fid, cpu.vcpu->vm->id);
        }
        ret = HC_E_SUCCESS;
    }

    return ret;
}

int64_t optee2_handle_sw(struct vcpu* optee_vcpu, uint64_t fid)
{
    int64_t ret = -HC_E_FAILURE;
    struct vcpu *guest_vcpu = vmstack_pop();
    (void) guest_vcpu;
    struct vcpu *ree_vcpu = cpu.vcpu;
    if (ree_vcpu != NULL) {
        switch (ID_TO_FUNCID(fid)) {
            case TEEHC_FUNCID_RETURN_SUSPEND_DONE:
            case TEEHC_FUNCID_RETURN_ON_DONE:
                sdtz_copy_args(ree_vcpu, optee_vcpu, 1);
                tee_arch_interrupt_enable();
                break;
            case TEEHC_FUNCID_RETURN_CALL_DONE:
                if(vcpu_readreg(cpu.vcpu, 1) == 0xffff0004){
                    /* interrupted */
                    /* TODO Not sure if needed */
                    sdtz_copy_args_call_done(ree_vcpu, optee_vcpu, 4);
                } else
                    sdtz_copy_args_call_done(ree_vcpu, optee_vcpu, 6);
            case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                tee_arch_interrupt_enable();
                break;
            default:
                ERROR("unknown tee call %0lx by vm %d", fid, cpu.vcpu->vm->id);
        }
        ret = HC_E_SUCCESS;
    }

    return ret;
}

#define ARM_SMCCC_OWNER_MASK	0x3F
#define ARM_SMCCC_OWNER_SHIFT	24

#define GET_OWNER(x) (((x) >> ARM_SMCCC_OWNER_SHIFT) & ARM_SMCCC_OWNER_MASK)
#define IS_OPTEE(x)  (GET_OWNER(x) >= (0x32) && GET_OWNER(x) <= (0x3f))
#define IS_OPTEE2(x) (GET_OWNER(x) >= (0x12) && GET_OWNER(x) <= (0x1f))

int64_t sdtz_handler(struct vcpu* vcpu, uint64_t fid) {
    int64_t ret = -HC_E_FAILURE;

    if (vcpu->vm->type == 0) {
	/* normal world */
        if(IS_OPTEE(fid)) {
            if(!optee_crash){
                ret = optee_handle_nw(vcpu);
            } else {
                /* TODO: arch specific */
                vcpu_writereg(cpu.vcpu, 10, 0x7);
            }
        } else if(IS_OPTEE2(fid)) {
            if(!optee2_crash){
                ret = optee2_handle_nw(vcpu);
            } else {
                /* TODO: arch specific */
                vcpu_writereg(cpu.vcpu, 10, 0x7);
            }
        }
    } else {
        /* secure world */
        /* TODO: get parent */
        if(cpu.vcpu->vm->type == 1){ /* host secure world */
            ret = optee_handle_sw(vcpu, fid);
        } else if (cpu.vcpu->vm->type == 2){ /* guest secure world */
            ret = optee2_handle_sw(vcpu, fid);
        }
    }

    return ret;
}

extern uint64_t interrupt_owner[MAX_INTERRUPTS];
static inline uint64_t interrupts_get_vmid(uint64_t int_id)
{
    return interrupt_owner[int_id];
}

void sdtz_handle_interrupt(struct vcpu* vcpu, irqid_t int_id)
{
    /* TODO: check current active handler */

   if(vcpu != cpu.vcpu && vcpu->state == VCPU_INACTIVE){
       if (cpu.vcpu->vm->type == 1) {
           /* TODO */
           /* interrupts_vm_inject(cpu.vcpu, 40); */
       }
   }
}

int64_t sdtz_handle_abort(struct vcpu* vcpu, uint64_t addr)
{
    int64_t res = HC_E_SUCCESS;

    if(vcpu->vm->type == 1){
        struct vcpu *ree_vcpu = vcpu_get_child(vcpu, 0);
        if (ree_vcpu != NULL) {
            vmstack_push(ree_vcpu);
        }
        optee_crash = 1;
        INFO("VM %d performed illegal access. Disabling.", vcpu->vm->id);
        tee_arch_interrupt_enable();
    } else if(vcpu->vm->type == 2){
        vmstack_pop();
        tee_arch_interrupt_enable();
        optee2_crash = 1;
        INFO("VM %d performed illegal access. Disabling.", vcpu->vm->id);
    }

    /* TODO: arch specific */
    vcpu_writereg(cpu.vcpu, 10, 0x7);
    return res;
}

static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    /* TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = sdtz_handle_interrupt,
};

static struct hndl_mem_abort mem_abort = {
    .handler = sdtz_handle_abort,
};


int64_t sdtz_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    sdtz_arch_handler_setup(vm);

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_irq_add(vm, &irq);
    vm_hndl_mem_abort_add(vm, &mem_abort);

    return ret;
}
