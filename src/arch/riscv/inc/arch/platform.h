/**
 CROSSCONHyp Hypervisor
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

#ifndef __ARCH_PLATFORM_H__
#define __ARCH_PLATFORM_H__

#include <crossconhyp.h>

struct arch_platform {
    paddr_t plic_base;
};

#endif /* __ARCH_PLATFORM_H__ */
