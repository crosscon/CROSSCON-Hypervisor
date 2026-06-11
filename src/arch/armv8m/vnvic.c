#include <vm.h>
#include <arch/vnvic.h>
#include <arch/nvic.h>
#include <arch/vm.h>
#include <bitmap.h>

void vnvic_init(void) { }

void vnvic_reset(void) { }

void vnvic_inject(struct vcpu* vcpu, irqid_t id)
{
    struct vnvic* vnvic = &vcpu->arch.vnvic;
    bitmap_set(vnvic->irq_pend, id);
}

static void vnvic_save_interrupt(irqid_t int_id, struct vnvic* vnvic)
{
    // Read int enable status and store in bitmap
    if (nvic_get_en(nvic_ns, int_id)) {
        bitmap_set(vnvic->irq_enab, int_id);
        // TODO:ARMV8M - We need to test if by disabling the interrupt, the irq still gets pended if
        //  triggered
        //  Deactivate int to prevent triggering of int inside bao
        nvic_enable(nvic_ns, int_id, false); // this forces to disable the interrupt in the hardware, but we want to keep this int enabled for the secure VM to interrupt non-secure VM or vice-versa
        nvic_enable(nvic_s, int_id, true); //the interrupt is handled in secure for being re-directed to the VM it belongs
    } else {
        bitmap_clear(vnvic->irq_enab, int_id);
    }

    // Read int pending status and store in bitmap
    if (nvic_get_pend(nvic_ns, int_id)) {
        bitmap_set(vnvic->irq_pend, int_id);
    } else {
        bitmap_clear(vnvic->irq_pend, int_id);
    }

    // Attention: "Secure software must ensure that when changing the target Security state of
    // an exception, the exception is not pending or active." - ARMv8-M Architecture Reference
    // Manual
    nvic_int_target(SECURE, int_id);
}

static void vnvic_restore_interrupt(irqid_t int_id, bool en, bool pend)
{
    nvic_int_target(NONSECURE, int_id);

    if (en) {
        nvic_enable(nvic_ns, int_id, true);
    }
    if (pend) {
        nvic_set_pend(nvic_ns, int_id);
    }
}

void vnvic_save_state(struct vnvic* vnvic, bitmap_t* vm_irqs)
{
    // TODO-ARMV8M - This can be optimized
    for (irqid_t int_id = 0; int_id < MAX_INTERRUPTS; int_id++) {
        if (bitmap_get(vm_irqs, int_id)) { //percorre todas as interrupções da vm e salva o estado de cada uma (colocando-a como segura)
            vnvic_save_interrupt(int_id, vnvic);
        }
    }
}

void vnvic_restore_state(struct vnvic* vnvic, bitmap_t* vm_irqs)
{
    // TODO-ARMV8M - This can be optimized
    for (irqid_t int_id = 0; int_id < MAX_INTERRUPTS; int_id++) {
        if (bitmap_get(vm_irqs, int_id)) { //percorre todas as interrupções da vm e salva o estado de cada uma (colocando-a como non-segura)
            vnvic_restore_interrupt(int_id, bitmap_get(vnvic->irq_enab, int_id),
                bitmap_get(vnvic->irq_pend, int_id));
        }
    }
}
