#include <tee.h>
#include <hypercall.h>
#include <vmstack.h>

#define TEEHC_FUNCID_RETURN_ENTRY_DONE          0
#define TEEHC_FUNCID_RETURN_ON_DONE             1
#define TEEHC_FUNCID_RETURN_OFF_DONE            2
#define TEEHC_FUNCID_RETURN_SUSPEND_DONE        3
#define TEEHC_FUNCID_RETURN_RESUME_DONE         4
#define TEEHC_FUNCID_RETURN_CALL_DONE           5
#define TEEHC_FUNCID_RETURN_FIQ_DONE            6
#define TEEHC_FUNCID_RETURN_SYSTEM_OFF_DONE     7
#define TEEHC_FUNCID_RETURN_SYSTEM_RESET_DONE   8

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

static inline void tee_clear_arg0_client_flag(struct vcpu *vcpu) {
    // Clear the clint flag from the first argument
    unsigned long a0 =
        vcpu_readreg(vcpu, HYPCALL_ARG_REG(0)) & ~(TEEHC_FUNCID_CLIENT_FLAG);
    vcpu_writereg(vcpu, HYPCALL_ARG_REG(0), a0);
}

int64_t tee_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {

    int64_t ret = -HC_E_FAILURE;

    struct vcpu *calling_vcpu = cpu.vcpu;

    if (calling_vcpu->vm->id == 2) {
        if (vmstack_pop() != NULL) {
            tee_copy_args(cpu.vcpu, calling_vcpu, 6);
            /* tee_clear_arg0_client_flag(cpu.vcpu); */
            ret = HC_E_SUCCESS;
        }
    } else {
        // in this model, the always has only 1 child vm, hence child = 0
        struct vcpu *ree_vcpu = vcpu_get_child(cpu.vcpu, 0); 
        if (ree_vcpu != NULL) {
            switch (id) {
                case TEEHC_FUNCID_RETURN_CALL_DONE:
                    tee_copy_args_call_done(ree_vcpu, cpu.vcpu, 5);
                    /* fallthough */
                case TEEHC_FUNCID_RETURN_ENTRY_DONE:
                    vmstack_push(ree_vcpu);
                    break;
                default:
                    WARNING("unknown tee hypercal %d", id);
            }
            ret = HC_E_SUCCESS;
        }
    }

    return ret;
}
