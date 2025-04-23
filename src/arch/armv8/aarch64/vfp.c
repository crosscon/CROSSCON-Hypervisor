#include <vfp.h>
#include <string.h>
#include <arch/sysregs.h>

void vfp_reset(struct vfp* vfp) {
    memset(vfp->q, 0, sizeof(vfp->q));
    vfp->fpcr = 0;
    vfp->fpexc32_el2 = 0;
    vfp->fpsr = 0;
}

void vfp_save_state(struct vfp* vfp) {
    __asm__ volatile(
        "stp q0,  q1, [%0, (0 * 32)]\n\t"
        "stp q2,  q3, [%0, (1 * 32)]\n\t"
        "stp q4,  q5, [%0, (2 * 32)]\n\t"
        "stp q6,  q7, [%0, (3 * 32)]\n\t"
        "stp q8,  q9, [%0, (4 * 32)]\n\t"
        "stp q10, q11, [%0, (5 * 32)]\n\t"
        "stp q12, q13, [%0, (6 * 32)]\n\t"
        "stp q14, q15, [%0, (7 * 32)]\n\t"
        "stp q16, q17, [%0, (8 * 32)]\n\t"
        "stp q18, q19, [%0, (9 * 32)]\n\t"
        "stp q20, q21, [%0, (10 * 32)]\n\t"
        "stp q22, q23, [%0, (11 * 32)]\n\t"
        "stp q24, q25, [%0, (12 * 32)]\n\t"
        "stp q26, q27, [%0, (13 * 32)]\n\t"
        "stp q28, q29, [%0, (14 * 32)]\n\t"
        "stp q30, q31, [%0, (15 * 32)]\n\t"
        : : "r"(vfp->q)
    );
    
    vfp->fpcr = sysreg_fpcr_read();
    vfp->fpexc32_el2 = sysreg_fpexc32_el2_read();
    vfp->fpsr = sysreg_fpsr_read();
}


void vfp_restore_state(struct vfp* vfp) {
    __asm__ volatile(
        "ldp q0,  q1, [%0, (0 * 32)]\n\t"
        "ldp q2,  q3, [%0, (1 * 32)]\n\t"
        "ldp q4,  q5, [%0, (2 * 32)]\n\t"
        "ldp q6,  q7, [%0, (3 * 32)]\n\t"
        "ldp q8,  q9, [%0, (4 * 32)]\n\t"
        "ldp q10, q11, [%0, (5 * 32)]\n\t"
        "ldp q12, q13, [%0, (6 * 32)]\n\t"
        "ldp q14, q15, [%0, (7 * 32)]\n\t"
        "ldp q16, q17, [%0, (8 * 32)]\n\t"
        "ldp q18, q19, [%0, (9 * 32)]\n\t"
        "ldp q20, q21, [%0, (10 * 32)]\n\t"
        "ldp q22, q23, [%0, (11 * 32)]\n\t"
        "ldp q24, q25, [%0, (12 * 32)]\n\t"
        "ldp q26, q27, [%0, (13 * 32)]\n\t"
        "ldp q28, q29, [%0, (14 * 32)]\n\t"
        "ldp q30, q31, [%0, (15 * 32)]\n\t"
        : : "r"(vfp->q)
    );
    sysreg_fpcr_write(vfp->fpcr);
    sysreg_fpexc32_el2_write(vfp->fpexc32_el2);
    sysreg_fpsr_write(vfp->fpsr);
}
