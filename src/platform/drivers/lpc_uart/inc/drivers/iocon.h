/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef LPC_IOCON_H
#define LPC_IOCON_H

#include <bao.h>
#include <drivers/syscon.h>

#define ICON_BASE               (0x50001000UL)

#define IOCON_PIO_FUNC(x)       (x)
#define IOCON_PIO_MODE_INACT    (0x0)
#define IOCON_PIO_SLEW_STANDARD (0x0)
#define IOCON_PIO_INV_DI        (0x0)
#define IOCON_PIO_DIGITAL_EN    (0x100)
#define IOCON_PIO_OPENDRAIN_DI  (0x0)

#ifndef __ASSEMBLER__

struct iocon {
    volatile uint32_t pio[2][32];
};

static struct iocon* const iocon_s = (struct iocon*)ICON_BASE;

typedef enum { port0, port1 } port_t;

typedef enum {
    pin00,
    pin01,
    pin02,
    pin03,
    pin04,
    pin05,
    pin06,
    pin07,
    pin08,
    pin09,
    pin10,
    pin11,
    pin12,
    pin13,
    pin14,
    pin15,
    pin16,
    pin17,
    pin18,
    pin19,
    pin20,
    pin21,
    pin22,
    pin23,
    pin24,
    pin25,
    pin26,
    pin27,
    pin28,
    pin29,
    pin30,
    pin31,
} pin_t;

static inline void iocon_init(void)
{
    // Enable clock
    syscon_enable_clk(AHBCLKCTRL0, SYSCON_AHBCLKTRL0_ICON);
}

static inline void iocon_pin_cfg(uint8_t port, uint8_t pin, uint32_t cfg)
{
    iocon_s->pio[port][pin] = cfg;
}

#endif

#endif /* LPC_IOCON_H */
