#ifndef TEE_H_
#define TEE_H_

#include <bao.h>

int64_t tee_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2);

#endif /* TEE_H_ */
