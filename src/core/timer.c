
#include <timer.h>
#include <util.h>
#include <cpu.h>
#include <interrupts.h>

static inline struct list* timer_cpu_list(void)
{
    return &cpu()->timer_event_list;
}

static int timer_event_compare(void* cookie, node_t* n1, node_t* n2)
{
    UNUSED_ARG(cookie);
    struct timer_event* te1 = CONTAINER_OF(struct timer_event, node, n1);
    struct timer_event* te2 = CONTAINER_OF(struct timer_event, node, n2);
    return (te1->timer == te2->timer) ? 0 : (te1->timer > te2->timer) ? 1 : -1;
}

static void timer_set_next_event(void)
{
    node_t* next_node = list_peek(timer_cpu_list());
    if (next_node != NULL) {
        struct timer_event* next_event = CONTAINER_OF(struct timer_event, node, next_node);
        timer_arch_set(next_event->timer);
    } else {
        timer_disable();
    }
}

static void timer_irq_handler(irqid_t int_id)
{
    UNUSED_ARG(int_id);
    node_t* next_node = NULL;

    // TODO:ARMV8M - This needs to be fixed for all architectures (code was removed ). In this
    // architectures the timer works as a systick timer instead of a event timer.
    next_node = list_pop(timer_cpu_list());
    struct timer_event* next_event = CONTAINER_OF(struct timer_event, node, next_node);

    if (next_event->handler != NULL) {
        next_event->handler(next_event);
    }

    timer_set_next_event();
}

void timer_event_add(struct timer_event* event)
{
    static const struct node_cmp timer_list_node_cmp = { .cmp = timer_event_compare };
    list_insert_ordered(timer_cpu_list(), &event->node, &timer_list_node_cmp);
    timer_set_next_event();
}

void timer_event_remove(struct timer_event* event)
{
    list_rm(timer_cpu_list(), &event->node);
    timer_set_next_event();
}

void timer_init()
{
    list_init(timer_cpu_list());
    timer_arch_init();
    interrupts_reserve(timer_arch_irq_id(), timer_irq_handler);
    interrupts_cpu_enable(timer_arch_irq_id(), true);
}
