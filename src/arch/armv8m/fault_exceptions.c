/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <arch/fault_exceptions.h>

void fault_exception_handler(void)
{
    // __asm volatile (
    //     "TST lr, #4             \n"
    //     "ITE EQ                 \n"
    //     "MRSEQ r0, MSP          \n"
    //     "MRSNE r0, PSP          \n"
    //     "B hard_fault_handler_c \n"
    // );
    while (1) { };
}

// void hard_fault_handler_c(uint32_t *stacked_regs) {
//     console_printk("R0  = 0x%08lx\n", stacked_regs[0]);
//     console_printk("R1  = 0x%08lx\n", stacked_regs[1]);
//     console_printk("R2  = 0x%08lx\n", stacked_regs[2]);
//     console_printk("R3  = 0x%08lx\n", stacked_regs[3]);
//     console_printk("R12 = 0x%08lx\n", stacked_regs[4]);
//     console_printk("LR  = 0x%08lx\n", stacked_regs[5]);
//     console_printk("PC  = 0x%08lx\n", stacked_regs[6]); // Instrução que causou o fault
//     console_printk("xPSR= 0x%08lx\n", stacked_regs[7]);

//     // Opcional: ver CFSR
//     uint32_t cfsr = *((volatile uint32_t*)0xE000ED28);
//     console_printk("CFSR = 0x%08lx\n", cfsr);
//     while (1); // Para inspeção em debug
// }