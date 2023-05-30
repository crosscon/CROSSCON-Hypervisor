#ifndef BAOENCLAVE_H
#define BAOENCLAVE_H

#include <bao.h>

int64_t baoenclave_dynamic_hypercall(uint64_t, uint64_t, uint64_t, uint64_t);
void alloc_baoenclave(void*, uint64_t);
int baoenclave_handle_abort(unsigned long addr);

#endif /* BAOENCLAVE_H */
