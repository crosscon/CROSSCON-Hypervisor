#ifndef ARCH_VFP_H
#define ARCH_VFP_H

#include <crossconhyp.h>

#define VFP_NUM_REGS (32)

#if RV64
typedef uint64_t vfp_reg_t;
#else
typedef uint32_t vfp_reg_t;
#endif

struct vfp {
    vfp_reg_t f[VFP_NUM_REGS];
    uint32_t fcsr;
};

void vfp_reset(struct vfp* vfp);
void vfp_save_state(struct vfp* vfp);
void vfp_restore_state(struct vfp* vfp);

#endif /* ARCH_VFP_H */
