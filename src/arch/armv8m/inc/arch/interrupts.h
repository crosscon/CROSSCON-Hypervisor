/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_INTERRUPTS_H__
#define __ARCH_INTERRUPTS_H__

#include <crossconhyp.h>
#include <plat/platform.h>

#define ARCH_MAX_INTERRUPTS    496
#define MAX_INTERRUPTS         PLAT_MAX_INTERRUPTS // TODO:ARMV8M - Should this plat or arch-defined?
#define MAX_GUEST_INTERRUPTS   (MAX_INTERRUPTS)

#define MAX_INTERRUPT_LINES    MAX_INTERRUPTS
#define MAX_INTERRUPT_HANDLERS MAX_INTERRUPTS

#define SYSTICK_INT_N          15

/* TODO platform dependent */
#define IPI_CPU_MSG            1304 /* TODO this is the first GPSR in TC49 */

#ifndef __ASSEMBLER__

struct vcpu;

void interrupts_arch_handle(void);
struct vcpu* get_vcpu_to_interrupt(void);
void interrupts_arch_pendsv_handle(void);
bool interrupts_arch_vm_irq_enter(struct vcpu* owner, irqid_t int_id);
bool interrupts_arch_vm_irq_resume(void);
void nvic_init_ipi(void);

#endif

#endif /* __ARCH_INTERRUPTS_H__ */
