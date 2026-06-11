#ifndef TEE_H_
#define TEE_H_

#include <crossconhyp.h>
#include <vm.h>

int32_t virtteev_handler_setup(struct vm* vm);

/* PRIVATE */
long virtteev_handler(struct vcpu* vcpu, uint32_t fid);

#endif /* TEE_H_ */