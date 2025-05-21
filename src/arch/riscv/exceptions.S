/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <arch/crossconhyp.h>
#include <asm_defs.h>
#include <arch/csrs.h>

#define VCPU_XREG_OFFSET(REG)   (VCPU_REGS_X_OFF + (REGLEN * ((REG)-1)))

.text 

.macro VM_EXIT

    csrrw   x30, sscratch, x30
    STORE   x31, CPU_ARCH_EXTRA_SCRATCH_OFF(x30)
    LOAD    x31, CPU_VCPU_OFF(x30)
    
    STORE   x1,   VCPU_XREG_OFFSET(1)(x31)
    STORE   x2,   VCPU_XREG_OFFSET(2)(x31)
    STORE   x3,   VCPU_XREG_OFFSET(3)(x31)
    STORE   x4,   VCPU_XREG_OFFSET(4)(x31)
    STORE   x5,   VCPU_XREG_OFFSET(5)(x31)
    STORE   x6,   VCPU_XREG_OFFSET(6)(x31)
    STORE   x7,   VCPU_XREG_OFFSET(7)(x31)
    STORE   x8,   VCPU_XREG_OFFSET(8)(x31)
    STORE   x9,   VCPU_XREG_OFFSET(9)(x31)
    STORE   x10, VCPU_XREG_OFFSET(10)(x31)
    STORE   x11, VCPU_XREG_OFFSET(11)(x31)
    STORE   x12, VCPU_XREG_OFFSET(12)(x31)
    STORE   x13, VCPU_XREG_OFFSET(13)(x31)
    STORE   x14, VCPU_XREG_OFFSET(14)(x31)
    STORE   x15, VCPU_XREG_OFFSET(15)(x31)
    STORE   x16, VCPU_XREG_OFFSET(16)(x31)
    STORE   x17, VCPU_XREG_OFFSET(17)(x31)
    STORE   x18, VCPU_XREG_OFFSET(18)(x31)
    STORE   x19, VCPU_XREG_OFFSET(19)(x31)
    STORE   x20, VCPU_XREG_OFFSET(20)(x31)
    STORE   x21, VCPU_XREG_OFFSET(21)(x31)
    STORE   x22, VCPU_XREG_OFFSET(22)(x31)
    STORE   x23, VCPU_XREG_OFFSET(23)(x31)
    STORE   x24, VCPU_XREG_OFFSET(24)(x31)
    STORE   x25, VCPU_XREG_OFFSET(25)(x31)
    STORE   x26, VCPU_XREG_OFFSET(26)(x31)
    STORE   x27, VCPU_XREG_OFFSET(27)(x31)
    STORE   x28, VCPU_XREG_OFFSET(28)(x31)
    STORE   x29, VCPU_XREG_OFFSET(29)(x31)

    LOAD    t0, CPU_ARCH_EXTRA_SCRATCH_OFF(x30)
    STORE   t0, VCPU_XREG_OFFSET(31)(x31)
    
    csrrw   t0, sscratch, x30
    STORE   t0, VCPU_XREG_OFFSET(30)(x31)

    csrr    t0, CSR_HSTATUS
    STORE   t0, VCPU_REGS_HSTATUS_OFF(x31)
    csrr    t0, sstatus
    STORE   t0, VCPU_REGS_SSTATUS_OFF(x31)
    csrr    t0, sepc
    STORE   t0, VCPU_REGS_SEPC_OFF(x31)

    li      t0, CPU_STACK_OFF + CPU_STACK_SIZE
    add     sp, x30, t0

#ifdef MEM_PROT_MPU
    csrr   t0, sscratch
    li    t0, CPU_AS_ARCH_SPMP_SWITCHMSK_OFF
    add   t0, t0, x30 
    LOAD  t1, (t0)
    csrw   CSR_SPMPSWITCH0, t1

#if RV32
    LOAD  t1, 4(t0)
    csrw  CSR_SPMPSWITCH1, t1
#endif /* RV32 */

#endif /* MEM_PROT_MPU */

    .option push
    .option norelax
    la  gp, __global_pointer$
    .option pop

.endm



.macro VM_ENTRY

    csrr   t0, sscratch
    LOAD   x31, CPU_NEXT_VCPU_OFF(t0)
    beqz   x31, 11f
    LOAD   x30, CPU_VCPU_OFF(t0)
    beq    x30, x31, 1f
    call vcpu_context_switch
1:
    li     x29, VCPU_BLOCKED_COUNT
    add    x29, x29, x31
    LOAD   x29, (x29)
    beqz   x29, 2f
11:
    call   cpu_arch_standby
    j .
2:
#ifdef MEM_PROT_MPU
    li     x1, VCPU_ARCH_SPMP_SWITCHMSK_OFF
    add    x1, x31, x1

#if RV32
    LOAD   x2, 4(x1)
    LOAD   x3, CPU_ARCH_SPMP_LOCKED_OFF+4(t0)
    or     x2, x2, x3
    csrw   CSR_SPMPSWITCH1, x2
#endif /* RV32 */

    LOAD   x2, (x1)
    LOAD   x3, CPU_ARCH_SPMP_LOCKED_OFF(t0)
    or     x2, x2, x3
    csrw   CSR_SPMPSWITCH0, x2

#endif /* MEM_PROT_MPU */

    LOAD   x1, VCPU_REGS_HSTATUS_OFF(x31)
    csrw   CSR_HSTATUS, x1
    LOAD   x1, VCPU_REGS_SSTATUS_OFF(x31)
    csrw   sstatus, x1
    LOAD   x1, VCPU_REGS_SEPC_OFF(x31)
    csrw   sepc, x1

    LOAD   x1,   VCPU_XREG_OFFSET(1)(x31)
    LOAD   x2,   VCPU_XREG_OFFSET(2)(x31)
    LOAD   x3,   VCPU_XREG_OFFSET(3)(x31)
    LOAD   x4,   VCPU_XREG_OFFSET(4)(x31)
    LOAD   x5,   VCPU_XREG_OFFSET(5)(x31)
    LOAD   x6,   VCPU_XREG_OFFSET(6)(x31)
    LOAD   x7,   VCPU_XREG_OFFSET(7)(x31)
    LOAD   x8,   VCPU_XREG_OFFSET(8)(x31)
    LOAD   x9,   VCPU_XREG_OFFSET(9)(x31)
    LOAD   x10, VCPU_XREG_OFFSET(10)(x31)
    LOAD   x11, VCPU_XREG_OFFSET(11)(x31)
    LOAD   x12, VCPU_XREG_OFFSET(12)(x31)
    LOAD   x13, VCPU_XREG_OFFSET(13)(x31)
    LOAD   x14, VCPU_XREG_OFFSET(14)(x31)
    LOAD   x15, VCPU_XREG_OFFSET(15)(x31)
    LOAD   x16, VCPU_XREG_OFFSET(16)(x31)
    LOAD   x17, VCPU_XREG_OFFSET(17)(x31)
    LOAD   x18, VCPU_XREG_OFFSET(18)(x31)
    LOAD   x19, VCPU_XREG_OFFSET(19)(x31)
    LOAD   x20, VCPU_XREG_OFFSET(20)(x31)
    LOAD   x21, VCPU_XREG_OFFSET(21)(x31)
    LOAD   x22, VCPU_XREG_OFFSET(22)(x31)
    LOAD   x23, VCPU_XREG_OFFSET(23)(x31)
    LOAD   x24, VCPU_XREG_OFFSET(24)(x31)
    LOAD   x25, VCPU_XREG_OFFSET(25)(x31)
    LOAD   x26, VCPU_XREG_OFFSET(26)(x31)
    LOAD   x27, VCPU_XREG_OFFSET(27)(x31)
    LOAD   x28, VCPU_XREG_OFFSET(28)(x31)
    LOAD   x29, VCPU_XREG_OFFSET(29)(x31)
    LOAD   x30, VCPU_XREG_OFFSET(30)(x31) 
    LOAD   x31, VCPU_XREG_OFFSET(31)(x31)  

    sret
    j   .
.endm

.balign 0x4
.global _hyp_trap_vector
_hyp_trap_vector:
    VM_EXIT
    csrr    t0, scause
    bltz    t0, 1f
    call    sync_exception_handler
    j       2f
1:
    call    interrupts_arch_handle
2:
.global vcpu_arch_entry
vcpu_arch_entry:
    VM_ENTRY
