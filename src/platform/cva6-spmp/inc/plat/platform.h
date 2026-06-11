#ifndef __PLAT_PLATFORM_H__
#define __PLAT_PLATFORM_H__

#define UART8250_REG_WIDTH   (4)
#define UART8250_PAGE_OFFSET (0x40)

#define PLAT_TIMER_FREQ (25000000ull)   // 25 MHz

#define CPU_EXT_SSTC 1

#include <drivers/8250_uart.h>

#endif
