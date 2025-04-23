/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <arch/sau.h>
#include <vmm.h>

void vmm_arch_init(void)
{
    sau_arch_enable();
}
