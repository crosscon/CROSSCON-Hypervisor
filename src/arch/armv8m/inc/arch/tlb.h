/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_TLB_H__
#define __ARCH_TLB_H__

#include <crossconhyp.h>
#include <arch/sysregs.h>
#include <arch/fences.h>

static inline void tlb_hyp_inv_va(vaddr_t va)
{
    UNUSED_ARG(va);
}

static inline void tlb_hyp_inv_all(void) { }

static inline void tlb_vm_inv_va(asid_t vmid, vaddr_t va)
{
    UNUSED_ARG(vmid);
    UNUSED_ARG(va);
}

static inline void tlb_vm_inv_all(asid_t vmid)
{
    UNUSED_ARG(vmid);
}

#endif /* __ARCH_TLB_H__ */
