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

/**
 * TODO: this is a naive implementation to get things going.
 * Optimizations needed. See ticket locks in ARMv8-A.
 */

#ifndef __ARCH_SPINLOCK__
#define __ARCH_SPINLOCK__

#include <bao.h>


typedef struct {
    uint32_t ticket;
    uint32_t next;
} spinlock_t;

#define SPINLOCK_INITVAL ((spinlock_t){ 0, 0 })

static inline void spinlock_init(spinlock_t* lock)
{
    lock->ticket = 0;
    lock->next = 0;
}

static inline void spin_lock(spinlock_t* lock)
{
    uint32_t ticket;
    uint32_t next;
    uint32_t temp;

    (void)lock;
    __asm__ volatile(
        /* Get ticket */
        "1:\n\t"
        "ldaxr  %w0, %3\n\t"
        "add    %w1, %w0, 1\n\t"
        "stxr   %w2, %w1, %3\n\t"
        "cbnz   %w2, 1b\n\t"
        /* Wait for your turn */
        "2:\n\t"
        "ldar   %w1, %4\n\t"
        "cmp    %w0, %w1\n\t"
        "b.eq   3f\n\t"
        "wfe\n\t"
        "b 2b\n\t"
        "3:\n\t" : "=&r"(ticket), "=&r"(next), "=&r"(temp) : "Q"(lock->ticket), "Q"(lock->next)
        : "memory");
}

static inline void spin_unlock(spinlock_t* lock)
{
    uint32_t temp;

    __asm__ volatile(
        /* increment to next ticket */
        "ldr    %w0, %1\n\t"
        "add    %w0, %w0, 1\n\t"
        "stlr   %w0, %1\n\t"
        "dsb ish\n\t"
        "sev\n\t" : "=&r"(temp) : "Q"(lock->next) : "memory");}

#endif /* __ARCH_SPINLOCK__ */
