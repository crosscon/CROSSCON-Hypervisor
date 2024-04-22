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

#ifndef __CROSSCONHYP_H__
#define __CROSSCONHYP_H__

#include <arch/crossconhyp.h>

#ifndef __ASSEMBLER__

#include <types.h>
#include <printk.h>
#include <util.h>

#define INFO(args, ...) \
    printk("CROSSCONHYP INFO: " args "\n" __VA_OPT__(, ) __VA_ARGS__);

#define WARNING(args, ...) \
    printk("CROSSCONHYP WARNING: " args "\n" __VA_OPT__(, ) __VA_ARGS__);

#define ERROR(args, ...)                                            \
    {                                                               \
        printk("CROSSCONHYP ERROR: " args "\n" __VA_OPT__(, ) __VA_ARGS__); \
        while (1)                                                   \
            ;                                                       \
    }

#endif /* __ASSEMBLER__ */

#endif /* __CROSSCONHYP_H__ */
