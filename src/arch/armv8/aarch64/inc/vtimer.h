#ifndef VTIMER_H
#define VTIMER_H

#include <crossconhyp.h>

#define VTIMER_IRQ_ID   (27)

struct vtimer {
    unsigned long cntkctl_el1;
    unsigned long cntv_ctl_el0;
    uint64_t cntv_cval_el0;
    uint64_t cntvoff_el2;
    // irq_id_t irq;
};

struct vcpu;

void vtimer_init(struct vtimer* vtimer);
void vtimer_reset(struct vcpu* vcpu);
void vtimer_save_state(struct vcpu* vcpu);
void vtimer_restore_state(struct vcpu* vcpu);
struct timer_event* vtimer_get_event(struct vcpu* vcpu);

#endif /* VTIMER_H */
