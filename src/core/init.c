/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <crossconhyp.h>

#include <cpu.h>
#include <mem.h>
#include <interrupts.h>
#include <console.h>
#include <printk.h>
#include <platform.h>
#include <vmm.h>

void init(cpuid_t cpu_id, paddr_t load_addr, paddr_t config_addr)
{
    /**
     * These initializations must be executed first and in fixed order.
     */

    cpu_init(cpu_id, load_addr);
    mem_init(load_addr, config_addr);

    /* -------------------------------------------------------------- */

    if (cpu.id == CPU_MASTER) {
        console_init();
        printk("\n\r"
               "   _____ _____   ____   _____ _____  _____ ____  _   _ \n\r"
               "  / ____|  __ \\ / __ \\ / ____/ ____|/ ____/ __ \\| \\ | |\n\r"
               " | |    | |__) | |  | | (___| (___ | |   | |  | |  \\| |\n\r"
               " | |    |  _  /| |  | |\\___ \\\\___ \\| |   | |  | | . ` |\n\r");
        printk(" | |____| | \\ \\| |__| |____) |___) | |___| |__| | |\\  |\n\r"
               "  \\_____|_|  \\_\\\\____/|_____/_____/ \\_____\\____/|_| \\_|\n\r");
        printk("  _    _                             _\n\r"
               " | |  | |                           (_)                \n\r"
               " | |__| |_   _ _ __    ___ _ ____   ___ ___  ___  _ __  \n\r");
        printk(" |  __  | | | | '_ \\ / _ \\ '__\\ \\ / / / __|/ _ \\| '__| \n\r"
               " | |  | | |_| | |_) |  __/ |   \\ V /| \\__ \\ (_) | |    \n\r"
               " |_|  |_|\\__, | .__/ \\___|_|    \\_/ |_|___/\\___/|_|    \n\r"
               "          __/ | |                                      \n\r"
               "         |___/|_| \n\r"
               "\n\r");
    }

    interrupts_init();

    vmm_init();

    /* Should never reach here */
    while (1);
}
