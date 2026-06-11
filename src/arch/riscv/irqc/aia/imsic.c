/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <imsic.h>
#include <mem.h>
#include <arch/csrs.h>
#include <irqc.h>
#include <cpu.h>
#include <interrupts.h>

#define STOPEI_EEID       (16)

#define IMSIC_EIDELIVERY  (0x70)
#define IMSIC_EITHRESHOLD (0x72)
#define IMSIC_EIP         (0x80)
#define IMSIC_EIE         (0xC0)
struct imsic_intp_file_hw {
    uint32_t seteipnum_le;
    uint32_t seteipnum_be;
} __attribute__((__packed__, aligned(0x1000ULL)));

struct imsic_global_hw {
    struct imsic_intp_file_hw s_file;
    struct imsic_intp_file_hw guest_file[PLAT_IMSIC_NUM_GUEST_FILES];
} __attribute__((__packed__, aligned(0x1000ULL)));

extern volatile struct imsic_global_hw* imsic[PLAT_CPU_NUM];

volatile struct imsic_global_hw* imsic[PLAT_CPU_NUM];
BITMAP_ALLOC(msi_reserved, IMSIC_MAX_INTERRUPTS);

static inline size_t imsic_eie_index(irqid_t int_id)
{
    size_t index = int_id / 32U;

    if (__riscv_xlen == 64) {
        index &= ~1UL;
    }

    return index;
}

static inline size_t imsic_eie_bit(irqid_t int_id)
{
    return (size_t)(int_id % __riscv_xlen);
}

void imsic_init(void)
{
    /** Every intp is triggrable */
    csrs_siselect_write(IMSIC_EITHRESHOLD);
    csrs_sireg_write(0);

    /** Disable all interrupts */
    csrs_siselect_write(IMSIC_EIE);
    csrs_sireg_write(0x0);

    /** Enable interrupt delivery */
    csrs_siselect_write(IMSIC_EIDELIVERY);
    csrs_sireg_write(1);

    /** Map the interrupt files */
    imsic[cpu()->id] = (void*)mem_alloc_map_dev(&cpu()->as, SEC_HYP_GLOBAL, INVALID_VA,
        platform.arch.irqc.aia.imsic.base + (cpu()->id * PLAT_IMSIC_HART_SIZE),
        NUM_PAGES(sizeof(struct imsic_global_hw)));

    csrs_hgeie_write(~0UL);
    cpu()->arch.imsic_guest_int_file_avail = csrs_hgeie_read() & ~1UL;
    csrs_hgeie_write(0UL);
}

void imsic_set_enbl(irqid_t intp_id)
{
    csrs_siselect_write(IMSIC_EIE + imsic_eie_index(intp_id));
    csrs_sireg_set(1UL << imsic_eie_bit(intp_id));
}

bool imsic_get_pend(irqid_t intp_id)
{
    csrs_siselect_write(IMSIC_EIP + imsic_eie_index(intp_id));
    return csrs_sireg_read() && (1ULL << imsic_eie_bit(intp_id));
}

void imsic_clr_pend(irqid_t intp_id)
{
    csrs_siselect_write(IMSIC_EIP + imsic_eie_index(intp_id));
    csrs_sireg_clear(1UL << imsic_eie_bit(intp_id));
}

void imsic_handle(void)
{
    /* Read STOPEI and write to it to claim the interrupt */
    uint32_t intp_identity = (uint32_t)(csrs_stopei_swap(0) >> STOPEI_EEID);

    if (intp_identity != 0) {
        interrupts_handle(intp_identity);
    };
}

irqid_t imsic_allocate_msi(void)
{
    static spinlock_t msi_alloc_lock = SPINLOCK_INITVAL;

    irqid_t msi_id = INVALID_IRQID;

    spin_lock(&msi_alloc_lock);
    ssize_t bit = bitmap_find_nth(msi_reserved, PLAT_IMSIC_MAX_INTERRUPTS, 1, 1, BITMAP_NOT_SET);
    if (bit >= 0) {
        msi_id = (irqid_t)bit;
        bitmap_set(msi_reserved, msi_id);
    }
    spin_unlock(&msi_alloc_lock);

    return msi_id;
}

ssize_t imsic_alloc_guest_int_file(void)
{
    ssize_t guest_int_file = bit_ffs(cpu()->arch.imsic_guest_int_file_avail);
    if (guest_int_file >= 0) {
        cpu()->arch.imsic_guest_int_file_avail =
            bit_clear(cpu()->arch.imsic_guest_int_file_avail, (size_t)guest_int_file);
    }
    return guest_int_file;
}

void imsic_send_msi(cpuid_t target_cpu, irqid_t msi_id)
{
    imsic[target_cpu]->s_file.seteipnum_le = msi_id;
}

void imsic_send_guest_msi(cpuid_t target_cpu, size_t guest_index, irqid_t msi_id)
{
    imsic[target_cpu]->guest_file[guest_index - 1].seteipnum_le = msi_id;
}
