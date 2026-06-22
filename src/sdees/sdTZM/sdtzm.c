#include <sdtzm.h>
#include <hypercall.h>
#include <vmstack.h>
#include <config.h>
#include <crossconhyp.h>
#include <types.h>
#include <vm.h>
#include <vmm.h>
#include <arch/sdtzm.h>

// #define TEE_BASE_HC_SG_ID 0x2 //the lower number for SG TEEs
#define MAX_STACK_LEVEL 0x4 //the lower number for SG TEEs
//#define MTOWER2_HC_SG_ID 0x3

uint32_t stack_level_after_boot = 1; 
uint32_t curr_stack_level = 1; 
static int optee_crash = 0;

static long mtower_handle_nw(struct vcpu* ree_vcpu, uint32_t fid)
{
    long ret = -HC_E_FAILURE;
    uint32_t pop_num = 0;
 
    if(fid < MAX_STACK_LEVEL) { //if hypercall reieved wants to invoke TEE on stack, should be a TEE lower in stack

        pop_num = stack_level_after_boot-fid;

        for(uint32_t i=0; i<pop_num; i++){
            if (vmstack_pop() == NULL) {
                return ret;
            }
            curr_stack_level--;
        }
    
        tee_arch_interrupt_disable(); 
        sdtzm_copy_args(cpu()->vcpu, ree_vcpu, 3);
        /* CROSSCON TODO: more generic stepping */
        /* in arm steeping is done here, but in RISC-V it is done outside */
        tee_step(cpu()->vcpu);
        ret = HC_E_SUCCESS;  
    }
 
    // if(fid == MTOWER_HC_SG_ID) {
    //     //CROSSCON TODO: ADD switch case for different calls:  CROSSCON_HC_SG_ID

    //      //CROSSCON TODO: complete interrupt disable function
    //     tee_arch_interrupt_disable(); 
    //     sdtzm_copy_args(cpu()->vcpu, ree_vcpu, 3);
    //     /* CROSSCON TODO: more generic stepping */
    //     /* in arm steeping is done here, but in RISC-V it is done outside */
    //     tee_step(cpu()->vcpu);
    //     ret = HC_E_SUCCESS;       

    //     if (vmstack_pop() != NULL) {
    //         //CROSSCON TODO: complete interrupt disable function
    //         tee_arch_interrupt_disable(); 
    //         sdtzm_copy_args(cpu()->vcpu, ree_vcpu, 3);
    //         /* CROSSCON TODO: more generic stepping */
    //         /* in arm steeping is done here, but in RISC-V it is done outside */
    //         tee_step(cpu()->vcpu);
    //         ret = HC_E_SUCCESS;
    //     }
    // }else if(fid == MTOWER2_HC_SG_ID) {
    //     //CROSSCON TODO: Get child using fid number;
    //     //mtower 2 is a child of FreeRTOS so it needs to get ree_vcpu child
    //     struct vcpu* tee_vcpu = vcpu_get_child(ree_vcpu, 0);
    //     if(tee_vcpu != NULL){
    //         tee_arch_interrupt_disable();
    //         vmstack_push(tee_vcpu);
    //         sdtzm_copy_args(cpu()->vcpu, ree_vcpu, 3);
    //         tee_step(cpu()->vcpu);
    //         ret = HC_E_SUCCESS;
    //     }
    // }
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
            case TEEHC_FUNCID_BOOT:
                vmstack_push(ree_vcpu);
                stack_level_after_boot++;
                curr_stack_level++;
                break;
            case TEEHC_FUNCID_BLNS:
                while(curr_stack_level != stack_level_after_boot){ //since last stack level is always the REE, we can just push the child vcpu until we reach the stack level after a complete boot
                    //push of all missing stack layers
                    vmstack_push(ree_vcpu);
                    ree_vcpu = vcpu_get_child(ree_vcpu, 0);
                    curr_stack_level++;
                }
                break;
            default:
                ERROR("unknown tee call %0lx by vm %d", fid, cpu()->vcpu->vm->id);
        }
        ret = HC_E_SUCCESS;
    }

    return ret;
}
// static long mtower2_handle_sw(void)
// {
//     long ret = -HC_E_FAILURE;

//     //we only use a case to handle secure world hypercalls, which is the case where TEE wants to return to normal world
//     if (vmstack_pop() != NULL) {
//         //CROSSCON TODO: complete interrupt disable function
//         tee_arch_interrupt_disable(); 
//         //since we use shared memory to pass arguments we do not need to pass arguments back to normal world
//         //in this sense, we do not use this: sdtzm_copy_args(ree_vcpu, cpu()->vcpu, 1);
//         tee_step(cpu()->vcpu);
//         ret = HC_E_SUCCESS;
//     }

//     return ret;
// }

long sdtzm_handler(struct vcpu* vcpu, uint32_t fid)
{
    long ret = -HC_E_FAILURE;

    if (vcpu->vm->type == 0) { //freeRTOS did a HC
        /* normal world call SG */
        /* FID means the mTower ID, e.g., mTower1 or mTower2 */
        ret = mtower_handle_nw(vcpu, fid);
    } else if (vcpu->vm->type == 1) { //mtower1 did a HC
        /* secure world wants to go back to normal world */
        /* FID means the function ID*/
        ret = mtower_handle_sw(vcpu, fid);
    }// else if (vcpu->vm->type == 2) { //mtower2 did a HC
    //     /* secure world wants to go back to normal world */
    //     /* FID means the function ID*/
    //     ret = mtower2_handle_sw();
    // }

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
            //interrupt occurred during secure world execution
            //who belongs the interrupt?
            /* interrupts_vm_inject(cpu()->vcpu, 40); */
        }else if (cpu()->vcpu->vm->type == 0) {
            /* CROSSCON TODO */
            //interrupt occurred during normal world execution
            //who belongs the interrupt?
            /* interrupts_vm_inject(cpu()->vcpu, 40); */
        }
    }
}

static int32_t sdtzm_handle_abort(struct vcpu* vcpu, long unsigned addr)
{
    UNUSED_ARG(addr);
    int32_t res = HC_E_SUCCESS;

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
    .num = 4,
    .irqs = { 32, 33, 78, 79 },
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
