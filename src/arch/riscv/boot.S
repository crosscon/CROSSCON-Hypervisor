/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <arch/crossconhyp.h>
#include <arch/csrs.h>
#include <asm_defs.h>
#include <platform_defs.h>
#include <bit.h>

.macro LD_SYM       rd, sym
    la      \rd, \sym
    LOAD      \rd, 0(\rd)
.endm

.data 
.align 3
/**
 * barrier is used to minimal synchronization in boot - other cores wait for bsp to set it.
 */
_barrier: .8byte 0		

/**
 * 	The following code MUST be at the base of the image, as this is bao's entrypoint. Therefore
 * .boot section must also be the first in the linker script. DO NOT implement any code before the
 * _reset_handler in this section.
 */
 .section ".boot_head", "ax"
.globl _reset_handler

_reset_handler:

    /**
    * The following registers are reserved to be passed to init function as arguments:
    *      a0 -> hart id (passed by sbi enviroment)
    *      a1 -> contains image base load address
    * The remaining code must use t0-t6 as scratchpad registers in the main flow and s0-s5 in
    * auxiliary routines. s6-s11 are used to hold constants a3-a7 are used as arguments and return
    * values (can be also corrputed in auxiliary routines).
    */

    la      a1, _image_start
    la      a3, _load_addr
    STORE   a1, 0(a3)

    /**
     * Setup stvec early. In case of we cause an exception in this boot code we end up at a known
     * place.
     */
    la      t0, _hyp_trap_vector
    and     t0, t0, ~STVEC_MODE_MSK
    or      t0, t0, STVEC_MODE_DIRECT
    csrw    stvec, t0

    /**
     * Bring processor to known supervisor state: make sure interrupts and memory translation are
     * disabled.
     */

    csrw   sstatus, zero
    csrw   sie, zero
    csrw   sip, zero
    csrw   satp, zero 


#if defined(CPU_MASTER_FIXED)
/**
 * All cpus set the CPU_MASTER with the same value to avoid synchronization.    
 */
    la      t0, CPU_MASTER
    li      t1, CPU_MASTER_FIXED
    STORE   t1, 0(t0)
#else
/**
 * The first hart to grab the lock is CPU_MASTER.
 */
.pushsection .data
_boot_lock:
    .4byte 0
.popsection
    la      t0, _boot_lock
    li      t1, 1
1:
    lr.w    t2, (t0)
    bnez    t2, 2f
    sc.w    t2, t1, (t0)   
    bnez    t2, 1b 
    la      t0, CPU_MASTER
    STORE      a0, 0(t0)
2:
#endif

    call boot_arch

    /* now set stvec in virtual address space */
    la      t0, _hyp_trap_vector
    and     t0, t0, ~STVEC_MODE_MSK
    or      t0, t0, STVEC_MODE_DIRECT
    csrw   stvec, t0

    /* Init stack pointer and global pointer */
    csrr    sp, sscratch
    li      t0, CPU_STACK_OFF + CPU_STACK_SIZE
    add     sp, sp, t0

    .option push
    .option norelax
    la  gp, __global_pointer$
    .option pop

    /* clear bss if hart 0 */
    LD_SYM  t0, CPU_MASTER
    bne     a0, t0, wait_for_bsp_2
    la      a3, _bss_start
    la      a4, _bss_end
    call    boot_clear

    fence   w, w
    la  t0, _barrier
    li  t1, 2
    STORE  t1, 0(t0)

wait_for_bsp_2:
    /* wait for hart 0 to finish clearing bss */
    la  t0, _barrier
    li  t1, 2
1:
    LOAD  t2, 0(t0)
    blt t2, t1, 1b

    j   init

	/* This point should never be reached */
	j	.	

.global boot_clear
boot_clear:
    sb  zero, 0(a3)
    add a3, a3, 1
    blt a3, a4, boot_clear
    ret

.section ".data"
.align 4
.global _load_addr

_load_addr:
    .word 0xDEADBEEF
    .word 0xDEADBEEF
