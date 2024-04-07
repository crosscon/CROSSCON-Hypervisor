/**
 CROSSCONHyp Hypervisor
 *
 * Copyright (c) bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __ARCH_CROSSCONHYP_H__
#define __ARCH_CROSSCONHYP_H__

#define RV64 (__riscv_xlen == 64)
#define RV32 (__riscv_xlen == 32)

#if (!(RV64))
#error "Unsupported __riscv_xlen #__riscv_xlen."
#endif

#if (RV64)
#define LOAD ld
#define STORE sd
#define REGLEN (8)
#elif (RV32)
#define LOAD lw
#define STORE sw
#define REGLEN (4)
#endif

#if (RV64)
#define CROSSCONHYP_VAS_BASE (0xffffffc000000000)
#define CROSSCONHYP_CPU_BASE (0xffffffc040000000)
#define CROSSCONHYP_VM_BASE (0xffffffe000000000)
#define CROSSCONHYP_VAS_TOP (0xfffffff000000000)
#elif (RV32)
#define CROSSCONHYP_VAS_BASE (0xc0000000)
#define CROSSCONHYP_CPU_BASE (0x00000000)
#define CROSSCONHYP_VM_BASE (0x00000000)
#define CROSSCONHYP_VAS_TOP (0xffffffff)
#endif

#define PAGE_SIZE (0x1000)

#ifndef __ASSEMBLER__

#endif /* !__ASSEMBLER__ */

#endif /* __ARCH_CROSSCONHYP_H__ */