#include <vm.h>
#include <arch/spmp.h>
#include <arch/instructions.h>

void vm_arch_mem_prot_init(struct vm* vm)
{
    vm->arch.hgatp = 0UL;
}

void vcpu_arch_mem_prot_init(struct vcpu* vcpu)
{
    spmp_init(&vcpu->arch.spmp);
}

void vcpu_arch_mem_prot_reset(struct vcpu* vcpu)
{
    if (VSPMP_NUM_ENTRIES > 0) {
        for (size_t i = 0; i < VSPMP_NUM_ENTRIES - 1; i++) {
            vcpu->regs.vspmp_entry[i].addr = 0UL;
            vcpu->regs.vspmp_entry[i].cfg.raw = 0;
        }

        spmp_cfg_t cfg;
        cfg.x = 1;
        cfg.w = 1;
        cfg.r = 1;
        cfg.a = SPMPCFG_A_NAPOT;
        vcpu->regs.vspmp_entry[VSPMP_NUM_ENTRIES - 1].addr = ~0UL;
        vcpu->regs.vspmp_entry[VSPMP_NUM_ENTRIES - 1].cfg.raw = cfg.raw;

        vcpu->regs.vspmpen |= (1ULL << (VSPMP_NUM_ENTRIES - 1));
    }
}

void vcpu_arch_mem_prot_save_state(struct vcpu* vcpu)
{
    for (size_t i = 0; i < VSPMP_NUM_ENTRIES; i++) {
        csrs_vsiselect_write(SPMP_SISELECT_BASE_ADDR + i);
        vcpu->regs.vspmp_entry[i].addr = csrs_vsireg_read();
        vcpu->regs.vspmp_entry[i].cfg.raw = (uint16_t)csrs_vsireg2_read();
    }

    vcpu->regs.vspmpen = csrs_vspmpen_read();
}

void vcpu_arch_mem_prot_restore_state(struct vcpu* vcpu)
{
    spmp_restore(&vcpu->arch.spmp);

    for (size_t i = 0; i < VSPMP_NUM_ENTRIES; i++) {
        csrs_vsiselect_write(SPMP_SISELECT_BASE_ADDR + i);
        csrs_vsireg_write(vcpu->regs.vspmp_entry[i].addr);
        csrs_vsireg2_write(vcpu->regs.vspmp_entry[i].cfg.raw);
    }

    hfence_vvma();

    csrs_vspmpen_write(vcpu->regs.vspmpen); 
}
