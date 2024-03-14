#ifndef SDGPOS_RISCV_H_
#define SDGPOS_RISCV_H_

#include <bao.h>
#include <vm.h>

// NOT HERE
#define SBI_EXTID_TEE (0x544545)

bool sdgpos_arch_setup(struct vm *vm);

#endif


