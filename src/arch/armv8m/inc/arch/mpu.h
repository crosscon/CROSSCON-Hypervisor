
/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef MPU_ARCH_H
#define MPU_ARCH_H

#include <crossconhyp.h>
#include <arch/sysregs.h>
#include <arch/mem.h>

typedef union {
    unsigned long raw;
    struct {
        uint16_t rbar;
        uint16_t rlar;
    };
} mpu_flags_t;

static inline void mpu_set_ctrl(struct mpu* mpu, uint32_t val)
{
    mpu->ctrl = val;
}

static inline uint32_t mpu_get_ctrl(struct mpu* mpu)
{
    return mpu->ctrl;
}

static inline void mpu_set_rnr(struct mpu* mpu, uint32_t val)
{
    mpu->rnr = val;
}

static inline uint32_t mpu_get_rnr(struct mpu* mpu)
{
    return mpu->rnr;
}

static inline void mpu_set_rbar(struct mpu* mpu, uint32_t val)
{
    mpu->rbar = val;
}

static inline uint32_t mpu_get_rbar(struct mpu* mpu)
{
    return mpu->rbar;
}

static inline void mpu_set_rlar(struct mpu* mpu, uint32_t val)
{
    mpu->rlar = val;
}

static inline uint32_t mpu_get_rlar(struct mpu* mpu)
{
    return mpu->rlar;
}

static inline void mpu_set_mair0(struct mpu* mpu, uint32_t val)
{
    mpu->mair0 = val;
}

static inline uint32_t mpu_get_mair0(struct mpu* mpu)
{
    return mpu->mair0;
}

static inline void mpu_set_mair1(struct mpu* mpu, uint32_t val)
{
    mpu->mair1 = val;
}

static inline uint32_t mpu_get_mair1(struct mpu* mpu)
{
    return mpu->mair1;
}

struct mp_region;

void mpu_arch_init(void);
void mpu_arch_enable(void);
bool mpu_add_region(struct mp_region* reg, bool locked);
bool mpu_remove_region(struct mp_region* reg);
bool mpu_update_region(struct mp_region* reg);

#endif /* MPU_ARCH_H */
