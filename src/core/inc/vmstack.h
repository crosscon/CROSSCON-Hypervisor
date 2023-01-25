/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef VMSTACK_H
#define VMSTACK_H

#include <bao.h>

#include <cpu.h>
#include <vm.h>

void vmstack_push(struct vcpu* vcpu);
struct vcpu* vmstack_pop();
void vmstack_unwind(struct vcpu* vcpu);
int64_t vmstack_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2);

#endif /* VMSTACK_H */
