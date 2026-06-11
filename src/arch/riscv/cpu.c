/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <cpu.h>
#include <arch/sbi.h>
#include <platform.h>

cpuid_t CPU_MASTER __attribute__((section(".datanocopy")));

static void cpu_arch_reset_fpu(void)
{
    cpu()->arch.fpu_state = FPU_INIT;
    csrs_sstatus_clear(SSTATUS_FS_MSK);
    csrs_sstatus_set(SSTATUS_FS_INITIAL);

    __asm__ volatile("fmv.w.x f0,  zero \n\t"
                     "fmv.w.x f1,  zero \n\t"
                     "fmv.w.x f2,  zero \n\t"
                     "fmv.w.x f3,  zero \n\t"
                     "fmv.w.x f4,  zero \n\t"
                     "fmv.w.x f5,  zero \n\t"
                     "fmv.w.x f6,  zero \n\t"
                     "fmv.w.x f7,  zero \n\t"
                     "fmv.w.x f8,  zero \n\t"
                     "fmv.w.x f9,  zero \n\t"
                     "fmv.w.x f10, zero \n\t"
                     "fmv.w.x f11, zero \n\t"
                     "fmv.w.x f12, zero \n\t"
                     "fmv.w.x f13, zero \n\t"
                     "fmv.w.x f14, zero \n\t"
                     "fmv.w.x f15, zero \n\t"
                     "fmv.w.x f16, zero \n\t"
                     "fmv.w.x f17, zero \n\t"
                     "fmv.w.x f18, zero \n\t"
                     "fmv.w.x f19, zero \n\t"
                     "fmv.w.x f20, zero \n\t"
                     "fmv.w.x f21, zero \n\t"
                     "fmv.w.x f22, zero \n\t"
                     "fmv.w.x f23, zero \n\t"
                     "fmv.w.x f24, zero \n\t"
                     "fmv.w.x f25, zero \n\t"
                     "fmv.w.x f26, zero \n\t"
                     "fmv.w.x f27, zero \n\t"
                     "fmv.w.x f28, zero \n\t"
                     "fmv.w.x f29, zero \n\t"
                     "fmv.w.x f30, zero \n\t"
                     "fmv.w.x f31, zero \n\t");
}

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
    cpu_arch_reset_fpu();
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

void cpu_arch_powerdown(void)
{
    __asm__ volatile("wfi\n\t" ::: "memory");
    __asm__ volatile("mv sp, %0\n\r"
                     "j cpu_powerdown_wakeup\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));
    ERROR("returned from powerdown wake up");
}

void cpu_arch_park(void)
{
    __asm__ volatile("mv sp, %0\n\r" ::"r"(&cpu()->stack[STACK_SIZE]));

    csrs_sstatus_set(SSTATUS_SIE_BIT);

    while (true) {
        __asm__ volatile("wfi");
    }
}
