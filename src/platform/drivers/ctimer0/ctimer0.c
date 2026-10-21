/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <drivers/ctimer0.h>
#include <drivers/syscon.h>

#define CTIMER_IR_MR0INT      (1UL << 0)
#define CTIMER_TCR_ENABLE     (1UL << 0)
#define CTIMER_TCR_RESET      (1UL << 1)
#define CTIMER_MCR_MR0I       (1UL << 0)
#define CTIMER_MCR_MR0R       (1UL << 1)
#define CTIMER_CTCR_TIMERMODE (0UL)

#if CTIMER0_USE_SECURE_ALIAS
#define CTIMER0_BASE CTIMER0_BASE_S
#else
#define CTIMER0_BASE CTIMER0_BASE_NS
#endif

struct ctimer_regs {
    volatile uint32_t ir;
    volatile uint32_t tcr;
    volatile uint32_t tc;
    volatile uint32_t pr;
    volatile uint32_t pc;
    volatile uint32_t mcr;
    volatile uint32_t mr[4];
    volatile uint32_t ccr;
    volatile uint32_t cr[4];
    volatile uint32_t emr;
    uint32_t reserved0[12];
    volatile uint32_t ctcr;
    volatile uint32_t pwmc;
};

static struct ctimer_regs* const ctimer0 = (struct ctimer_regs*)CTIMER0_BASE;

static ctimer0_callback_t ctimer0_callback;
static void* ctimer0_callback_arg;

static void ctimer0_clk_init(void)
{
    syscon_clk_src(CTIMERCLKSEL0, SYSCON_FCCLKSEL_FRO12M);
    syscon_enable_clk(AHBCLKCTRL1, SYSCON_AHBCLKTRL1_CTIMER0);
    syscon_rst_sign(CTIMER0_RST);
}

void ctimer0_stop(void)
{
    ctimer0->tcr &= ~CTIMER_TCR_ENABLE;
}

void ctimer0_start(void)
{
    ctimer0_clear_irq();
    ctimer0->tcr |= CTIMER_TCR_ENABLE;
}

void ctimer0_set_period_ticks(uint32_t ticks)
{
    if (ticks == 0U) {
        ticks = 1U;
    }

    ctimer0_stop();
    ctimer0->tcr |= CTIMER_TCR_RESET;
    ctimer0->pr = 0;
    ctimer0->pc = 0;
    ctimer0->tc = 0;
    ctimer0->mr[0] = ticks - 1U;
    ctimer0->mcr = CTIMER_MCR_MR0I | CTIMER_MCR_MR0R;
    ctimer0->tcr &= ~CTIMER_TCR_RESET;
    ctimer0_clear_irq();
}

void ctimer0_set_period_hz(uint32_t timer_clk_hz, uint32_t period_hz)
{
    uint32_t ticks = 1U;

    if ((timer_clk_hz != 0U) && (period_hz != 0U)) {
        ticks = timer_clk_hz / period_hz;
        if (ticks == 0U) {
            ticks = 1U;
        }
    }

    ctimer0_set_period_ticks(ticks);
}

void ctimer0_clear_irq(void)
{
    ctimer0->ir = CTIMER_IR_MR0INT;
}

bool ctimer0_is_irq_pending(void)
{
    return (ctimer0->ir & CTIMER_IR_MR0INT) != 0U;
}

uint32_t ctimer0_get_count(void)
{
    return ctimer0->tc;
}

int ctimer0_init(const struct ctimer0_config* cfg)
{
    if ((cfg == NULL) || (cfg->timer_clk_hz == 0U) || (cfg->period_hz == 0U)) {
        return -1;
    }

    ctimer0_callback = cfg->callback;
    ctimer0_callback_arg = cfg->callback_arg;

    ctimer0_clk_init();
    ctimer0_stop();
    ctimer0->ctcr = CTIMER_CTCR_TIMERMODE;
    ctimer0_set_period_hz(cfg->timer_clk_hz, cfg->period_hz);

    return 0;
}

void ctimer0_irq_handler(void)
{
    ctimer0_clear_irq();

    if (ctimer0_callback != NULL) {
        ctimer0_callback(ctimer0_callback_arg);
    }
}
