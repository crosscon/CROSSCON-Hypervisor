/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <mem.h>
#include <cpu.h>
#include <vm.h>
#include <arch/sysregs.h>
#include <arch/fences.h>
#include <arch/sau.h>

struct sau_temp {
    unsigned long rbar;
    unsigned long rlar;
    unsigned long rlar_attr;
    unsigned long rbar_attr;
} sau_temp[8];

void sau_read_and_save(void);

void sau_read_and_save(void)
{
    for (size_t i = 0; i < 8; i++) {
        sau->rnr = i;
        /*sau_temp[i].rbar = sau->rbar & SAU_RBAR_BADDR_MSK;

        if (sau_temp[i].rbar & 0x1) {
            sau_temp[i].rlar = sau->rlar | 0x1F;
        } else {
            sau_temp[i].rlar = sau->rlar;
        }*/
        sau_temp[i].rbar_attr = sau->rbar & 0x1F;
        sau_temp[i].rbar = sau->rbar & SAU_RBAR_BADDR_MSK;
        sau_temp[i].rlar = sau->rlar | 0x1F;
        sau_temp[i].rlar_attr = sau->rlar & 0x1F;
    }
}

static inline size_t sau_num_entries(void)
{
    return (size_t)SAU_TYPE_N_RGN(sau->type);
}

static inline void sau_lock_entry(mpid_t mpid)
{
    bitmap_set(cpu()->vcpu->arch.sau_vm.locked, mpid);
}

static inline bool sau_entry_locked(mpid_t mpid)
{
    return !!bitmap_get(cpu()->vcpu->arch.sau_vm.locked, mpid);
}

static void sau_entry_set(struct sau_vm* sau_vm, mpid_t mpid, struct mp_region* mpr)
{
    unsigned long lim = mpr->base + mpr->size - 1;
    unsigned long rbar = (mpr->base & SAU_RBAR_BADDR_MSK);
    unsigned long rlar = (lim & SAU_RLAR_LADDR_MSK) | mpr->mem_flags.rlar;

    sau->rnr = mpid;
    ISB();
    sau->rbar = rbar;
    sau_vm->entry[mpid].rbar = rbar;
    sau->rlar = rlar;
    sau_vm->entry[mpid].rlar = rlar;
}

static mpid_t sau_entry_allocate(void)
{
    mpid_t reg_num = INVALID_MPID;
    for (mpid_t i = 0; i < (mpid_t)sau_num_entries(); i++) {
        if (bitmap_get(cpu()->vcpu->arch.sau_vm.bitmap, i) == 0) {
            bitmap_set(cpu()->vcpu->arch.sau_vm.bitmap, i);
            reg_num = i;
            break;
        }
    }
    return reg_num;
}

static struct sau_vm* sau_vm_get_local(struct addr_space* as)
{
    struct sau_vm* sau_vm = NULL;

    list_foreach (cpu()->vcpu_list, struct vcpu, vcpu) {
        if (as == &vcpu->vm->as) {
            sau_vm = &vcpu->arch.sau_vm;
            break;
        }
    }

    return sau_vm;
}

bool sau_add_region(struct addr_space* as, struct mp_region* reg, bool locked)
{
    bool failed = true;
    struct sau_vm* sau_vm = sau_vm_get_local(as);

    if (reg->size > 0) {
        mpid_t mpid = sau_entry_allocate();

        if (mpid != INVALID_MPID) {
            failed = false;
            sau_entry_set(sau_vm, mpid, reg);
            if (locked) {
                sau_lock_entry(mpid);
            }
        }
    }

    sau_read_and_save();

    return !failed;
}

bool sau_perms_compatible(mem_flags_t perms1, mem_flags_t perms2)
{
    UNUSED_ARG(perms1);
    UNUSED_ARG(perms2);
    // TODO:ARMV8M - IMPLEMENT on all archs
    // uint8_t perms_mask = SPMPCFG_S_BIT | SPMPCFG_R_BIT | SPMPCFG_W_BIT | SPMPCFG_X_BIT;
    // return (perms1 & perms_mask) == (perms2 & perms_mask);

    return 1;
}

static void sau_entry_get_region(mpid_t mpid, struct mp_region* mpe)
{
    sau->rnr = mpid;
    ISB();

    unsigned long rbar = sau->rbar;
    unsigned long rlar = sau->rlar;

    mpe->mem_flags.rlar = SAU_RLAR_FLAGS(rlar);
    mpe->base = SAU_RBAR_BASE(rbar);
    mpe->size = (SAU_RLAR_LIMIT(rlar) + 1) - mpe->base;
    mpe->as_sec = SEC_UNKNOWN;
}

static mpid_t sau_entry_get_region_id(struct mp_region* mpe)
{
    mpid_t mpid = INVALID_MPID;

    for (mpid_t i = 0; i < (mpid_t)sau_num_entries(); i++) {
        struct mp_region mpe_cmp;
        sau_entry_get_region(i, &mpe_cmp);

        if (mpe_cmp.base == mpe->base && mpe_cmp.size == mpe->size) {
            mpid = i;
            break;
        }
    }

    return mpid;
}

static void sau_entry_clear(struct sau_vm* sau_vm, mpid_t mpid)
{
    sau->rnr = mpid;
    ISB();
    sau->rlar = 0;
    sau_vm->entry[mpid].rlar = 0;
    sau->rbar = 0;
    sau_vm->entry[mpid].rbar = 0;
}

static inline void sau_entry_free(struct sau_vm* sau_vm, mpid_t mpid)
{
    sau_entry_clear(sau_vm, mpid);
    bitmap_clear(cpu()->vcpu->arch.sau_vm.bitmap, mpid);
}

bool sau_remove_region(struct addr_space* as, struct mp_region* reg)
{
    bool failed = true;
    struct sau_vm* sau_vm = sau_vm_get_local(as);

    if (reg->size > 0) {
        mpid_t mpid = sau_entry_get_region_id(reg);

        if (mpid != INVALID_MPID) {
            failed = false;
            sau_entry_free(sau_vm, mpid);
        }
    }
    // TODO:ARMV8M - REMOVE
    sau_read_and_save();

    return !failed;
}

bool sau_update_region(struct addr_space* as, struct mp_region* mpr)
{
    bool failed = true;
    struct sau_vm* sau_vm = sau_vm_get_local(as);

    for (mpid_t mpid = 0; mpid < (mpid_t)sau_num_entries(); mpid++) {
        if (bitmap_get(cpu()->vcpu->arch.sau_vm.bitmap, mpid) == 0) {
            continue;
        }
        struct mp_region mpe_cmp;
        sau_entry_get_region(mpid, &mpe_cmp);

        if (mpe_cmp.base == mpr->base) {
            sau_entry_set(sau_vm, mpid, mpr);
            failed = false;
            break;
        }
    }
    // TODO:ARMV8M - REMOVE
    sau_read_and_save();

    return !failed;
}

static inline bool sau_entry_valid(mpid_t mpid)
{
    sau->rnr = mpid;
    ISB();
    return !!(sau->rlar & SAU_RLAR_EN);
}

void sau_arch_init(void)
{
    bitmap_clear_consecutive(cpu()->vcpu->arch.sau_vm.bitmap, 0, sau_num_entries());

    for (mpid_t mpid = 0; mpid < (mpid_t)sau_num_entries(); mpid++) {
        if (sau_entry_valid(mpid)) {
            bitmap_set(cpu()->vcpu->arch.sau_vm.bitmap, mpid);
            bitmap_set(cpu()->vcpu->arch.sau_vm.locked, mpid);
        }
    }
}

void sau_arch_enable(void)
{
    sau->ctrl |= SAU_CTRL_ENABLE;
    ISB();
}

void sau_arch_disable(void)
{
    sau->ctrl &= ~SAU_CTRL_ENABLE;
    ISB();
}

void sau_restore(struct sau_vm* sau_vm)
{
    sau_arch_disable();
    for (mpid_t i = 0; i < SAU_ARCH_MAX_NUM_ENTRIES; i++) {
        sau->rnr = i;
        ISB();
        sau->rlar = sau_vm->entry[i].rlar;
        sau->rbar = sau_vm->entry[i].rbar;
    }
    sau_arch_enable();
    sau_read_and_save();
}
