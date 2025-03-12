/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef PLAT_PLATFORM_H
#define PLAT_PLATFORM_H

#ifndef __ASSEMBLER__
#include <drivers/uart.h>
#endif

#define PLAT_MAX_INTERRUPTS  75

#define PLAT_TIMER_FREQ      12000000UL
#define PLAT_MAX_MPU_REGIONS 8

#endif /* PLAT_PLATFORM_H */
