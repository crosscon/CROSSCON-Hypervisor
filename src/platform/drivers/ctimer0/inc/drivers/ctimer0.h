/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef CTIMER0_H
#define CTIMER0_H

#include <crossconhyp.h>

#define CTIMER0_IRQ_N  (14U)
#define CTIMER0_INT_ID (CTIMER0_IRQ_N + 16U)

#ifndef CTIMER0_USE_SECURE_ALIAS
#define CTIMER0_USE_SECURE_ALIAS 1
#endif

#define CTIMER0_BASE_NS (0x40008000UL)
#define CTIMER0_BASE_S  (0x50008000UL)

typedef void (*ctimer0_callback_t)(void* arg);

struct ctimer0_config {
    uint32_t timer_clk_hz;
    uint32_t period_hz;
    uint32_t irq_priority;
    ctimer0_callback_t callback;
    void* callback_arg;
};

int ctimer0_init(const struct ctimer0_config* cfg);
void ctimer0_start(void);
void ctimer0_stop(void);
void ctimer0_set_period_ticks(uint32_t ticks);
void ctimer0_set_period_hz(uint32_t timer_clk_hz, uint32_t period_hz);
void ctimer0_clear_irq(void);
bool ctimer0_is_irq_pending(void);
uint32_t ctimer0_get_count(void);
void ctimer0_irq_handler(void);

#endif /* CTIMER0_H */
