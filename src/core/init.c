/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>

#include <cpu.h>
#include <mem.h>
#include <interrupts.h>
#include <console.h>
#include <printk.h>
#include <platform.h>
#include <sched.h>
#include <timer.h>
#include <vmm.h>

void init(cpuid_t cpu_id)
{
    /**
     * These initializations must be executed first and in fixed order.
     */
    if (cpu_id == CPU_MASTER) {
        /* for (volatile int x = 1; x;); */
    }
    cpu_init(cpu_id);
    mem_init();

    /* -------------------------------------------------------------- */

    platform_init();

    console_init();

    if (cpu_is_master()) {
        console_printk("   _____ _____   ____   _____ _____  _____ ____  _   _ \n");
        console_printk("  / ____|  __ \\ / __ \\ / ____/ ____|/ ____/ __ \\| \\ | |\n");
        console_printk(" | |    | |__) | |  | | (___| (___ | |   | |  | |  \\| |\n");
        console_printk(" | |    |  _  /| |  | |\\___ \\\\___ \\| |   | |  | | . ` |\n");
        console_printk(" | |____| | \\ \\| |__| |____) |___) | |___| |__| | |\\  |\n");
        console_printk("  \\_____|_|  \\_\\\\____/|_____/_____/ \\_____\\____/|_| \\_|\n");
        console_printk("  _    _                             _\n");
        console_printk(" | |  | |                           (_)                \n");
        console_printk(" | |__| |_   _ _ __   ___ _ ____   ___ ___  ___  _ __  \n");
        console_printk(" |  __  | | | | '_ \\ / _ \\ '__\\ \\ / / / __|/ _ \\| '__| \n");
        console_printk(" | |  | | |_| | |_) |  __/ |   \\ V /| \\__ \\ (_) | |    \n");
        console_printk(" |_|  |_|\\__, | .__/ \\___|_|    \\_/ |_|___/\\___/|_|    \n");
        console_printk("          __/ | |                                      \n");
        console_printk("         |___/|_| %s\n", __TIME__);
        console_printk("Bao version: 2.0.0\n");
        console_printk("\n");
    }

    interrupts_init();

    timer_init();

    vmm_init();

    sched_start();

    vcpu_arch_entry();

    /* Should never reach here */
    while (1) { }
}
