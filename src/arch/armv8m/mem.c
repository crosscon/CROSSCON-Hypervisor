/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <list.h>
#include <mem.h>
#include <cpu.h>
#include <vm.h>
#include <arch/mpu.h>
#include <arch/sau.h>

void as_arch_init(struct addr_space* as)
{
    UNUSED_ARG(as);
}

bool mpu_map(struct addr_space* as, struct mp_region* mpr, bool locked)
{
    bool failed = true;

    if (as->type == AS_HYP) {
        /* Add region to MPU */
        if (!mpu_add_region(mpr, locked)) {
            ERROR("failed to register mpu entry");
        } else {
            failed = false;
        }
    } else if (as->type == AS_VM) {
        /* Add region to SAU  */
        if (!sau_add_region(as, mpr, locked)) {
            ERROR("failed to register sau entry");
        } else {
            failed = false;
        }
    }
    return !failed;
}

bool mpu_unmap(struct addr_space* as, struct mp_region* mpr)
{
    bool failed = true;

    if (as->type == AS_HYP) {
        /* Distinguish if the region is for hypercall or part of Bao image */
        if (mpr->as_sec == SEC_HYP_HC) {
            /* Add region to SAU as NSC */
            if (!sau_remove_region(as, mpr)) {
                ERROR("failed to register sau nsc entry");
            } else {
                failed = false;
            }
        } else {
            /* Remove region */
            if (!mpu_remove_region(mpr)) {
                ERROR("failed to register mpu entry");
            } else {
                failed = false;
            }
        }
    } else {
        if (as->type == AS_VM) {
            /* Remove region */
            if (!sau_remove_region(as, mpr)) {
                ERROR("failed to register sau entry");
            } else {
                failed = false;
            }
        }
    }

    return !failed;
}

bool mpu_update(struct addr_space* as, struct mp_region* mpr)
{
    bool failed = true;

    if (as->type == AS_HYP) {
        /* Update region */
        if (!mpu_update_region(mpr)) {
            ERROR("failed to register mpu entry");
        } else {
            failed = false;
        }
    } else {
        if (as->type == AS_VM) {
            /* Update region */
            if (!sau_update_region(as, mpr)) {
                ERROR("failed to register sau entry");
            } else {
                failed = false;
            }
        }
    }

    return !failed;
}

bool mpu_perms_compatible(unsigned long perms1, unsigned long perms2)
{
    // bool failed = true;

    // if (as->type == AS_HYP) {
    //     if (!mpu_arch_perms_compatible(perms1, perms2)) {
    //     } else {
    //         failed = false;
    //     }
    // } else {
    //     if (as->type == AS_VM) {
    //         if (!sau_perms_compatible(perms1, perms2)) {
    //         } else {
    //             failed = false;
    //         }
    //     }
    // }

    // return !failed;


    return perms1 == perms2;
}

void mem_guest_ipa_translate(struct addr_space* as, vaddr_t ipa, paddr_t* pa)
{
    UNUSED_ARG(as);
    UNUSED_ARG(ipa);
    UNUSED_ARG(pa);
}

void mpu_init(void)
{
    mpu_arch_init();
}

void mpu_enable()
{
    mpu_arch_enable();
}
