/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Angelo Ruocco <angeloruocco90@gmail.com>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <platform.h>

struct platform_desc platform = {
    .cpu_num = 4,
    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x80000,
            .size = 0x100000000
        },
    },

    .console = {
        .base = 0xfe215000
    },

    .arch = {
        .gic = {
            .gicd_addr = 0xff841000,
            .gicc_addr = 0xff842000,
            .gich_addr = 0xff844000,
            .gicv_addr = 0xff846000,
            .maintenance_id = 25
        },
    }
};
