#ifndef SDSGX_H
#define SDSGX_H

#include <crossconhyp.h>
#include <config.h>
#include <vm.h>

void sdsgx_donate(struct vm* vm, struct config* cfg, uint64_t);
void sdsgx_reclaim(struct vcpu* host, struct vcpu* nvclv);
int64_t sdsgx_handler_setup(struct vm *vm);

#endif /* SDSGX_H */
