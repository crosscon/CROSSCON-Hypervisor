/**
 * SPDX-License-Identifier: Apache-2.0U
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef NVIC_H
#define NVIC_H

#include <bao.h>
#include <arch/sysregs.h>
#include <arch/systick.h>

typedef enum int_target { SECURE = 0, NONSECURE = 1 } int_target_t;

static inline irqid_t nvic_linearize_id(irqid_t id)
{
    // In Armv8-M, NVIC external interrupts start from zero. Linearize the ID to match Bao's global
    // interrupt notion.
    return id - (irqid_t)EXT_IRQ_BASE;
}

static inline bool nvic_conflict(bitmap_t* interrupt_bitmap, irqid_t int_id)
{
    return bitmap_get(interrupt_bitmap, int_id);
}

// TODO:ARMV8M - Dismantle into separate functions
static inline void nvic_enable(struct nvic* ic, irqid_t int_id, bool en)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    if (en) {
        ic->iser[linear_id >> 5UL] |= (irqid_t)(1UL << (linear_id & 0x1FUL));
    } else {
        ic->icer[linear_id >> 5UL] |= (irqid_t)(1UL << (linear_id & 0x1FUL));
    }
}

static inline bool nvic_get_en(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    return ((ic->iser[linear_id >> 5UL] & (irqid_t)(1UL << (linear_id & 0x1FUL))) != 0UL) ? true :
                                                                                            false;
}

static inline bool nvic_get_act(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    return ((ic->iabr[linear_id >> 5UL] & (irqid_t)(1UL << (linear_id & 0x1FUL))) != 0UL) ? true :
                                                                                            false;
}

static inline void systick_int_enable(struct systick* st, bool en)
{
    if (en) {
        st->csr |= (irqid_t)SYSTICK_CSR_TICKINT;
    } else {
        st->csr &= ~(irqid_t)SYSTICK_CSR_TICKINT;
    }
}

static inline bool nvic_get_pend(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    return ((ic->ispr[linear_id >> 5UL] & (irqid_t)(1UL << (linear_id & 0x1FUL))) != 0UL) ? true :
                                                                                            false;
}

static inline bool systick_get_pend(struct systick* st)
{
    if (st == systick_ns) {
        return ((scb_ns->icsr & SCB_ICSR_PENDSTSET) != 0UL) ? true : false;
    } else {
        return ((scb_s->icsr & SCB_ICSR_PENDSTSET) != 0UL) ? true : false;
    }
}

static inline void nvic_set_pend(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    ic->ispr[linear_id >> 5UL] |= (irqid_t)(1UL << (linear_id & 0x1FUL));
}

static inline void systick_set_pend(struct systick* st)
{
    if (st == systick_ns) {
        scb_ns->icsr |= SCB_ICSR_PENDSTSET;
    } else {
        scb_s->icsr |= SCB_ICSR_PENDSTSET;
    }
}

static inline void nvic_clr_pend(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    ic->icpr[linear_id >> 5UL] |= (irqid_t)(1UL << (linear_id & 0x1FUL));
}

static inline void systick_clr_pend(struct systick* st)
{
    if (st == systick_ns) {
        scb_ns->icsr |= SCB_ICSR_PENDSTCLR;
    } else {
        scb_s->icsr |= SCB_ICSR_PENDSTCLR;
    }
}

static inline void nvic_int_target(int_target_t target, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    if (!target) { // secure
        nvic_s->itns[linear_id >> 5UL] &= ~(irqid_t)(1UL << (linear_id & 0x1FUL));
    } else {       // non-secure
        nvic_s->itns[linear_id >> 5UL] |= (irqid_t)(1UL << (linear_id & 0x1FUL));
    }
}

static inline void nvic_set_prio(struct nvic* ic, irqid_t int_id, uint32_t prio)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    ic->ipr[linear_id] = (uint8_t)((prio << (8U - NVIC_PRIO_BITS)) & (uint32_t)0xFFUL);
}

static inline void systick_set_prio(struct systick* st, uint32_t prio)
{
    if (st == systick_ns) {
        scb_ns->shpr3 = ((prio << (8U - NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << 24;
    } else {
        scb_s->shpr3 = ((prio << (8U - NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << 24;
    }
}

static inline uint32_t nvic_get_prio(struct nvic* ic, irqid_t int_id)
{
    irqid_t linear_id = nvic_linearize_id(int_id);
    return (ic->ipr[linear_id] >> (8U - NVIC_PRIO_BITS));
}

static inline uint32_t systick_get_prio(struct systick* st)
{
    if (st == systick_ns) {
        return ((scb_ns->shpr3 >> 24) >> (8U - NVIC_PRIO_BITS));
    } else {
        return ((scb_s->shpr3 >> 24) >> (8U - NVIC_PRIO_BITS));
    }
}

void nvic_init(void);
void nvic_int_handle(void);
bool nvic_any_act_irq(struct nvic* ic);

#endif /* NVIC_H */
