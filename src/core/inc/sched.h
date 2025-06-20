#ifndef SCHED_H
#define SCHED_H

#include <crossconhyp.h>
#include <vm.h>

extern unsigned long long time_slice;

void sched_start(void);
void sched_yield(void);
void sched_init(void);

long int sched_lock_hypercall(struct vcpu* vcpu);

#endif /* SCHED_H */
