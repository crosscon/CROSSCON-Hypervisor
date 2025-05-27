/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <asm_defs.h>
#include <arch/sysregs.h>
#include <arch/interrupts.h>

/*
 * TODO:ARMV8M
 * In Armv8-M the vector table is naturally aligned to a power of two, with an alignment value that
 * is, e.g., 128 bytes for CM33, 256 bytes for CM23. As such, this needs to be cpu-specific.
 *
 * The number of interrupts is also cpu-specific, e.g., 480 for CM33, 240 for CM23.
 */
#define VECTTBL_ALIGN   (0x80)

.macro define_handler name
    .thumb
    .thumb_func
    .global \name
    \name:
.endm

.syntax unified
.text

.macro VM_EXIT_ESF
    push {r0,r1}
    /* Read CPU pointer (psp) to r0 */
    mrs r1, psp
    ldr r0, [r1, #CPU_VCPU_OFF]

    /* Point to end of esf struct */
    add r0, #(VCPU_REGS_OFF+VCPU_ESF_OFF+VCPU_ESF_SIZE)

    /* Save all esf registers */
    mrs r1, psr
    stmdb r0!, {r1}
    stmdb r0!, {r2, r3, r12, lr}
    mov r12, r0
    pop {r0,r1}
    stmdb r12!, {r0, r1}
.endm

.macro VM_EXIT
    /* Read CPU pointer (psp) to r0 */
    mrs r1, psp
    ldr r0, [r1, #CPU_VCPU_OFF]

    //TODO:ARMV8M - This should be done only if we are scheduling another VM
    /* Save all special registers */
    add r0, #(VCPU_REGS_OFF)
    stmia r0!, {r4-r11,r14}

    mrs r1, msp_ns
    mrs r2, psp_ns
    mrs r3, msplim_ns
    mrs r4, psplim_ns
    mrs r5, basepri_ns
    mrs r6, primask_ns
    mrs r7, faultmask_ns
    mrs r8, control_ns

    /* Save all registers */
    add r0, #4                  /* Point to LR */
    stmia   r0!, {r1-r8}
.endm

.macro VM_ENTRY_ESF
    /* Read CPU pointer (psp) to r0 */
    mrs r1, psp
    ldr r0, [r1, #CPU_VCPU_OFF]

    /* Point to end of esf struct */
    add r0, #(VCPU_REGS_OFF+VCPU_ESF_OFF+VCPU_ESF_SIZE)

    /* Save all esf registers */
    ldmdb r0!, {r1}
    ldmdb r0!, {r2, r3, r12, lr}
    msr apsr_nzcvq, r1
    push {r4}
    mov r4, r0
    ldmdb r4!, {r0, r1}
    pop {r4}
.endm

.macro VM_ENTRY
    /* Read CPU pointer (psp) to r0 */
    mrs r0, psp                         //r0 pointing to cpu()
    ldr r1, [r0, #CPU_NEXT_VCPU_OFF]
    cmp r1, #0
    beq 11f
    ldr r3, [r0, #CPU_VCPU_OFF]
    cmp r1, r3
    beq 1f
    bl vcpu_context_switch

1:
    //TODO:ARMV8M - this needs optimization
    mrs r0, psp
    ldr r1, [r0, #CPU_NEXT_VCPU_OFF]
    mov r2, r1                          //r1 pointing to VCPU
    ldr r3, [r2, #VCPU_BLOCKED_COUNT]
    cmp r3, #0
    beq 2f
11:
    bl cpu_arch_park
    b .
2:
    add r1, #(VCPU_REGS_OFF+VCPU_SPREGS_OFF+VCPU_SPREGS_SIZE)
    /* Restore sp_regs */
    ldmdb r1!, {r2-r9}

    msr msp_ns,       r2  /* r2 (MSP)         */
    msr psp_ns,       r3  /* r3 (PSP)         */
    msr msplim_ns,    r4  /* r4 (MSPLIM)      */
    msr psplim_ns,    r5  /* r5 (PSPLIM)      */
    msr basepri_ns,   r6  /* r6 (BASEPRI)     */
    msr primask_ns,   r7  /* r7 (PRIMASK)     */
    msr faultmask_ns, r8  /* r8 (FAULTMASK)   */
    msr control_ns,   r9  /* r9 (CONTROL)     */

    /* Restore all registers */
    sub r1, #4                  /* Point to LR */
    ldmdb r1!, {r4-r11, lr}     /* R4-R11, R14 */

    //TODO:ARMV8-M - Still missing some procedures (Security Extensions on Dev Tools)
.endm

.balign VECTTBL_ALIGN
.global _hyp_vector_table
_hyp_vector_table:
    /* Exceptions */
    /*
     * In Armv8-M, the processor fetches the first two words of the vector table at each power-on/
     * reset sequence. The first is initial value of the MSP (stack pointer) and the second is the
     * reset vector. The reset vector is the address of the reset handler.
     */
    .word 0                     //TODO:ARMV8M should this be initialized with the real MSP value?
    .word _reset_handler
    .rept 13
    .word _exception_handler
    .endr
    .word _systick_handler
    /* Interrupts */
    .rept MAX_INTERRUPTS
    .word _irq_handler
    .endr


/* Handlers */
define_handler _exception_handler
_exception_handler:
    b   fault_exception_handler
    b   .

define_handler _irq_handler
_irq_handler:
    b   .

define_handler _systick_handler
_systick_handler:
    /*
    * This function is used to check if there is any active external interrupt. If there is, it will
    * return to the preempted VM. If not, it will proceed with the scheduling of the next VM.
    */
    push {lr}
    bl vm_nvic_act_irq
    pop {lr}
    cmp r0, #0
    bne excep_return
    VM_EXIT
    mov r0, #SYSTICK_INT_N
    bl interrupts_handle

.global vcpu_arch_entry
vcpu_arch_entry:
    VM_ENTRY
    mrs r0, psp
    ldr r1, [r0, #(CPU_NEXT_VCPU_OFF)]
    add r2, r1, #VCPU_FIRST_RUN_OFF
    ldr r3, [r2]
    cmp r3, #0x0
    beq first_entry
excep_return:
    bx lr
first_entry:
    mov r3, #0xF
    str r3, [r2]
    ldr r0, [r1, #(VCPU_REGS_OFF+VCPU_REGS_PC_OFF)]
    /* Clear systick active bit */
    ldr r1, =SCB_BASE
    ldr r2, [r1, #SCB_SHCSR_OFF]
    bic r2, r2, #SCB_SHCSR_SYSTICKACT
    str r2, [r1, #SCB_SHCSR_OFF]
    /* Sanitize callee-saved registers */
    mov r12, #0
    mov r4, #0
    mov r3, #0
    mov r2, #0
    mov r1, #0
    bxns r0

define_handler _hypercall_handler
hypercall_handler:
    cpsid i
    push {r0}
    VM_EXIT_ESF
    VM_EXIT
    pop {r0}
    bl arch_hypercall
    VM_ENTRY
    VM_ENTRY_ESF
    cpsie i
    bxns lr
