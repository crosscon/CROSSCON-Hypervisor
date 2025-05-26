
#include <sched.h>
#include <list.h>
#include <timer.h>
#include <objpool.h>
#include <config.h>
#include <vm.h>
#include <cpu.h>

const unsigned long long time_slice = TIME_MS(10);

void sched_init() { }

static inline timer_value_t sched_next_event_time(void)
{
    // hardcoded 10 ms time slice
    return (timer_value_t)(timer_arch_get_count() + time_slice);
}

static void sched_timer_event_handler(struct timer_event* timer_event);

static void sched_set_next_timer_event(void)
{
    struct timer_event* timer_event = &cpu()->sched.timer_event;
    timer_value_t timer = sched_next_event_time();
    timer_event_set(timer_event, timer, sched_timer_event_handler);
    timer_event_add(timer_event);
}

static void sched_next(void)
{
    node_t* next_node = list_pop(&cpu()->vcpu_sched_lst);
    list_push(&cpu()->vcpu_sched_lst, next_node);

    struct vcpu* next = CONTAINER_OF(struct vcpu, sched_node, next_node);

    if (next == NULL) {
        while (true) {
            cpu_powerdown();
        }
    }

    node_t* node = list_peek(&next->vcpu_stack_lst);
    next = CONTAINER_OF(struct vcpu, vmstack_node, node);

    cpu()->next_vcpu = next;
}

static void sched_timer_event_handler(struct timer_event* timer_event)
{
    (void)timer_event;
    sched_next();
    sched_set_next_timer_event();
}

void sched_yield(void)
{
    sched_timer_event_handler(NULL);
}

void sched_start(void)
{
    sched_next();
    if (list_size(&cpu()->vcpu_sched_lst) > 1) {
        sched_set_next_timer_event();
    }
}
