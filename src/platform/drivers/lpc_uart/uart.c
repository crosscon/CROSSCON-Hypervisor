/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <crossconhyp.h>
#include <drivers/uart.h>
#include <drivers/iocon.h>
#include <drivers/syscon.h>

typedef enum {
    NONE,
    USART,
    SPI,
    I2C,
    I2S_TX,
    I2S_RX,
} fc_cfg_t;

static void fc0_clk_init(void)
{
    // Configure FR0 12MHz to FLEXCOMM0
    syscon_clk_src(FCCLKSEL0, SYSCON_FCCLKSEL_FRO12M);

    // Enable clock
    syscon_enable_clk(AHBCLKCTRL1, SYSCON_AHBCLKTRL1_FC0);
}

static void fc0_init(volatile fc_uart0_t* uart)
{
    // Configure Flexcomm0 clock
    fc0_clk_init();

    // Reset peripheral
    syscon_rst_sign(FC0_RST);

    // Set and lock Flexcomm0 to UART
    fc0_cfg_s->pselid = USART | FC_PSELID_LOCK;

    // Empty and enable txFIFO
    uart->fifocfg |= FCUART_FIFOCFG_EMPTYTX | FCUART_FIFOCFG_ENABLETX;
    uart->fifotrig &= ~FCUART_FIFOTRIG_TXLVL_MSK;
    uart->fifotrig |= FCUART_FIFOTRIG_TXLVLENA;

    // Setup configuration
    uart->cfg |= FCUART_CFG_PARITY(UART_NOPARITY) | FCUART_CFG_STOPLEN(UART_ONESTOPBIT) |
        FCUART_CFG_DATALEN(UART_DATALEN_8B) | FCUART_CFG_LOOP(0) | FCUART_CFG_SYNCEN(0) |
        FCUART_CFG_SYNCMST(0) | FCUART_CFG_CLKPOL(0) | FCUART_CFG_MODE32K(0) | FCUART_CFG_CTSEN(0);

    // Setup baudrate
    uart->brg = UART_BRG;
    uart->osr = UART_OSR;

    // Disable SCLK
    uart->ctl &= ~(FCUART_CTL_CC);
}

void uart_init(volatile fc_uart0_t* uart)
{
    // Configure USART pins
    iocon_init();

    // Configure RXD pin
    iocon_pin_cfg(port0, pin29,
        IOCON_PIO_FUNC(1) | IOCON_PIO_MODE_INACT | IOCON_PIO_SLEW_STANDARD | IOCON_PIO_INV_DI |
            IOCON_PIO_DIGITAL_EN | IOCON_PIO_OPENDRAIN_DI);

    // Configure TXD pin
    iocon_pin_cfg(port0, pin30,
        IOCON_PIO_FUNC(1) | IOCON_PIO_MODE_INACT | IOCON_PIO_SLEW_STANDARD | IOCON_PIO_INV_DI |
            IOCON_PIO_DIGITAL_EN | IOCON_PIO_OPENDRAIN_DI);

    // Initialize the flexcomm dev with uart configurations
    fc0_init(uart);
}

void uart_putc(volatile fc_uart0_t* uart, int8_t c)
{
    while (0U == (uart->fifostat & FCUART_FIFOSTAT_TXNOTFULL)) { }

    uart->fifowr = (uint32_t)c;

    while (0U == (uart->stat & FCUART_STAT_TXIDLE)) { }
    return;
}

void uart_puts(volatile fc_uart0_t* uart, int8_t const* str)
{
    UNUSED_ARG(uart);

    while (*str) {
        uart_putc(uart, *str++);
    }
}
