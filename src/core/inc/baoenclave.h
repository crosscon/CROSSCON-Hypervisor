#ifndef BAOENCLAVE_H
#define BAOENCLAVE_H

#include <bao.h>
#include <config.h>
#include <vm.h>

int64_t baoenclave_dynamic_hypercall(uint64_t);
void baoenclave_donate(struct vm* vm, struct config* cfg, uint64_t);
void baoenclave_reclaim(struct vcpu* host, struct vcpu* nvclv);
int baoenclave_handle_abort(unsigned long addr);
int64_t sdsgx_handler_setup(struct vm *vm);

#endif /* BAOENCLAVE_H */
