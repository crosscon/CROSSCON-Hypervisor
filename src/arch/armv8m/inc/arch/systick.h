#ifndef SYSTICK_ARCH_H
#define SYSTICK_ARCH_H

#include <crossconhyp.h>
#include <platform.h>
#include <arch/cpu.h>
#include <arch/sysregs.h>

static inline uint32_t systick_get_cvr(struct systick* st)
{
    return st->cvr;
}

static inline void systick_clear_actbit(struct systick* st)
{
    if (st == systick_ns) {
        scb_ns->shcsr &= ~SCB_SHCSR_SYSTICKACT;
    } else {
        scb_s->shcsr &= ~SCB_SHCSR_SYSTICKACT;
    }
}

static inline void systick_clear_pendbit(struct systick* st)
{
    if (st == systick_ns) {
        scb_ns->icsr |= SCB_ICSR_PENDSTCLR;
    } else {
        scb_s->icsr |= SCB_ICSR_PENDSTCLR;
    }
}

static inline void systick_set_rvr(struct systick* st, uint32_t val)
{
    st->rvr = val & (0xFFFFFFUL);
}

static inline void systick_set_cvr(struct systick* st, uint32_t val)
{
    // Note: this is a write-to-clear register
    st->cvr = val & (0xFFFFFFUL);
}

static inline uint32_t systick_get_rvr(struct systick* st)
{
    return st->rvr;
}

static inline void systick_set_csr(struct systick* st, uint32_t val)
{
    st->csr = val;
}

static inline uint32_t systick_get_csr(struct systick* st)
{
    return st->csr;
}

#endif /* SYSTICK_ARCH_H */
