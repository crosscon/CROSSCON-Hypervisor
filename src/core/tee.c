#include <tee.h>
#include <hypercall.h>
#include <vmstack.h>
#include <arch/tee.h>

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

static inline void tee_copy_args(struct vcpu *vcpu_dst, struct vcpu *vcpu_src, size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid));
    }
}

static inline void tee_copy_args_call_done(struct vcpu *vcpu_dst, struct vcpu *vcpu_src, size_t num_args) {
    for (size_t i = 0; i < num_args; i++) {
        size_t regid = HYPCALL_ARG_REG(i);
        vcpu_writereg(vcpu_dst, regid, vcpu_readreg(vcpu_src, regid+1));
    }
}

int64_t tee_handler(uint64_t id) {

    int64_t ret = -HC_E_FAILURE;

    struct vcpu *calling_vcpu = cpu.vcpu;

    if (calling_vcpu->vm->id == 2) {
        if (vmstack_pop() != NULL) {
	    tee_arch_interrupt_disable();
            tee_copy_args(cpu.vcpu, calling_vcpu, 7);
            ret = HC_E_SUCCESS;
        }
    } else {
        // in this model, the always has only 1 child vm, hence child = 0
        struct vcpu *ree_vcpu = vcpu_get_child(cpu.vcpu, 0);
        if (ree_vcpu != NULL) {
            switch (ID_TO_FUNCID(id)) {
                case TEEHC_FUNCID_RETURN_CALL_DONE:
                    tee_copy_args_call_done(ree_vcpu, cpu.vcpu, 4);
                    /* fallthough */
                case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                    vmstack_push(ree_vcpu);
		    tee_arch_interrupt_enable();
                    break;
                default:
                    ERROR("unknown tee call %0lx", id);
            }
            ret = HC_E_SUCCESS;
        }
    }

    return ret;
}
