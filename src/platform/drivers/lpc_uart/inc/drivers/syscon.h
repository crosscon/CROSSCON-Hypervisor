/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef LPC_SYSCON_H
#define LPC_SYSCON_H

#include <crossconhyp.h>

#define SYSCON_BASE              (0x50000000UL)

#define SYSCON_AHBCLKTRL0_ICON   (1 << 13)
#define SYSCON_AHBCLKTRL1_FC0    (1 << 11)
#define SYSCON_AHBCLKTRL1_FC1    (1 << 12)
#define SYSCON_AHBCLKTRL1_FC2    (1 << 13)
#define SYSCON_AHBCLKTRL1_FC3    (1 << 14)
#define SYSCON_AHBCLKTRL1_FC4    (1 << 15)
#define SYSCON_AHBCLKTRL1_FC5    (1 << 16)
#define SYSCON_AHBCLKTRL1_FC6    (1 << 17)
#define SYSCON_AHBCLKTRL1_FC7    (1 << 18)

#define SYSCON_FCCLKSEL_MAINCLK  (0x0)
#define SYSCON_FCCLKSEL_PLL0DIV  (0x1)
#define SYSCON_FCCLKSEL_FRO12M   (0x2)
#define SYSCON_FCCLKSEL_FROHFDIV (0x3)
#define SYSCON_FCCLKSEL_FRO1M    (0x4)
#define SYSCON_FCCLKSEL_MCLK     (0x5)
#define SYSCON_FCCLKSEL_OSC32K   (0x6)
#define SYSCON_FCCLKSEL_NONE     (0x7)

#ifndef __ASSEMBLER__

/* SYSCON */
struct syscon {
    volatile uint32_t memoryremap;  // 0x000
    uint32_t reserved_0[3];
    volatile uint32_t ahbmatprio;   // 0x010
    uint32_t reserved_1[9];
    volatile uint32_t cpu0stckcal;  // 0x38
    volatile uint32_t cpu0nstckcal; // 0x3C
    volatile uint32_t cpu1stckcal;  // 0x40
    uint32_t reserved_2;
    volatile uint32_t nmisrc;       // 0x48
    uint32_t reserved_3[45];
    union {
        struct {
            volatile uint32_t presetctrl0; // 0x100
            volatile uint32_t presetctrl1; // 0x104
            volatile uint32_t presetctrl2; // 0x108
        } presetctrl;
        volatile uint32_t presetctrlx[3];
    };
    uint32_t reserved_4[5];
    volatile uint32_t presetctrlset[3]; // 0x120
    uint32_t reserved_5[5];
    volatile uint32_t presetctrlclr[3]; // 0x140
    uint32_t reserved_6[5];
    uint32_t swr_reset;                 // 0x160
    uint32_t reserved_7[39];
    union {
        struct {
            volatile uint32_t ahbclkctrl0; // 0x200
            volatile uint32_t ahbclkctrl1; // 0x204
            volatile uint32_t ahbclkctrl2; // 0x208
        } ahbclkctrl;
        volatile uint32_t ahbclkctrlx[3];
    };
    uint32_t reserved_8[5];
    volatile uint32_t ahbclkctrlset[3]; // 0x220
    uint32_t reserved_9[5];
    volatile uint32_t ahbclkctrlclr[3]; // 0x240
    uint32_t reserved_10[5];
    union {
        struct {
            volatile uint32_t systickclksel0; // 0x260
            volatile uint32_t systickclksel1; // 0x264
        } systickclksel;
        volatile uint32_t systickclkselx[2];
    };
    volatile uint32_t traceclksel;           // 0x268
    union {
        struct {
            volatile uint32_t ctimerclksel0; // 0x26C
            volatile uint32_t ctimerclksel1;
            volatile uint32_t ctimerclksel2;
            volatile uint32_t ctimerclksel3;
            volatile uint32_t ctimerclksel4;
        } ctimerclksel;
        volatile uint32_t ctimerclkselx[5];
    };
    volatile uint32_t mainclksela;
    volatile uint32_t mainclkselb;
    volatile uint32_t clkoutsel;
    uint32_t reserved_11;
    volatile uint32_t pll0clksel;
    volatile uint32_t pll1clksel;
    uint32_t reserved_12[3];
    volatile uint32_t adcclksel;
    volatile uint32_t usb0clksel;
    uint32_t reserved_13;
    union {
        struct {
            volatile uint32_t fcclksel0;
            volatile uint32_t fcclksel1;
            volatile uint32_t fcclksel2;
            volatile uint32_t fcclksel3;
            volatile uint32_t fcclksel4;
            volatile uint32_t fcclksel5;
            volatile uint32_t fcclksel6;
            volatile uint32_t fcclksel7;
        } fcclksel;
        volatile uint32_t fcclkselx[8];
    };
    volatile uint32_t hslspiclksel;
    uint32_t reserved_14[3];
    volatile uint32_t mclkclksel;
    uint32_t reserved_15[3];
    volatile uint32_t sctclksel;
    uint32_t reserved_16;
    volatile uint32_t sdioclksel;
    uint32_t reserved_17;
    volatile uint32_t systickclkdiv0;
    volatile uint32_t systickclkdiv1;
    volatile uint32_t traceclkdiv;
    uint32_t reserved_18[5];
    union {
        struct {
            volatile uint32_t flexfrg0ctrl;
            volatile uint32_t flexfrg1ctrl;
            volatile uint32_t flexfrg2ctrl;
            volatile uint32_t flexfrg3ctrl;
            volatile uint32_t flexfrg4ctrl;
            volatile uint32_t flexfrg5ctrl;
            volatile uint32_t flexfrg6ctrl;
            volatile uint32_t flexfrg7ctrl;
        } flexfrgctrl;
        volatile uint32_t flexfrgxctrl[8];
    };
    uint32_t reserved_19[4];
    volatile uint32_t ahbclkdiv;
    volatile uint32_t clkoutdiv;
    volatile uint32_t frohfdiv;
    volatile uint32_t wdtclkdiv;
    uint32_t reserved_20;
    volatile uint32_t adcclkdiv;
    volatile uint32_t usb0clkdiv;
    uint32_t reserved_21[4];
    volatile uint32_t mclkdiv;
    uint32_t reserved_22;
    volatile uint32_t sctclkdiv;
    uint32_t reserved_23;
    volatile uint32_t sdioclkdiv;
    uint32_t reserved_24;
    volatile uint32_t pll0clkdiv;
    uint32_t reserved_25[13];
    volatile uint32_t clockgenupdatelockout;
    volatile uint32_t fmccr;
    uint32_t reserved_26[2];
    volatile uint32_t usb0needclkctrl;
    volatile uint32_t usb0needclkstat;
    uint32_t reserved_27[2];
    uint32_t fmcflush;
    volatile uint32_t mclkio;
    volatile uint32_t usb1needclkctrl;
    volatile uint32_t usb1needclkstat;
    uint32_t reserved_28[13];
    volatile uint32_t sdioclkctrl;
    uint32_t reserved_29[63];
    volatile uint32_t pll1ctrl;
    volatile uint32_t pll1stat;
    volatile uint32_t pll1ndec;
    volatile uint32_t pll1mdec;
    volatile uint32_t pll1pdec;
    uint32_t reserved_30[3];
    volatile uint32_t pll0ctrl;
    volatile uint32_t pll0stat;
    volatile uint32_t pll0ndec;
    volatile uint32_t pll0pdec;
    volatile uint32_t pll0sscg0;
    volatile uint32_t pll0sscg1;
    uint32_t reserved_31[91];
    volatile uint32_t funcretentionctrl;
    uint32_t reserved_32[62];
    volatile uint32_t cpuctrl;
    volatile uint32_t cpboot;
    uint32_t reserved_33;
    volatile uint32_t cpstat;
    uint32_t reserved_34[130];
    volatile uint32_t clock_ctrl;
    uint32_t reserved_35[61];
    volatile uint32_t comp_int_ctrl;
    volatile uint32_t comp_int_status;
    uint32_t reserved_36[187];
    volatile uint32_t autoclkgateoverride;
    volatile uint32_t gpiopsync;
    uint32_t reserved_37[101];
    volatile uint32_t debug_lock_en;
    volatile uint32_t debug_features;
    volatile uint32_t debug_features_dp;
    uint32_t reserved_38[4];
    uint32_t key_block;
    volatile uint32_t debug_auth_beacon;
    uint32_t reserved_39[4];
    volatile uint32_t cpucfg;
    uint32_t reserved_40[8];
    volatile uint32_t device_id0;
    volatile uint32_t dieid;
};

static struct syscon* const syscon_s = (struct syscon*)SYSCON_BASE;

typedef enum {
    AHBCLKCTRL0,
    AHBCLKCTRL1,
    AHBCLKCTRL2,
} ahb_t;

typedef enum {
    SYSTICKCLKSEL1 = 0x0,
    SYSTICKCLKSEL2 = 0x4,
    TRACECLKSEL = 0x8,
    CTIMERCLKSEL0 = 0xC,
    CTIMERCLKSEL1 = 0x10,
    CTIMERCLKSEL2 = 0x14,
    CTIMERCLKSEL3 = 0x18,
    CTIMERCLKSEL4 = 0x1C,
    MAINCLKSELA = 0x20,
    MAINCLKSELB = 0x24,
    CLKOUTSEL = 0x28,
    PLL0CLKSEL = 0x30,
    PLL1CLKSEL = 0x34,
    ADCCLKSEL = 0x44,
    USB0CLKSEL = 0x48,
    FCCLKSEL0 = 0x50,
    FCCLKSEL1 = 0x54,
    FCCLKSEL2 = 0x58,
    FCCLKSEL3 = 0x5C,
    FCCLKSEL4 = 0x60,
    FCCLKSEL5 = 0x64,
    FCCLKSEL6 = 0x68,
    FCCLKSEL7 = 0x6C,
    HSLSPICLKSEL = 0x70,
    MCLKCLKSEL = 0x80,
    SCTCLKSEL = 0x90,
    SDIOCLKSEL = 0x98,
} clk_src_t;

typedef enum {
    ROM_RST = 1,
    SRAM1_RST = 3,
    SRAM2_RST = 4,
    SRAM3_RST = 5,
    SRAM4_RST = 6,
    FLASH_RST = 7,
    FMC_RST = 8,
    SPIFI_RST = 10,
    MUX0_RST = 11,
    IOCON_RST = 13,
    GPIO0_RST = 14,
    GPIO1_RST = 15,
    GPIO2_RST = 16,
    GPIO3_RST = 17,
    PINT_RST = 18,
    GINT_RST = 19,
    DMA0_RST = 20,
    CRC_RST = 21,
    WWDT_RST = 22,
    RTC_RST = 23,
    MAILBOX_RST = 26,
    ADC0_RST = 27,
    MRT_RST = 0xFF00 | 0,
    OSTIMER0_RST = 0xFF00 | 1,
    SCT0_RST = 0xFF00 | 2,
    SCTIPU_RST = 0xFF00 | 6,
    UTICK_RST = 0xFF00 | 10,
    FC0_RST = 0xFF00 | 11,
    FC1_RST = 0xFF00 | 12,
    FC2_RST = 0xFF00 | 13,
    FC3_RST = 0xFF00 | 14,
    FC4_RST = 0xFF00 | 15,
    FC5_RST = 0xFF00 | 16,
    FC6_RST = 0xFF00 | 17,
    FC7_RST = 0xFF00 | 18,
    CTIMER2_RST = 0xFF00 | 22,
    USB0D_RST = 0xFF00 | 25,
    CTIMER0_RST = 0xFF00 | 26,
    CTIMER1_RST = 0xFF00 | 27,
    PVT_RST = 0xFF00 | 28,
    EZHA_RST = 0xFF00 | 30,
    EZHB_RST = 0xFF00 | 31,
    DMA1_RST = 0xFF0000 | 1,
    CMP_RST = 0xFF0000 | 2,
    SDIO_RST = 0xFF0000 | 3,
    USB1H_RST = 0xFF0000 | 4,
    USB1D_RST = 0xFF0000 | 5,
    USB1RAM_RST = 0xFF0000 | 6,
    USB1_RST = 0xFF0000 | 7,
    FREQME_RST = 0xFF0000 | 8,
    GPIO4_RST = 0xFF0000 | 9,
    GPIO5_RST = 0xFF0000 | 10,
    AES_RST = 0xFF0000 | 11,
    OTP_RST = 0xFF0000 | 12,
    RNG_RST = 0xFF0000 | 13,
    MUX1_RST = 0xFF0000 | 14,
    USB0HMR_RST = 0xFF0000 | 16,
    USB0HSL_RST = 0xFF0000 | 17,
    HASHCRYPT_RST = 0xFF0000 | 18,
    POWERQUAD_RST = 0xFF0000 | 19,
    PLULUT_RST = 0xFF0000 | 20,
    CTIMER3_RST = 0xFF0000 | 21,
    CTIMER4_RST = 0xFF0000 | 22,
    PUF_RST = 0xFF0000 | 23,
    CASPER_RST = 0xFF0000 | 24,
    CAP0_RST = 0xFF0000 | 25,
    OSTIMER1_RST = 0xFF0000 | 26,
    ANALOGCTL_RST = 0xFF0000 | 27,
    HSLSPI_RST = 0xFF0000 | 28,
    GPIOSEC_RST = 0xFF0000 | 29,
    GPIOSECINT_RST = 0xFF0000 | 30,
} rst_sign_t;

static inline void syscon_enable_clk(ahb_t ahbclk, size_t bit)
{
    syscon_s->ahbclkctrlx[ahbclk] |= bit;
}

static inline void syscon_clk_src(clk_src_t clk_src, size_t clk)
{
    volatile uint32_t* clk_off = &syscon_s->systickclksel.systickclksel0;

    *(clk_off + (clk_src / sizeof(uint32_t))) = clk;
}

static inline void syscon_rst_sign(rst_sign_t rsg_sign)
{
    if (rsg_sign < 0xFF00UL) {
        syscon_s->presetctrlset[0] = (uint32_t)(1 << rsg_sign);
        syscon_s->presetctrlclr[0] = (uint32_t)(1 << rsg_sign);
    } else if (rsg_sign >= 0xFF00UL && rsg_sign < 0xFF0000UL) {
        syscon_s->presetctrlset[1] = (uint32_t)(1 << (rsg_sign & (~0xFF00UL)));
        syscon_s->presetctrlclr[1] = (uint32_t)(1 << (rsg_sign & (~0xFF00UL)));
    } else {
        syscon_s->presetctrlset[2] = (uint32_t)(1 << (rsg_sign & (~0xFF0000UL)));
        syscon_s->presetctrlclr[2] = (uint32_t)(1 << (rsg_sign & (~0xFF0000UL)));
    }
}

#endif

#endif /* LPC_SYSCON_H */
