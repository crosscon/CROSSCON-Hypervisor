/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <platform.h>

struct platform platform = {

    .cpu_num = 1,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x30000,
            .size = 0x50000,
        },
    },

    .arch = {
        .irqc.plic.base = 0xc000000,
    },
};
