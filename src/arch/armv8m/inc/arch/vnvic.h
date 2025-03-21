#ifndef VNVIC_H
#define VNVIC_H

#include <bao.h>
#include <bitmap.h>
#include <arch/interrupts.h>

struct vnvic {
    BITMAP_ALLOC(irq_enab, MAX_INTERRUPTS);
    BITMAP_ALLOC(irq_pend, MAX_INTERRUPTS);
};

struct vnvic;
struct vcpu;

void vnvic_init(void);
void vnvic_reset(void);
void vnvic_save_state(struct vnvic* vnvic, bitmap_t* vm_irqs);
void vnvic_restore_state(struct vnvic* vnvic, bitmap_t* vm_irqs);
void vnvic_inject(struct vcpu* vcpu, irqid_t id);

#endif /* VNVIC_H */
