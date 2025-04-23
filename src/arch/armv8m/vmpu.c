#include <arch/vmpu.h>
#include <vm.h>
#include <string.h>
#include <arch/sysregs.h>

typedef unsigned long region_t;

void vmpu_init(struct vmpu* vmpu)
{
    UNUSED_ARG(vmpu);
}

void vmpu_reset(struct vmpu* vmpu)
{
    memset(vmpu, 0, sizeof(struct vmpu));
}

static inline void vmpu_save_region(struct vmpu* vmpu, region_t rgn)
{
    mpu_ns->rnr = rgn;
    vmpu->rbar[rgn] = mpu_ns->rbar;
    vmpu->rlar[rgn] = mpu_ns->rlar;
}

static inline void vmpu_restore_region(struct vmpu* vmpu, region_t rgn)
{
    mpu_ns->rnr = rgn;
    mpu_ns->rbar = vmpu->rbar[rgn];
    mpu_ns->rlar = vmpu->rlar[rgn];
}

void vmpu_save_state(struct vmpu* vmpu)
{
    vmpu->ctrl = mpu_ns->ctrl;
    vmpu->rnr = mpu_ns->rnr;

    for (region_t rgn = 0; rgn < MPU_MAX_REGIONS; rgn++) {
        vmpu_save_region(vmpu, rgn);
    }

    vmpu->mair0 = mpu_ns->mair0;
    vmpu->mair1 = mpu_ns->mair1;
}

void vmpu_restore_state(struct vmpu* vmpu)
{
    mpu_ns->ctrl = vmpu->ctrl;

    for (region_t rgn = 0; rgn < MPU_MAX_REGIONS; rgn++) {
        vmpu_restore_region(vmpu, rgn);
    }

    mpu_ns->rnr = vmpu->rnr;
    mpu_ns->mair0 = vmpu->mair0;
    mpu_ns->mair1 = vmpu->mair1;
}
