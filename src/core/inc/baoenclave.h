#ifndef BAOENCLAVE_H
#define BAOENCLAVE_H

#include <bao.h>

int64_t baoenclave_dynamic_hypercall(uint64_t, uint64_t, uint64_t, uint64_t);
void alloc_baoenclave(void*, uint64_t);

#endif /* BAOENCLAVE_H */