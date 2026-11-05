/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_MPU_H__
#define __ARCH_MPU_H__

#include <crossconhyp.h>
#include <arch/sysregs.h>
#include <bitmap.h>
#include <mem.h>

enum mpu_share_kind { MPU_SHARE_NONE, MPU_SHARE_DEVICE, MPU_SHARE_IPC };

struct mpu_share_key {
    enum mpu_share_kind kind;
    size_t id; /* IPC: shmem_id; device: interval is part of the region key. */
};

struct mpu_slot {
    struct mp_region region; /* Logical attributes; never stores the runtime enable bit. */
    enum AS_TYPE type;
    struct mpu_share_key share;
    size_t refs; /* One binding per local address space, represented by its mask. */
};

struct mpu_arch {
    BITMAP_ALLOC(allocated_entries, MPU_ARCH_MAX_NUM_ENTRIES);
    BITMAP_ALLOC(locked_entries, MPU_ARCH_MAX_NUM_ENTRIES);
    struct mpu_slot slots[MPU_ARCH_MAX_NUM_ENTRIES];
    size_t num_entries;
    unsigned long mpu_entry_mask;
};

#endif /* __ARCH_MPU_H__ */
