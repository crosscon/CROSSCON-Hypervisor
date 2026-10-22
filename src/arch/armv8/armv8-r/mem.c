/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <mem.h>

void as_arch_init(struct addr_space* as)
{
    UNUSED_ARG(as);
}

void mem_guest_ipa_translate(struct addr_space* as, vaddr_t ipa, paddr_t* pa)
{
    UNUSED_ARG(as);

    if (pa != NULL) {
        *pa = (paddr_t)ipa;
    }
}
