/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <cpu.h>
#include <arch/sbi.h>
#include <platform.h>

cpuid_t CPU_MASTER __attribute__((section(".data")));

/* Perform architecture dependent cpu cores initializations */
void cpu_arch_init(cpuid_t cpuid, paddr_t load_addr)
{
    if (cpuid == CPU_MASTER) {
        sbi_init();
        for (size_t hartid = 0; hartid < platform.cpu_num; hartid++) {
            if (hartid == cpuid) {
                continue;
            }
            struct sbiret ret = sbi_hart_start(hartid, load_addr, 0);
            if (ret.error < 0) {
                WARNING("failed to wake up hart %d\n", hartid);
            }
        }
    }
}

void cpu_arch_standby(void)
{
    struct sbiret ret = sbi_hart_suspend(SBI_HSM_SUSPEND_RET_DEFAULT, 0, 0);
    if (ret.error < 0) {
        ERROR("failed to suspend hart %d", cpu()->id);
    }
    __asm__ volatile("mv sp, %0\n\r"
                     "j cpu_standby_wakeup\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));
    ERROR("returned from standby wake up");
}

void cpu_arch_idle()
{
    __asm__ volatile("wfi\n\t" ::: "memory");
    __asm__ volatile("mv sp, %0\n\r"
                     "j cpu_idle_wakeup\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));
    ERROR("returned from idle wake up");
}

void cpu_arch_park()
{
    // reset stack
    __asm__ volatile("mv sp, %0\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));

    csrs_sstatus_set(SSTATUS_SIE_BIT);

    while (true) {
        __asm__ volatile("wfi");
    }
}

void cpu_arch_powerdown(void)
{
    __asm__ volatile("wfi\n\t" ::: "memory");
    __asm__ volatile("mv sp, %0\n\r"
                     "j cpu_powerdown_wakeup\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));
    ERROR("returned from powerdown wake up");
}
