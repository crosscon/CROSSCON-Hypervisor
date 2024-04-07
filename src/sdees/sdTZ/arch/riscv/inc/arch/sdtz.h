#ifndef SDTZ_ARMV8_H
#define SDTZ_ARMV8_H

#include <vm.h>

#define SBI_EXTID_TEE (0x544545)

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

int64_t sdtz_arch_handler_setup(struct vm *vm);

void tee_arch_interrupt_disable();
void tee_arch_interrupt_enable();
void tee_step(struct vcpu* vcpu);

#endif
