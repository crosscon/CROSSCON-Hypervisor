#include <drivers/sbi_uart.h>
#include <arch/sbi.h>

bool uart_init(crossconhyp_uart_t* uart)
{
    return true;
}
void uart_enable(crossconhyp_uart_t* uart) {}

void uart_puts(crossconhyp_uart_t* uart, char const* const str)
{
    char const* ptr = str;
    while (*ptr) sbi_console_putchar(*ptr++);
}
