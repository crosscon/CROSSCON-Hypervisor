/**
 * CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
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

#ifndef __ARCH_CPU_H__
#define __ARCH_CPU_H__

#include <crossconhyp.h>
#include <arch/psci.h>
#include <list.h>

#define CPU_MAX (8UL)

struct cpu_arch {
    struct psci_off_state psci_off_state;
    unsigned long mpidr;
    struct {
        struct vcpu * next_vcpu;
        struct list event_list;
    } vtimer;
};

unsigned long cpu_id_to_mpidr(cpuid_t id);
cpuid_t cpu_mpidr_to_id(unsigned long mpdir);

extern cpuid_t CPU_MASTER;

#endif /* __ARCH_CPU_H__ */
