#ifndef TEE_H_
#define TEE_H_

#include <crossconhyp.h>
#include <vm.h>

int64_t sdtz_handler_setup(struct vm* vm);

/* PRIVATE */
long sdtz_handler(struct vcpu* vcpu, unsigned long fid);

#endif /* TEE_H_ */
