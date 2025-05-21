#ifndef SPMP_H
#define SPMP_H

#include <crossconhyp.h>
#include <arch/csrs.h>
#include <bitmap.h>
#include <list.h>
#include <arch/spmp.h>

#define SPMP_MAX_NUM_ENTRIES 64

typedef union {
    struct {
        uint8_t r : 1;
        uint8_t w : 1;
        uint8_t x : 1;
        uint8_t a : 2;
        uint8_t res : 2;
        uint8_t s : 1;
    };

    uint8_t raw;
} spmp_cfg_t;

typedef spmp_cfg_t mem_flags_t;

struct spmp {
    bool active;

    priv_t priv;

    // The maximum number of spmp entries is 64, thus 1 bit per entry
    uint64_t alloc_entries;
    uint64_t switchmsk;
    uint64_t locked;

    struct {
        spmp_cfg_t cfg;
        unsigned long addr;
    } entry[SPMP_MAX_ENTRIES];

    struct {
        struct list list;
        struct spmp_node {
            node_t node;
            mpid_t mpid;
        } node[SPMP_MAX_ENTRIES];
    } order;
};

void spmp_enable(void);
void spmp_init(struct spmp* spmp, priv_t priv);
static inline void spmp_set_active(struct spmp* spmp, bool active)
{
    spmp->active = active;
}

void spmp_restore(struct spmp* spmp);
void spmp_enable_hyp_whitelist_mode(void);

struct addr_space;
struct mp_region;

bool spmp_update(struct addr_space* as, struct mp_region* mpr);
bool spmp_perms_compatible(mem_flags_t perms1, mem_flags_t perms2);
#endif /* SPMP_H */
