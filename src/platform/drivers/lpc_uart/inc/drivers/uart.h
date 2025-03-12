/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef LPC_UART_H
#define LPC_UART_H

#include <bao.h>
#include <drivers/syscon.h>

#define FC0_BASE      0x50086000UL

#define UART_BAUDRATE 115200U
#if (UART_BAUDRATE == 115200)
#define UART_BRG 7
#define UART_OSR 12
#else
#error "This needs to be calculated"
#endif
#define UART_CLK                  12000000U
#define UART_NOPARITY             0U
#define UART_ONESTOPBIT           0U
#define UART_DATALEN_8B           1U
#define UART_TXFIFO               0U

#define FC_PSELID_LOCK            (1 << 3)

#define FCUART_FIFOCFG_ENABLETX   (1 << 0)
#define FCUART_FIFOCFG_EMPTYTX    (1 << 16)

#define FCUART_FIFOTRIG_TXLVLENA  (1 << 0)
#define FCUART_FIFOTRIG_TXLVL_POS (8)
#define FCUART_FIFOTRIG_TXLVL_MSK (0xF00UL)

#define FCUART_CFG_ENABLE         (1 << 0)

#define FCUART_CFG_DATALEN(x)     (((x) << 2) & 0xC)
#define FCUART_CFG_PARITY(x)      (((x) << 4) & 0x30)
#define FCUART_CFG_STOPLEN(x)     (((x) << 6) & 0x40)
#define FCUART_CFG_MODE32K(x)     (((x) << 7) & 0x80)
#define FCUART_CFG_LINMODE(x)     (((x) << 8) & 0x100)
#define FCUART_CFG_CTSEN(x)       (((x) << 9) & 0x200)
#define FCUART_CFG_SYNCEN(x)      (((x) << 11) & 0x800)
#define FCUART_CFG_CLKPOL(x)      (((x) << 12) & 0x1000)
#define FCUART_CFG_SYNCMST(x)     (((x) << 14) & 0x4000)
#define FCUART_CFG_LOOP(x)        (((x) << 15) & 0x8000)
#define FCUART_CFG_OETA(x)        (((x) << 18) & 0x40000)
#define FCUART_CFG_AUTOADDR(x)    (((x) << 19) & 0x80000)
#define FCUART_CFG_OESEL(x)       (((x) << 20) & 0x100000)
#define FCUART_CFG_OEPOL(x)       (((x) << 21) & 0x200000)
#define FCUART_CFG_RXPOL(x)       (((x) << 22) & 0x400000)
#define FCUART_CFG_TXPOL(x)       (((x) << 23) & 0x800000)

#define FCUART_CTL_CC             (1UL << 8UL)

#define FCUART_FIFOSTAT_TXNOTFULL (1 << 5)

#define FCUART_STAT_TXIDLE        (1 << 3)

#ifndef __ASSEMBLER__

struct fc_cfg {
    uint32_t reserved_0[1022];
    volatile uint32_t pselid;
    volatile uint32_t pid;
};

struct fc_uart {
    volatile uint32_t cfg;
    volatile uint32_t ctl;
    volatile uint32_t stat;
    volatile uint32_t intenset;
    volatile uint32_t intenclr;
    uint32_t reserved1[3];
    volatile uint32_t brg;
    volatile const uint32_t intstat;
    volatile uint32_t osr;
    volatile uint32_t addr;
    uint32_t reserved2[884];
    volatile uint32_t fifocfg;
    volatile uint32_t fifostat;
    volatile uint32_t fifotrig;
    uint32_t reserved3;
    volatile uint32_t fifointenset;
    volatile uint32_t fifointenclr;
    volatile const uint32_t fifointstat;
    uint32_t reserved4;
    volatile uint32_t fifowr;
    uint32_t reserved5[3];
    volatile uint32_t fiford;
    uint32_t reserved6[3];
    volatile uint32_t fifordnopop;
    uint32_t reserved7;
    volatile uint32_t fifosize;
    uint32_t reserved8[108];
    volatile uint32_t id;
};

typedef volatile struct fc_uart fc_uart0_t;
typedef fc_uart0_t bao_uart_t;

static struct fc_cfg* const fc0_cfg_s = (struct fc_cfg*)FC0_BASE;

void uart_init(volatile fc_uart0_t* uart);
void uart_putc(volatile fc_uart0_t* uart, int8_t str);
void uart_puts(volatile fc_uart0_t* uart, int8_t const* str);

static inline void uart_enable(volatile fc_uart0_t* uart)
{
    uart->cfg |= FCUART_CFG_ENABLE;
}

#endif

#endif /* LPC_UART_H */
