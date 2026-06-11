/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/platform.h>
#include <mem.h>
#include <imsic.h>

void vimsic_init(struct vm* vm, const union vm_irqc_dscrp* vm_irqc_dscrp)
{
    if (cpu()->id == vm->master) {
        paddr_t imsic_paddr;
        vaddr_t imsic_vaddr = vm_irqc_dscrp->aia.imsic.base;

        for (size_t i = 0; i < vm->cpu_num; i++) {
            struct vcpu* vcpu = vm_get_vcpu(vm, i);
            imsic_paddr = platform.arch.irqc.aia.imsic.base +
                (vcpu->phys_id * PLAT_IMSIC_HART_SIZE) +
                (PAGE_SIZE * vcpu->arch.imsic_guest_file_index);

#ifdef MEM_PROT_MPU
            /**
             * For SPMP-based systems, we can't give the guest a true view of IMSIC layout as
             * defined by the AIA spec, since we don't have translation to map the guest interrupt
             * files contigously for the guest. This might be a problem for some guests that expect
             * this layout.
             */
            imsic_vaddr = imsic_paddr;
#endif

            mem_alloc_map_dev(&vm->as, SEC_VM_ANY, imsic_vaddr, imsic_paddr, 1);

            imsic_vaddr += PAGE_SIZE;
        }
    }
}
