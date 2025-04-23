
#include <vm.h>
#include <mem.h>
#include <page_table.h>
#include <arch/csrs.h>

void vm_arch_mem_prot_init(struct vm* vm)
{
    paddr_t root_pt_pa;
    mem_translate(&cpu()->as, (vaddr_t)vm->as.pt.root, &root_pt_pa);

    vm->arch.hgatp = (root_pt_pa >> PAGE_SHIFT) | (HGATP_MODE_DFLT) |
        ((vm->id << HGATP_VMID_OFF) & HGATP_VMID_MSK);
}
