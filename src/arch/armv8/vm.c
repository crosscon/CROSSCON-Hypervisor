/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <vm.h>
#include <arch/sysregs.h>
#include <fences.h>
#include <string.h>
#include <config.h>
#include "util.h"

void vm_arch_init(struct vm* vm, const struct vm_config* vm_config)
{
    vm_arch_profile_init(vm);

    if (vm->master == cpu()->id) {
        vgic_init(vm, &vm_config->platform.arch.gic);
    }
    cpu_sync_and_clear_msgs(&vm->sync);
}

struct vcpu* vm_get_vcpu_by_mpidr(struct vm* vm, unsigned long mpidr)
{
    for (cpuid_t vcpuid = 0; vcpuid < vm->cpu_num; vcpuid++) {
        struct vcpu* vcpu = vm_get_vcpu(vm, vcpuid);
        if ((vcpu->arch.vmpidr & MPIDR_AFF_MSK) == (mpidr & MPIDR_AFF_MSK)) {
            return vcpu;
        }
    }

    return NULL;
}

static unsigned long vm_cpuid_to_mpidr(struct vm* vm, vcpuid_t cpuid)
{
    if (cpuid > vm->cpu_num) {
        return ~(~MPIDR_RES1 & MPIDR_RES0_MSK); // return an invlid mpidr by
                                                // inverting res bits
    }

    unsigned long mpidr = cpuid | MPIDR_RES1;

    if (vm->cpu_num == 1) {
        mpidr |= MPIDR_U_BIT;
    }

    return mpidr;
}

void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm)
{
    // /* CROSSCON TODO */
    // /*vcpu->arch.vmpidr = vm_cpuid_to_mpidr(vm, vcpu->id);
    // sysreg_vmpidr_el2_write(vcpu->arch.vmpidr); */
    // This commented code is from CROSSCON but we think it should be like below, like bao, TODO verify
    // vcpu->arch.sysregs.hyp.vmpidr_el2 = vm_cpuid_to_mpidr(vm, vcpu->id);
    // vcpu->arch.sysregs.hyp.cntvoff_el2 = 0;

    // vcpu->arch.psci_ctx.state = vcpu->id == 0 ? ON : OFF;

    // vcpu_arch_profile_init(vcpu, vm);

    // vgic_cpu_init(vcpu);
    //

    vcpu->arch.vmpidr = vm_cpuid_to_mpidr(vm, vcpu->id);
    sysreg_vmpidr_el2_write(vcpu->arch.vmpidr);

    vcpu->arch.psci_ctx.state = vcpu->id == 0 ? ON : OFF;

    vcpu_arch_profile_init(vcpu, vm);

    vgic_cpu_init(vcpu);
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    memset(&vcpu->regs, 0, sizeof(struct arch_regs));

    vcpu_subarch_reset(vcpu);
    /* CROSSCON TODO */
    vcpu->arch.sysregs.hyp.elr_el2 = entry;
    vcpu->arch.sysregs.hyp.spsr_el2 = SPSR_EL1h | SPSR_F | SPSR_I | SPSR_A | SPSR_D;

    vcpu->arch.sysregs.hyp.cntvoff_el2 = 0;
    // sysreg_cntvoff_el2_write(0);
    vcpu_writepc(vcpu, entry);

    /**
     *  See ARMv8-A ARM section D1.9.1 for registers that must be in a known state at reset.
     */
    /* CROSSCON TODO */
    // sysreg_sctlr_el1_write(SCTLR_RES1);
    vcpu->arch.sysregs.vm.sctlr_el1 = SCTLR_RES1;

    vcpu->arch.sysregs.vm.cntkctl_el1 = 0;
    /* sysreg_cntkctl_el1_write(0); */
    /* CROSSCON TODO sysreg_pmcr_el0_write(0); */

    /**
     *  TODO: ARMv8-A ARM mentions another implementation optional registers that reset to a known
     * value.
     */
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    return vcpu->arch.psci_ctx.state == ON;
}

void vtimer_save_state(struct vcpu* vcpu);
void vtimer_save_state(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);
    //    uint64_t timer_ctl = MRS(CNTV_CTL_EL0);
    //    uint64_t timer_cmp = MRS(CNTV_CVAL_EL0);
    //
    //    if((timer_ctl & 0x3) == 0x1 && vm_has_interrupt(vcpu->vm, 27) &&
    //        vgic_int_get_enabled(vcpu, 27)) {
    //
    //        struct vcpu *next_vcpu = cpu.arch.vtimer.next_vcpu;
    //        if(next_vcpu != NULL){
    //            if((next_vcpu->arch.sysregs.vm.cntv_ctl_el0 & 0x3) == 0x1){
    //                if(next_vcpu->arch.sysregs.vm.cntv_cval_el0 > timer_cmp){
    //                    node_data_t *node = objcache_alloc(&partition->nodes);
    //                    node->data = next_vcpu;
    //                    list_push(&cpu.arch.vtimer.event_list, (node_t*)node);
    //                    cpu.arch.vtimer.next_vcpu = vcpu;
    //                    MSR(CNTHP_CTL_EL2, timer_ctl);
    //                    MSR(CNTHP_CVAL_EL2, timer_cmp);
    //                }
    //            }
    //        } else {
    //            MSR(CNTHP_CTL_EL2, timer_ctl);
    //            MSR(CNTHP_CVAL_EL2, timer_cmp);
    //            cpu.arch.vtimer.next_vcpu = vcpu;
    //        }
    //    }
}

void vtimer_restore_state(struct vcpu* vcpu);
void vtimer_restore_state(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);
    //    if(!vm_has_interrupt(vcpu->vm, 27)) {
    //        gic_set_enable(27, false);
    //        return;
    //    }
    //
    //    vgic_hw_commit(vcpu, 27);
    //
    //    if(cpu.arch.vtimer.next_vcpu == vcpu){
    //        node_data_t *node = (node_data_t*) list_pop(&cpu.arch.vtimer.event_list);
    //        if(node != NULL){
    //            struct vcpu *vcpu = node->data;
    //            objcache_free(&partition->nodes, node);
    //            cpu.arch.vtimer.next_vcpu = vcpu;
    //            MSR(CNTHP_CTL_EL2, vcpu->arch.sysregs.vm.cntv_ctl_el0);
    //            MSR(CNTHP_CVAL_EL2, vcpu->arch.sysregs.vm.cntv_cval_el0);
    //        } else {
    //            cpu.arch.vtimer.next_vcpu = NULL;
    //            MSR(CNTHP_CTL_EL2, 0x2);
    //        }
    //    }
}

void vcpu_save_state(struct vcpu* vcpu);
void vcpu_save_state(struct vcpu* vcpu)
{
    if (vcpu == NULL) {
        return;
    }

    vcpu->arch.sysregs.hyp.elr_el2 = sysreg_elr_el2_read();
    vcpu->arch.sysregs.hyp.spsr_el2 = sysreg_spsr_el2_read();
    vcpu->arch.sysregs.hyp.vttbr_el2 = sysreg_vttbr_el2_read();
    vcpu->arch.sysregs.hyp.vmpidr_el2 = sysreg_vmpidr_el2_read();
    vcpu->arch.sysregs.hyp.cntvoff_el2 = sysreg_cntvoff_el2_read();
    vcpu->arch.sysregs.vm.vbar_el1 = sysreg_vbar_el1_read();
    vcpu->arch.sysregs.vm.tpidr_el1 = sysreg_tpidr_el1_read();
    vcpu->arch.sysregs.vm.mair_el1 = sysreg_mair_el1_read();
    vcpu->arch.sysregs.vm.amair_el1 = sysreg_amair_el1_read();
    vcpu->arch.sysregs.vm.tcr_el1 = sysreg_tcr_el1_read();
    vcpu->arch.sysregs.vm.ttbr0_el1 = sysreg_ttbr0_el1_read();
    vcpu->arch.sysregs.vm.ttbr1_el1 = sysreg_ttbr1_el1_read();
    vcpu->arch.sysregs.vm.sp_el0 = sysreg_sp_el0_read();
    vcpu->arch.sysregs.vm.sp_el1 = sysreg_sp_el1_read();
    vcpu->arch.sysregs.vm.spsr_el1 = sysreg_spsr_el1_read();
    vcpu->arch.sysregs.vm.sctlr_el1 = sysreg_sctlr_el1_read();
    vcpu->arch.sysregs.vm.actlr_el1 = sysreg_actlr_el1_read();
    vcpu->arch.sysregs.vm.par_el1 = sysreg_par_el1_read();
    vcpu->arch.sysregs.vm.far_el1 = sysreg_far_el1_read();
    vcpu->arch.sysregs.vm.esr_el1 = sysreg_esr_el1_read();
    vcpu->arch.sysregs.vm.elr_el1 = sysreg_elr_el1_read();
    vcpu->arch.sysregs.vm.afsr0_el1 = sysreg_afsr0_el1_read();
    vcpu->arch.sysregs.vm.afsr1_el1 = sysreg_afsr1_el1_read();
    vcpu->arch.sysregs.vm.tpidrro_el0 = sysreg_tpidrro_el0_read();
    vcpu->arch.sysregs.vm.tpidr_el0 = sysreg_tpidr_el0_read();
    vcpu->arch.sysregs.vm.cntv_ctl_el0 = sysreg_cntv_ctl_el0_read();
    vcpu->arch.sysregs.vm.cntv_cval_el0 = sysreg_cntv_cval_el0_read();
    vcpu->arch.sysregs.vm.cntkctl_el1 = sysreg_cntkctl_el1_read();

    vgic_save_state(vcpu);
    vtimer_save_state(vcpu);
}

void vcpu_restore_state(struct vcpu* vcpu);
void vcpu_restore_state(struct vcpu* vcpu)
{
    if (vcpu == NULL) {
        return;
    }
    sysreg_elr_el2_write(vcpu->arch.sysregs.hyp.elr_el2);
    sysreg_spsr_el2_write(vcpu->arch.sysregs.hyp.spsr_el2);
    sysreg_vttbr_el2_write(vcpu->arch.sysregs.hyp.vttbr_el2);
    sysreg_vmpidr_el2_write(vcpu->arch.sysregs.hyp.vmpidr_el2);
    sysreg_cntvoff_el2_write(vcpu->arch.sysregs.hyp.cntvoff_el2);
    sysreg_vbar_el1_write(vcpu->arch.sysregs.vm.vbar_el1);
    sysreg_tpidr_el1_write(vcpu->arch.sysregs.vm.tpidr_el1);
    sysreg_mair_el1_write(vcpu->arch.sysregs.vm.mair_el1);
    sysreg_amair_el1_write(vcpu->arch.sysregs.vm.amair_el1);
    sysreg_tcr_el1_write(vcpu->arch.sysregs.vm.tcr_el1);
    sysreg_ttbr0_el1_write(vcpu->arch.sysregs.vm.ttbr0_el1);
    sysreg_ttbr1_el1_write(vcpu->arch.sysregs.vm.ttbr1_el1);
    sysreg_sp_el0_write(vcpu->arch.sysregs.vm.sp_el0);
    sysreg_sp_el1_write(vcpu->arch.sysregs.vm.sp_el1);
    sysreg_spsr_el1_write(vcpu->arch.sysregs.vm.spsr_el1);
    sysreg_sctlr_el1_write(vcpu->arch.sysregs.vm.sctlr_el1);
    sysreg_actlr_el1_write(vcpu->arch.sysregs.vm.actlr_el1);
    sysreg_par_el1_write(vcpu->arch.sysregs.vm.par_el1);
    sysreg_far_el1_write(vcpu->arch.sysregs.vm.far_el1);
    sysreg_esr_el1_write(vcpu->arch.sysregs.vm.esr_el1);
    sysreg_elr_el1_write(vcpu->arch.sysregs.vm.elr_el1);
    sysreg_afsr0_el1_write(vcpu->arch.sysregs.vm.afsr0_el1);
    sysreg_afsr1_el1_write(vcpu->arch.sysregs.vm.afsr1_el1);
    sysreg_tpidrro_el0_write(vcpu->arch.sysregs.vm.tpidrro_el0);
    sysreg_tpidr_el0_write(vcpu->arch.sysregs.vm.tpidr_el0);
    sysreg_cntv_ctl_el0_write(vcpu->arch.sysregs.vm.cntv_ctl_el0);
    sysreg_cntv_cval_el0_write(vcpu->arch.sysregs.vm.cntv_cval_el0);
    sysreg_cntkctl_el1_write(vcpu->arch.sysregs.vm.cntkctl_el1);

    vgic_restore_state(vcpu);
    vtimer_restore_state(vcpu);
}
