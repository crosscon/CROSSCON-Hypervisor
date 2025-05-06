#include <vtimer.h>
#include <vm.h>
#include <timer.h>

void vtimer_init(struct vtimer* vtimer)
{
    UNUSED_ARG(vtimer);
}

void vtimer_reset(struct vcpu* vcpu)
{
    vcpu->arch.vtimer.cntv_ctl_el0 = 0;
    vcpu->arch.vtimer.cntv_cval_el0 = 0;
    vcpu->arch.vtimer.cntvoff_el2 = 0;
}

void vtimer_save_state(struct vcpu* vcpu)
{
    vcpu->arch.vtimer.cntkctl_el1 = sysreg_cntkctl_el1_read();
    vcpu->arch.vtimer.cntv_ctl_el0 = sysreg_cntv_ctl_el0_read();
    vcpu->arch.vtimer.cntv_cval_el0 = sysreg_cntv_cval_el0_read();
    vgic_hw_irq_save_state(vcpu, VTIMER_IRQ_ID);
    gic_set_enable(VTIMER_IRQ_ID, false);
}

void vtimer_restore_state(struct vcpu* vcpu)
{
    sysreg_cntkctl_el1_write(vcpu->arch.vtimer.cntkctl_el1);
    sysreg_cntv_ctl_el0_write(vcpu->arch.vtimer.cntv_ctl_el0);
    sysreg_cntv_cval_el0_write(vcpu->arch.vtimer.cntv_cval_el0);
    sysreg_cntvoff_el2_write(vcpu->arch.vtimer.cntvoff_el2);
    vgic_hw_commit(vcpu, VTIMER_IRQ_ID);
}
