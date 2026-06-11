#ifndef ARCH_VFPU_H
#define ARCH_VFPU_H

#include <crossconhyp.h>

#define VFPU_NUM_REGS (32)

#if RV64
typedef uint64_t vfpu_reg_t;
#else
typedef uint32_t vfpu_reg_t;
#endif

struct vfpu {
    vfpu_reg_t f[VFPU_NUM_REGS];
    uint32_t fcsr;
};

struct vcpu;

void vfpu_reset(struct vcpu* vcpu);
void vfpu_save_state(struct vcpu* vcpu);
void vfpu_restore_state(struct vcpu* vcpu);

#endif /* ARCH_VFPU_H */
