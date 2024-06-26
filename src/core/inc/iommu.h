/**
 crossconhyphu separation kernel
 *
 * Copyright (c) Jose Martins, Sandro Pinto, David Cerdeira
 *
 * Authors:
 *      David Cerdeira <davidmcerdeira@gmail.com>
 *
 crossconhyphu is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __IOMMU_H__
#define __IOMMU_H__

#include <crossconhyp.h>
#include <arch/iommu.h>
#include <objcache.h>
#include <list.h>

struct vm_config;
struct vm;

struct iommu_vm {
    struct list dev_list;
    struct objcache dev_oc;
    struct iommu_vm_arch arch;
};

/* Mainly for HW initialization. */
void iommu_init();

/* iommu api for vms. */
bool iommu_vm_init(struct vm *vm, const struct vm_config *config);
bool iommu_vm_add_device(struct vm *vm, streamid_t dev_id);

/* Must be implemented by architecture. */
bool iommu_arch_init();
bool iommu_arch_vm_init(struct vm *vm, const struct vm_config *config);
bool iommu_arch_vm_add_device(struct vm *vm, streamid_t id);

#endif
