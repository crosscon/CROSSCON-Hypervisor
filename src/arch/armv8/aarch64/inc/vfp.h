#ifndef ARCH_VFP_H
#define ARCH_VFP_H

#include <types.h>

#define VFP_NUM_REGS    (31)

struct vfp_reg {
    uint64_t d0;
    uint64_t d1;
} __attribute__((aligned(16)));

struct vfp {
    struct vfp_reg q[VFP_NUM_REGS];
    uint64_t fpcr;
    uint64_t fpexc32_el2;
    uint64_t fpsr;
};

void vfp_reset(struct vfp* vfp);
void vfp_save_state(struct vfp* vfp);
void vfp_restore_state(struct vfp* vfp);

#endif /* ARCH_VFP_H */
