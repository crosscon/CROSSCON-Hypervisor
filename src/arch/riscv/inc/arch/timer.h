#ifndef TIMER_ARCH_H
#define TIMER_ARCH_H

#include <crossconhyp.h>
#include <platform.h>
#include <arch/cpu.h>
#include <arch/csrs.h>
#include <arch/sbi.h>

typedef uint64_t timer_value_t;

#define TIMER_ARCH_FREQ() (PLAT_TIMER_FREQ)

irqid_t timer_arch_irq_id(void);

static inline void timer_arch_set(timer_value_t value)
{
    if (CPU_HAS_EXTENSION(CPU_EXT_SSTC)) {
        csrs_stimecmp_write(value);
    } else {
        sbi_set_timer(value); // assumes always success
    }
}

static inline timer_value_t timer_arch_get_count(void)
{
    return csrs_time_read();
}

static inline void timer_arch_disable(void)
{
    timer_arch_set(~0UL);
}

static inline void timer_arch_init(void)
{
    timer_arch_disable();
}

#endif /* TIMER_ARCH_H */
