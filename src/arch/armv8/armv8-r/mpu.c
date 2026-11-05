/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <mem.h>
#include <cpu.h>
#include <vm.h>
#include <config.h>
#include <shmem.h>
#include <string.h>
#include <arch/sysregs.h>
#include <arch/fences.h>

/* Operations run on the local CPU, with guest execution stopped in EL2.
 * Slot ownership belongs to address-space masks, not to cpu()->vcpu.
 */
static struct vcpu* mpu_local_vcpu(struct addr_space* as)
{
    struct vcpu* found = NULL;
    list_foreach (cpu()->vcpu_lst, node_t, node) {
        struct vcpu* vcpu = CONTAINER_OF(struct vcpu, list_node, node);
        if (&vcpu->vm->as == as) {
            if (found != NULL) {
                ERROR("MPU: multiple local vCPUs for AS %lu", (unsigned long)as->id);
            }
            found = vcpu;
        }
    }
    return found;
}

static unsigned long* mpu_owner_mask(struct addr_space* as)
{
    if (as == &cpu()->as) {
        return &cpu()->arch.profile.mpu.mpu_entry_mask;
    }
    struct vcpu* vcpu = mpu_local_vcpu(as);
    return vcpu == NULL ? NULL : &vcpu->arch.mpu_entry_mask;
}

/* Only explicitly declared IPC and MMIO resources are eligible for sharing.
 * Private RAM, HYP entries, locked entries and different IPC objects stay separate.
 */
static struct mpu_share_key mpu_share_key(struct addr_space* as, struct mp_region* mpr,
    bool locked)
{
    struct mpu_share_key key = { .kind = MPU_SHARE_NONE, .id = 0 };
    if (as->type != AS_VM || locked || mpr->as_sec != SEC_VM_ANY) {
        return key;
    }
    struct vcpu* vcpu = mpu_local_vcpu(as);
    if (vcpu == NULL) {
        return key;
    }
    const struct vm_config* cfg = vcpu->vm->config;
    for (size_t i = 0; i < cfg->platform.ipc_num; i++) {
        struct ipc* ipc = &cfg->platform.ipcs[i];
        struct shmem* shmem = shmem_get(ipc->shmem_id);
        if (shmem != NULL && ipc->base == mpr->base && shmem->phys == mpr->base &&
            NUM_PAGES(min(ipc->size, shmem->size)) * PAGE_SIZE == mpr->size) {
            key.kind = MPU_SHARE_IPC;
            key.id = ipc->shmem_id;
            return key;
        }
    }
    for (size_t i = 0; i < cfg->platform.dev_num; i++) {
        struct vm_dev_region* dev = &cfg->platform.devs[i];
        if (dev->va == mpr->base && dev->pa == mpr->base &&
            NUM_PAGES(dev->size) * PAGE_SIZE == mpr->size) {
            key.kind = MPU_SHARE_DEVICE;
            return key;
        }
    }
    return key;
}

static struct mp_region mpu_canonical_region(struct mp_region* mpr)
{
    struct mp_region result = *mpr;
    result.mem_flags.prlar &= (uint16_t)~PRLAR_EN;
    return result;
}

static bool mpu_same_region(struct mp_region* a, struct mp_region* b)
{
    return a->base == b->base && a->size == b->size && a->as_sec == b->as_sec &&
        a->mem_flags.prbar == b->mem_flags.prbar &&
        a->mem_flags.prlar == b->mem_flags.prlar;
}

static mpid_t mpu_find_shared(struct mp_region* region, struct mpu_share_key key)
{
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    if (key.kind == MPU_SHARE_NONE) {
        return INVALID_MPID;
    }
    for (mpid_t i = 0; i < mpu->num_entries; i++) {
        struct mpu_slot* slot = &mpu->slots[i];
        if (slot->refs != 0 && slot->type == AS_VM &&
            !bitmap_get(mpu->locked_entries, i) && slot->share.kind == key.kind &&
            slot->share.id == key.id && mpu_same_region(&slot->region, region)) {
            return i;
        }
    }
    return INVALID_MPID;
}

static mpid_t mpu_find_owned(unsigned long mask, struct mp_region* region)
{
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    for (mpid_t i = 0; i < mpu->num_entries; i++) {
        if ((mask & (1UL << i)) != 0 && mpu->slots[i].refs != 0 &&
            mpu->slots[i].region.base == region->base &&
            mpu->slots[i].region.as_sec == region->as_sec) {
            return i;
        }
    }
    return INVALID_MPID;
}

static void mpu_program(mpid_t id, struct mpu_slot* slot)
{
    unsigned long limit = slot->region.base + slot->region.size - 1;
    unsigned long flags = slot->region.mem_flags.prlar;
    if (slot->type == AS_HYP) {
        flags |= PRLAR_EN;
    }
    DSB();
    sysreg_prselr_el2_write(id);
    ISB();
    sysreg_prlar_el2_write(0);
    ISB();
    sysreg_prbar_el2_write((slot->region.base & PRBAR_BASE_MSK) |
        slot->region.mem_flags.prbar);
    sysreg_prlar_el2_write((limit & PRLAR_LIMIT_MSK) | flags);
    DSB();
    ISB();
}

static mpid_t mpu_new_slot(struct addr_space* as, struct mp_region* region,
    struct mpu_share_key key, bool locked)
{
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    ssize_t id = bitmap_find_nth(mpu->allocated_entries, mpu->num_entries, 1, 0,
        BITMAP_NOT_SET);
    if (id < 0) {
        return INVALID_MPID;
    }
    mpid_t mpid = (mpid_t)id;
    mpu->slots[mpid] = (struct mpu_slot){
        .region = *region, .type = as->type, .share = key, .refs = 0,
    };
    bitmap_set(mpu->allocated_entries, mpid);
    if (locked) {
        bitmap_set(mpu->locked_entries, mpid);
    }
    mpu_program(mpid, &mpu->slots[mpid]);
    return mpid;
}

static void mpu_bind(unsigned long* mask, mpid_t id)
{
    if ((*mask & (1UL << id)) != 0) {
        ERROR("MPU: duplicate physical binding %lu", (unsigned long)id);
    }
    cpu()->arch.profile.mpu.slots[id].refs++;
    *mask |= 1UL << id;
}

static void mpu_release(unsigned long* mask, mpid_t id)
{
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    if ((*mask & (1UL << id)) == 0 || mpu->slots[id].refs == 0) {
        ERROR("MPU: invalid physical binding %lu", (unsigned long)id);
    }
    *mask &= ~(1UL << id);
    if (--mpu->slots[id].refs == 0) {
        DSB();
        sysreg_prselr_el2_write(id);
        ISB();
        sysreg_prlar_el2_write(0);
        sysreg_prbar_el2_write(0);
        DSB();
        ISB();
        bitmap_clear(mpu->allocated_entries, id);
        bitmap_clear(mpu->locked_entries, id);
        memset(&mpu->slots[id], 0, sizeof(mpu->slots[id]));
    }
}

bool mpu_map(struct addr_space* as, struct mp_region* mpr, bool locked)
{
    unsigned long* mask = mpu_owner_mask(as);
    if (mask == NULL || mpr->size == 0 || (as->type != AS_VM && as->type != AS_HYP)) {
        return false;
    }
    struct mp_region region = mpu_canonical_region(mpr);
    struct mpu_share_key key = mpu_share_key(as, mpr, locked);
    mpid_t id = mpu_find_shared(&region, key);
    if (id == INVALID_MPID) {
        id = mpu_new_slot(as, &region, key, locked);
    }
    if (id == INVALID_MPID) {
        ERROR("MPU exhausted: AS %lu base 0x%lx size 0x%lx capacity %lu",
            (unsigned long)as->id, (unsigned long)mpr->base, (unsigned long)mpr->size,
            (unsigned long)cpu()->arch.profile.mpu.num_entries);
        return false;
    }
    mpu_bind(mask, id);
    return true;
}

bool mpu_unmap(struct addr_space* as, struct mp_region* mpr)
{
    unsigned long* mask = mpu_owner_mask(as);
    if (mask == NULL) {
        return false;
    }
    struct mp_region region = mpu_canonical_region(mpr);
    mpid_t id = mpu_find_owned(*mask, &region);
    if (id == INVALID_MPID ||
        !mpu_same_region(&cpu()->arch.profile.mpu.slots[id].region, &region)) {
        return false;
    }
    mpu_release(mask, id);
    return true;
}

bool mpu_update(struct addr_space* as, struct mp_region* mpr)
{
    unsigned long* mask = mpu_owner_mask(as);
    if (mask == NULL) {
        return false;
    }
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    struct mp_region region = mpu_canonical_region(mpr);
    mpid_t old = mpu_find_owned(*mask, &region);
    if (old == INVALID_MPID) {
        return false;
    }
    if (mpu_same_region(&mpu->slots[old].region, &region)) {
        return true;
    }
    bool locked = bitmap_get(mpu->locked_entries, old) != 0;
    struct mpu_share_key key = mpu_share_key(as, mpr, locked);
    mpid_t id = mpu_find_shared(&region, key);
    if (id == INVALID_MPID && mpu->slots[old].refs == 1) {
        /* No other owner can observe this update. */
        mpu->slots[old].region = region;
        mpu->slots[old].share = key;
        mpu_program(old, &mpu->slots[old]);
        return true;
    }
    if (id == INVALID_MPID) {
        id = mpu_new_slot(as, &region, key, locked);
    }
    if (id == INVALID_MPID) {
        /* A coalesce is optional: keep both original mappings unchanged. */
        return false;
    }
    mpu_bind(mask, id);
    mpu_release(mask, old);
    return true;
}

bool mpu_perms_compatible(unsigned long perms1, unsigned long perms2)
{
    return perms1 == perms2;
}

void mpu_enable(void)
{
    unsigned long value = sysreg_sctlr_el2_read();
    sysreg_sctlr_el2_write(value | SCTLR_M | SCTLR_C);
    DSB(ish);
    ISB();
}

void mpu_init(void)
{
    struct mpu_arch* mpu = &cpu()->arch.profile.mpu;
    memset(mpu, 0, sizeof(*mpu));
    mpu->num_entries = MPUIR_REGION(sysreg_mpuir_el2_read());
    if (mpu->num_entries == 0 || mpu->num_entries > MPU_ARCH_MAX_NUM_ENTRIES ||
        mpu->num_entries > sizeof(unsigned long) * 8) {
        ERROR("MPU: unsupported physical capacity %lu", (unsigned long)mpu->num_entries);
    }
    /* Called before MPU enable; do not inherit untracked enabled regions. */
    for (mpid_t i = 0; i < mpu->num_entries; i++) {
        sysreg_prselr_el2_write(i);
        ISB();
        sysreg_prlar_el2_write(0);
        sysreg_prbar_el2_write(0);
    }
    DSB();
    ISB();
}
