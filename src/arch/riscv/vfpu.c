
#include <arch/vfpu.h>
#include <vm.h>
#include <string.h>
#include <arch/csrs.h>
#include <cpu.h>
#include <platform.h>

#if defined(RV32)
#if CPU_HAS_EXTENSION(CPU_EXT_D)
#define LOADFP  fld
#define STOREFP fsd
#define FREGLEN (8)
#else
#define LOADFP  flw
#define STOREFP fsw
#define FREGLEN (4)
 #endif
#elif(RV64)
#define LOADFP  fld
#define STOREFP fsd
#define FREGLEN (8)
#endif



void vfpu_reset(struct vcpu* vcpu)
{
    memset(vcpu->regs.vfpu.f, 0, sizeof(vcpu->regs.vfpu.f));
    vcpu->regs.vfpu.fcsr = 0;
}

void vfpu_save_state(struct vcpu* vcpu)
{
    bool vfpu_state_is_dirty = (vcpu->regs.sstatus & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY;
    if (vfpu_state_is_dirty) {
        __asm__ volatile(
            XSTR(STOREFP) " f0,  (" XSTR(FREGLEN) "*0)(%0)\n\t" XSTR(STOREFP) " f1,  (" XSTR(FREGLEN) "*1)(%0)\n\t" XSTR(STOREFP) " f2,  (" XSTR(FREGLEN) "*2)(%0)\n\t" XSTR(STOREFP) " f3,  (" XSTR(FREGLEN) "*3)(%0)\n\t" XSTR(STOREFP) " f4,  (" XSTR(FREGLEN) "*4)(%0)\n\t" XSTR(STOREFP) " f5,  (" XSTR(FREGLEN) "*5)(%0)\n\t" XSTR(STOREFP) " f6,  (" XSTR(FREGLEN) "*6)(%0)\n\t" XSTR(STOREFP) " f7,  (" XSTR(FREGLEN) "*7)(%0)\n\t" XSTR(STOREFP) " f8,  (" XSTR(FREGLEN) "*8)(%0)\n\t" XSTR(STOREFP) " f9,  (" XSTR(FREGLEN) "*9)(%0)\n\t" XSTR(STOREFP) " f10, (" XSTR(FREGLEN) "*10)(%0)\n\t" XSTR(STOREFP) " f11, (" XSTR(FREGLEN) "*11)(%0)\n\t" XSTR(STOREFP) " f12, (" XSTR(FREGLEN) "*12)(%0)\n\t" XSTR(STOREFP) " f13, (" XSTR(FREGLEN) "*13)(%0)\n\t" XSTR(STOREFP) " f14, (" XSTR(FREGLEN) "*14)(%0)\n\t" XSTR(STOREFP) " f15, (" XSTR(FREGLEN) "*15)(%0)\n\t" XSTR(
                STOREFP) " f16, (" XSTR(FREGLEN) "*16)(%0)\n\t" XSTR(STOREFP) " f17, (" XSTR(FREGLEN) "*17)(%0)\n\t" XSTR(STOREFP) " f18, (" XSTR(FREGLEN) "*18)(%0)\n\t" XSTR(STOREFP) " f19, (" XSTR(FREGLEN) "*19)(%0)\n\t" XSTR(STOREFP) " f20, (" XSTR(FREGLEN) "*20)(%0)\n\t" XSTR(STOREFP) " f21, (" XSTR(FREGLEN) "*21)(%0)\n\t" XSTR(STOREFP) " f22, (" XSTR(FREGLEN) "*22)(%0)\n\t" XSTR(STOREFP) " f23, (" XSTR(FREGLEN) "*23)(%0)\n\t" XSTR(STOREFP) " f24, (" XSTR(FREGLEN) "*24)(%0)\n\t" XSTR(STOREFP) " f25, (" XSTR(FREGLEN) "*25)(%0)\n\t" XSTR(STOREFP) " f26, (" XSTR(FREGLEN) "*26)(%0)\n\t" XSTR(STOREFP) " f27, (" XSTR(FREGLEN) "*27)(%0)\n\t" XSTR(STOREFP) " f28, (" XSTR(FREGLEN) "*28)(%0)\n\t" XSTR(STOREFP) " f29, (" XSTR(FREGLEN) "*29)(%0)\n\t" XSTR(STOREFP) " f30, (" XSTR(FREGLEN) "*30)(%0)\n\t" XSTR(STOREFP) " f31, (" XSTR(FREGLEN) "*31)(%0)\n\t"
            : : "r"(vcpu->regs.vfpu.f));

        vcpu->regs.vfpu.fcsr = (uint32_t)csrs_fcsr_read();

        vcpu->regs.sstatus &= ~SSTATUS_FS_MSK;
        vcpu->regs.sstatus |= SSTATUS_FS_CLEAN;
    }
}

void vfpu_restore_state(struct vcpu* vcpu)
{
    bool vfpu_state_is_init = (vcpu->regs.sstatus & SSTATUS_FS_MSK) == SSTATUS_FS_INITIAL;
    if (!(vfpu_state_is_init && (cpu()->arch.fpu_state == FPU_INIT))) {
        __asm__ volatile(
            XSTR(LOADFP) " f0,  (" XSTR(FREGLEN) "*0)(%0)\n\t" XSTR(LOADFP) " f1,  (" XSTR(FREGLEN) "*1)(%0)\n\t" XSTR(LOADFP) " f2,  (" XSTR(FREGLEN) "*2)(%0)\n\t" XSTR(LOADFP) " f3,  (" XSTR(FREGLEN) "*3)(%0)\n\t" XSTR(LOADFP) " f4,  (" XSTR(FREGLEN) "*4)(%0)\n\t" XSTR(LOADFP) " f5,  (" XSTR(FREGLEN) "*5)(%0)\n\t" XSTR(LOADFP) " f6,  (" XSTR(FREGLEN) "*6)(%0)\n\t" XSTR(LOADFP) " f7,  (" XSTR(FREGLEN) "*7)(%0)\n\t" XSTR(LOADFP) " f8,  (" XSTR(FREGLEN) "*8)(%0)\n\t" XSTR(LOADFP) " f9,  (" XSTR(FREGLEN) "*9)(%0)\n\t" XSTR(LOADFP) " f10, (" XSTR(FREGLEN) "*10)(%0)\n\t" XSTR(LOADFP) " f11, (" XSTR(FREGLEN) "*11)(%0)\n\t" XSTR(LOADFP) " f12, (" XSTR(FREGLEN) "*12)(%0)\n\t" XSTR(LOADFP) " f13, (" XSTR(FREGLEN) "*13)(%0)\n\t" XSTR(LOADFP) " f14, (" XSTR(FREGLEN) "*14)(%0)\n\t" XSTR(LOADFP) " f15, (" XSTR(FREGLEN) "*15)(%0)\n\t" XSTR(
                LOADFP) " f16, (" XSTR(FREGLEN) "*16)(%0)\n\t" XSTR(LOADFP) " f17, (" XSTR(FREGLEN) "*17)(%0)\n\t" XSTR(LOADFP) " f18, (" XSTR(FREGLEN) "*18)(%0)\n\t" XSTR(LOADFP) " f19, (" XSTR(FREGLEN) "*19)(%0)\n\t" XSTR(LOADFP) " f20, (" XSTR(FREGLEN) "*20)(%0)\n\t" XSTR(LOADFP) " f21, (" XSTR(FREGLEN) "*21)(%0)\n\t" XSTR(LOADFP) " f22, (" XSTR(FREGLEN) "*22)(%0)\n\t" XSTR(LOADFP) " f23, (" XSTR(FREGLEN) "*23)(%0)\n\t" XSTR(LOADFP) " f24, (" XSTR(FREGLEN) "*24)(%0)\n\t" XSTR(LOADFP) " f25, (" XSTR(FREGLEN) "*25)(%0)\n\t" XSTR(LOADFP) " f26, (" XSTR(FREGLEN) "*26)(%0)\n\t" XSTR(LOADFP) " f27, (" XSTR(FREGLEN) "*27)(%0)\n\t" XSTR(LOADFP) " f28, (" XSTR(FREGLEN) "*28)(%0)\n\t" XSTR(LOADFP) " f29, (" XSTR(FREGLEN) "*29)(%0)\n\t" XSTR(LOADFP) " f30, (" XSTR(FREGLEN) "*30)(%0)\n\t" XSTR(LOADFP) " f31, (" XSTR(FREGLEN) "*31)(%0)\n\t"
            : : "r"(vcpu->regs.vfpu.f));

        csrs_fcsr_write(vcpu->regs.vfpu.fcsr);
        cpu()->arch.fpu_state = vfpu_state_is_init ? FPU_INIT : FPU_DIRTY;
    }
}
