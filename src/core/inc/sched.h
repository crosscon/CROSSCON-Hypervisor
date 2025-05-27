#ifndef SCHED_H
#define SCHED_H

#include <crossconhyp.h>

extern unsigned long long time_slice;

void sched_start(void);
void sched_yield(void);
void sched_init(void);

#endif /* SCHED_H */
