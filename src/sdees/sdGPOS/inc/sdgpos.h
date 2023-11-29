#ifndef SDGPOS_H_
#define SDGPOS_H_

#include <bao.h>
#include <vm.h>

#define SBI_EXTID_TEE (0x544545)

int64_t sdgpos_handler_setup(struct vm *vm);

#endif /* TEE_H_ */
