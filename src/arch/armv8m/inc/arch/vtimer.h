#ifndef VTIMER_H
#define VTIMER_H

#include <crossconhyp.h>

#define VTIMER_IRQ_ID (15)

struct vtimer {
    uint32_t csr;
    uint32_t rvr;
    uint32_t cvr;
    // irq_id_t irq;
};

struct vtimer;

void vtimer_init(struct vtimer* vtimer);
void vtimer_reset(struct vtimer* vtimer);
void vtimer_save_state(struct vtimer* vtimer);
void vtimer_restore_state(struct vtimer* vtimer);

#endif /* VTIMER_H */
