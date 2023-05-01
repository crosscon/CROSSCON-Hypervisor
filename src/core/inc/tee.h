#ifndef TEE_H_
#define TEE_H_

#include <bao.h>

#define SBI_EXTID_TEE (0x544545)

int64_t tee_handler(uint64_t id);

#endif /* TEE_H_ */
