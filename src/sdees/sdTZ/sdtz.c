#include <sdtz.h>
#include <hypercall.h>
#include <vmstack.h>
#include <arch/tee.h>
#include <config.h>
#include "types.h"
#include "vm.h"
#include "vmm.h"

#define PREFIX_MASK 0xff000000
#define ID_TO_FUNCID(x) (x & ~(PREFIX_MASK))
#define PREFIX 0
#define TEEHC_FUNCID_RETURN_ENTRY_DONE          (PREFIX | 0)
#define TEEHC_FUNCID_RETURN_ON_DONE             (PREFIX | 1)
#define TEEHC_FUNCID_RETURN_OFF_DONE            (PREFIX | 2)
#define TEEHC_FUNCID_RETURN_SUSPEND_DONE        (PREFIX | 3)
#define TEEHC_FUNCID_RETURN_RESUME_DONE         (PREFIX | 4)
#define TEEHC_FUNCID_RETURN_CALL_DONE           (PREFIX | 5)
#define TEEHC_FUNCID_RETURN_FIQ_DONE            (PREFIX | 6)
#define TEEHC_FUNCID_RETURN_SYSTEM_OFF_DONE     (PREFIX | 7)
#define TEEHC_FUNCID_RETURN_SYSTEM_RESET_DONE   (PREFIX | 8)

#define OPTEE_MSG_CMD_OPEN_SESSION	    U(0)
#define OPTEE_MSG_CMD_INVOKE_COMMAND	U(1)
#define OPTEE_MSG_CMD_CLOSE_SESSION	    U(2)
#define OPTEE_MSG_CMD_CANCEL		    U(3)
#define OPTEE_MSG_CMD_REGISTER_SHM	    U(4)
#define OPTEE_MSG_CMD_UNREGISTER_SHM	U(5)
#define OPTEE_MSG_CMD_DO_BOTTOM_HALF	U(6)
#define OPTEE_MSG_CMD_STOP_ASYNC_NOTIF	U(7)
#define OPTEE_MSG_FUNCID_CALL_WITH_ARG	U(0x0004)

#define TEEHC_FUNCID_CLIENT_FLAG     (0x80000000)
#define TEE_NUM_ARGS    (6)

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

int64_t sdtz_handler(struct vcpu* vcpu, uint64_t fid) {
    int64_t ret = -HC_E_FAILURE;

    if (vcpu->vm->id == 2) {
	/* normal world */
        if (vmstack_pop() != NULL) {
	    tee_arch_interrupt_disable();
            sdtz_copy_args(cpu.vcpu, vcpu, 7);
            /* TODO: more generic stepping */
            uint64_t pc_step = 2 + (2 * 1);
            vcpu_writepc(cpu.vcpu, vcpu_readpc(cpu.vcpu) + pc_step);
            ret = HC_E_SUCCESS;
        }
    } else {
	/* secure world */
        /* TODO: get parent */
        struct vcpu *ree_vcpu = vcpu_get_child(vcpu, 0);
        if (ree_vcpu != NULL) {
            switch (ID_TO_FUNCID(fid)) {
                case TEEHC_FUNCID_RETURN_CALL_DONE:
                    sdtz_copy_args_call_done(ree_vcpu, cpu.vcpu, 4);
                    /* fallthough */
                case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                    vmstack_push(ree_vcpu);
		    tee_arch_interrupt_enable();
                    break;
                default:
                    ERROR("unknown tee call %0lx", fid);
            }
            ret = HC_E_SUCCESS;
        }
    }
    return ret;
}

int64_t sdtz_sbi_handler(struct vcpu* vcpu, uint64_t smc_fid) {
    int64_t ret = -HC_E_FAILURE;

    ret = sdtz_handler(vcpu, smc_fid);
    return ret;
}

int64_t sdtz_smc_handler(struct vcpu* vcpu, uint64_t smc_fid) {

    int64_t ret = -HC_E_FAILURE;

    struct vcpu *calling_vcpu = cpu.vcpu;

    if (calling_vcpu->vm->id == 2) { /* normal world */
        if (is_psci_fid(smc_fid)) {
            /* TODO: signal trusted OS a PSCI event is comming up */
            /* potentially handle core going to sleep */
            return HC_E_SUCCESS;
        } else {
            /* TODO: If SMC call is for trusted OS */
            ret = sdtz_handler(vcpu, smc_fid);
        }
    }else{
            ret = sdtz_handler(vcpu, smc_fid);
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
   if(vcpu != cpu.vcpu && vcpu->state == VCPU_STACKED){
       if(cpu.vcpu->vm->id == 1){ /* currently running secure world */
           vmstack_push(vcpu); /* transition to normal world */
       }
   }
}

static struct hndl_smc smc = {
    /* TODO: obtain this to decide whether to invoke handler early on */
    .end = 0xffff0000,
    .start = 0x00000000,
    .handler = sdtz_smc_handler,
};

static struct hndl_irq irq = {
    /* TODO: obtain this from config file */
    /* TODO: obtain this to decide whether to invoke handler early on */
    .num = 10,
    .irqs = {27,33,72,73,74,75,76,77,78,79},
    .handler = sdtz_handle_interrupt,
};

int64_t sdtz_handler_setup(struct vm *vm)
{
    int64_t ret = 0;

    if(vm == NULL)
        return -1;

    /* TODO: check config structure or something to check if this VMs wants tz
     * to handle its events */
    vm_hndl_smc_add(vm, &smc);
    vm_hndl_irq_add(vm, &irq);

    return ret;
}
