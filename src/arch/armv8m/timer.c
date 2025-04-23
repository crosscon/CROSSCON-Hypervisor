
#include <timer.h>
#include <interrupts.h>

irqid_t timer_arch_irq_id(void)
{
    return (irqid_t)SYSTICK_INT_N;
}
