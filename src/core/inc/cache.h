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

#ifndef __CACHE_H__
#define __CACHE_H__

#include <crossconhyp.h>
#include <arch/cache.h>

struct cache {
    size_t lvls;
    size_t min_shared_lvl;
    enum { UNIFIED, SEPARATE, DATA, INSTRUCTION } type[CACHE_MAX_LVL];
    enum { PIPT, VIPT } indexed[CACHE_MAX_LVL][2];
    size_t line_size[CACHE_MAX_LVL][2];
    size_t assoc[CACHE_MAX_LVL][2];
    size_t numset[CACHE_MAX_LVL][2];
};

extern size_t COLOR_NUM;
extern size_t COLOR_SIZE;

void cache_enumerate();
void cache_flush_range(vaddr_t base, size_t size);

void cache_arch_enumerate(struct cache* dscrp);

#endif /* __CACHE_H__ */
