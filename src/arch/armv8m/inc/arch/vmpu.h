#ifndef VMPU_H
#define VMPU_H

#include <crossconhyp.h>
#include <platform.h>

#define MPU_MAX_REGIONS PLAT_MAX_MPU_REGIONS

struct vmpu {
    uint32_t ctrl;
    uint32_t rnr;
    uint32_t rbar[MPU_MAX_REGIONS];
    uint32_t rlar[MPU_MAX_REGIONS];
    uint32_t mair0;
    uint32_t mair1;
};

struct vmpu;

void vmpu_init(struct vmpu* vmpu);
void vmpu_reset(struct vmpu* vmpu);
void vmpu_save_state(struct vmpu* vmpu);
void vmpu_restore_state(struct vmpu* vmpu);

#endif /* VMPU_H */
