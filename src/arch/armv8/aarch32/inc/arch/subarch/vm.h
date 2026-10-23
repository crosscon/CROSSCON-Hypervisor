/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef VM_SUBARCH_H
#define VM_SUBARCH_H

#include <crossconhyp.h>

struct arch_regs {
    uint32_t elr_hyp;
    uint32_t spsr_hyp;
    uint32_t x[15];
};

/*
 * AArch32 exception modes have banked registers which are not saved by the
 * VM_EXIT frame.  They belong to the vCPU, even though there is only one
 * physical copy of each register per pCPU.
 *
 * LR_usr is deliberately absent: in Hyp mode r14 aliases LR_usr.  VM_EXIT
 * saves it in arch_regs.x[14] before issuing a BL and VM_ENTRY restores it
 * immediately before ERET.
 */
struct arch_banked_regs {
    uint32_t sp_usr;

    uint32_t sp_irq;
    uint32_t lr_irq;
    uint32_t spsr_irq;

    uint32_t sp_fiq;
    uint32_t lr_fiq;
    uint32_t spsr_fiq;
    uint32_t r8_fiq;
    uint32_t r9_fiq;
    uint32_t r10_fiq;
    uint32_t r11_fiq;
    uint32_t r12_fiq;

    uint32_t sp_svc;
    uint32_t lr_svc;
    uint32_t spsr_svc;

    uint32_t sp_abt;
    uint32_t lr_abt;
    uint32_t spsr_abt;

    uint32_t sp_und;
    uint32_t lr_und;
    uint32_t spsr_und;
};

/* AArch32 has separate data/instruction fault state registers. */
struct arch_fault_regs {
    uint32_t dfsr;
    uint32_t ifsr;
    uint32_t adfsr;
    uint32_t aifsr;
    uint32_t dfar;
    uint32_t ifar;
};

#endif /* VM_SUBARCH_H */
