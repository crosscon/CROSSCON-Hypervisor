/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_CSR_H__
#define __ARCH_CSR_H__

#include <crossconhyp.h>

#define CSR_SISELECT      0x150
#define CSR_SIREG         0x151
#define CSR_STOPEI        0x15c
#define CSR_STOPI         0xdb0
#define CSR_VSSTATUS      0x200
#define CSR_VSIE          0x204
#define CSR_VSTVEC        0x205
#define CSR_VSSCRATCH     0x240
#define CSR_VSEPC         0x241
#define CSR_VSCAUSE       0x242
#define CSR_VSTVAL        0x243
#define CSR_VSIP          0x244
#define CSR_VSISELECT     0x250
#define CSR_VSIREG        0x251
#define CSR_VSATP         0x280
/* Sstc Extension */
#define CSR_VSTIMECMP     0x24D
#define CSR_VSTIMECMPH    0x25D

#define CSR_HSTATUS       0x600
#define CSR_HEDELEG       0x602
#define CSR_HIDELEG       0x603
#define CSR_HIE           0x604
#define CSR_HTIMEDELTA    0x605
#define CSR_HTIMEDELTAH   0x615
#define CSR_HCOUNTEREN    0x606
#define CSR_HGEIE         0x607
#define CSR_HTVAL         0x643
#define CSR_HIP           0x644
#define CSR_HVIP          0x645
#define CSR_HTINST        0x64A
#define CSR_HGATP         0x680
#define CSR_HGEIP         0xE07
/* Hypervisor Configuration */
#define CSR_HENVCFG       0x60A
#define CSR_HENVCFGH      0x61A

#define TOPI_IID_SHIFT    (16)

/* Sstc Extension */
#define CSR_STIMECMP      0x14D
#define CSR_STIMECMPH     0x15D

#define STVEC_MODE_OFF    (0)
#define STVEC_MODE_LEN    (2)
#define STVEC_MODE_MSK    BIT_MASK(STVEC_MODE_OFF, STVEC_MODE_LEN)
#define STVEC_MODE_DIRECT (0)
#define STVEC_MODE_VECTRD (1)

#if (RV64)
#define SATP_MODE_OFF  (60)
#define SATP_MODE_DFLT SATP_MODE_39
#define SATP_ASID_OFF  (44)
#define SATP_ASID_LEN  (16)
#define HGATP_VMID_OFF (44)
#define HGATP_VMID_LEN (14)
#elif (RV32)
#define SATP_MODE_OFF  (31)
#define SATP_MODE_DFLT SATP_MODE_32
#define SATP_ASID_OFF  (22)
#define SATP_ASID_LEN  (9)
#define HGATP_VMID_OFF (22)
#define HGATP_VMID_LEN (7)
#endif

#define SATP_MODE_BARE              (0UL << SATP_MODE_OFF)
#define SATP_MODE_32                (1UL << SATP_MODE_OFF)
#define SATP_MODE_39                (8UL << SATP_MODE_OFF)
#define SATP_MODE_48                (9UL << SATP_MODE_OFF)
#define SATP_ASID_MSK               BIT_MASK(SATP_ASID_OFF, SATP_ASID_LEN)

#define CSR_SPMPCFG0                0x500
#define CSR_SPMPCFG1                0x501
#define CSR_SPMPCFG2                0x502
#define CSR_SPMPCFG3                0x503
#define CSR_SPMPCFG4                0x504
#define CSR_SPMPCFG5                0x505
#define CSR_SPMPCFG6                0x506
#define CSR_SPMPCFG7                0x507
#define CSR_SPMPCFG8                0x508
#define CSR_SPMPCFG9                0x509
#define CSR_SPMPCFG10               0x50a
#define CSR_SPMPCFG11               0x50b
#define CSR_SPMPCFG12               0x50c
#define CSR_SPMPCFG13               0x50d
#define CSR_SPMPCFG14               0x50e
#define CSR_SPMPCFG15               0x50f
#define CSR_SPMPADDR0               0x510
#define CSR_SPMPADDR1               0x511
#define CSR_SPMPADDR2               0x512
#define CSR_SPMPADDR3               0x513
#define CSR_SPMPADDR4               0x514
#define CSR_SPMPADDR5               0x515
#define CSR_SPMPADDR6               0x516
#define CSR_SPMPADDR7               0x517
#define CSR_SPMPADDR8               0x518
#define CSR_SPMPADDR9               0x519
#define CSR_SPMPADDR10              0x51a
#define CSR_SPMPADDR11              0x51b
#define CSR_SPMPADDR12              0x51c
#define CSR_SPMPADDR13              0x51d
#define CSR_SPMPADDR14              0x51e
#define CSR_SPMPADDR15              0x51f
#define CSR_SPMPADDR16              0x520
#define CSR_SPMPADDR17              0x521
#define CSR_SPMPADDR18              0x522
#define CSR_SPMPADDR19              0x523
#define CSR_SPMPADDR20              0x524
#define CSR_SPMPADDR21              0x525
#define CSR_SPMPADDR22              0x526
#define CSR_SPMPADDR23              0x527
#define CSR_SPMPADDR24              0x528
#define CSR_SPMPADDR25              0x529
#define CSR_SPMPADDR26              0x52a
#define CSR_SPMPADDR27              0x52b
#define CSR_SPMPADDR28              0x52c
#define CSR_SPMPADDR29              0x52d
#define CSR_SPMPADDR30              0x52e
#define CSR_SPMPADDR31              0x52f
#define CSR_SPMPADDR32              0x530
#define CSR_SPMPADDR33              0x531
#define CSR_SPMPADDR34              0x532
#define CSR_SPMPADDR35              0x533
#define CSR_SPMPADDR36              0x534
#define CSR_SPMPADDR37              0x535
#define CSR_SPMPADDR38              0x536
#define CSR_SPMPADDR39              0x537
#define CSR_SPMPADDR40              0x538
#define CSR_SPMPADDR41              0x539
#define CSR_SPMPADDR42              0x53a
#define CSR_SPMPADDR43              0x53b
#define CSR_SPMPADDR44              0x53c
#define CSR_SPMPADDR45              0x53d
#define CSR_SPMPADDR46              0x53e
#define CSR_SPMPADDR47              0x53f
#define CSR_SPMPADDR48              0x540
#define CSR_SPMPADDR49              0x541
#define CSR_SPMPADDR50              0x542
#define CSR_SPMPADDR51              0x543
#define CSR_SPMPADDR52              0x544
#define CSR_SPMPADDR53              0x545
#define CSR_SPMPADDR54              0x546
#define CSR_SPMPADDR55              0x547
#define CSR_SPMPADDR56              0x548
#define CSR_SPMPADDR57              0x549
#define CSR_SPMPADDR58              0x54a
#define CSR_SPMPADDR59              0x54b
#define CSR_SPMPADDR60              0x54c
#define CSR_SPMPADDR61              0x54d
#define CSR_SPMPADDR62              0x54e
#define CSR_SPMPADDR63              0x54f

#define CSR_SPMPSWITCH0             0x550
#define CSR_SPMPSWITCH1             0x551

#define SPMPCFG_R_BIT               0x01
#define SPMPCFG_W_BIT               0x02
#define SPMPCFG_X_BIT               0x04
#define SPMPCFG_A_OFF               0
#define SPMPCFG_A_TOR               1
#define SPMPCFG_A_NA4               2
#define SPMPCFG_A_NAPOT             3
#define SPMPCFG_S_BIT               0x80
#define SPMPCFG_ICFG_OFF            8
#define SPMPCFG_ENTRIES_PER_CFG     (8)
#define SPMPCFG_ICFG_MSK            ((1UL << SPMPCFG_ICFG_OFF) - 1)
#define SPMP_MAX_ENTRIES            (64)

#define SPMPCFG_TOR                 0x08
#define SPMPCFG_NA4                 0x10
#define SPMPCFG_NAPOT               0x18

#define HGATP_MODE_OFF              SATP_MODE_OFF
#define HGATP_MODE_DFLT             SATP_MODE_DFLT
#define HGATP_VMID_MSK              BIT_MASK(HGATP_VMID_OFF, HGATP_VMID_LEN)

#define SSTATUS_UIE_BIT             (1UL << 0)
#define SSTATUS_SIE_BIT             (1UL << 1)
#define SSTATUS_UPIE_BIT            (1UL << 4)
#define SSTATUS_SPIE_BIT            (1UL << 5)
#define SSTATUS_SPP_BIT             (1UL << 8)
#define SSTATUS_FS_OFF              (13)
#define SSTATUS_FS_LEN              (2)
#define SSTATUS_FS_MSK              BIT_MASK(SSTATUS_FS_OFF, SSTATUS_FS_LEN)
#define SSTATUS_FS_AOFF             (0)
#define SSTATUS_FS_INITIAL          (1UL << SSTATUS_FS_OFF)
#define SSTATUS_FS_CLEAN            (2UL << SSTATUS_FS_OFF)
#define SSTATUS_FS_DIRTY            (3UL << SSTATUS_FS_OFF)
#define SSTATUS_XS_OFF              (15)
#define SSTATUS_XS_LEN              (2)
#define SSTATUS_XS_MSK              BIT_MASK(SSTATUS_XS_OFF, SSTATUS_XS_LEN)
#define SSTATUS_XS_AOFF             (0)
#define SSTATUS_XS_INITIAL          (1UL << SSTATUS_XS_OFF)
#define SSTATUS_XS_CLEAN            (2UL << SSTATUS_XS_OFF)
#define SSTATUS_XS_DIRTY            (3UL << SSTATUS_XS_OFF)
#define SSTATUS_SUM                 (1UL << 18)
#define SSTATUS_MXR                 (1UL << 19)
#define SSTATUS_SD                  (1UL << ((REGLEN * 8) - 1))

#define SIE_USIE                    (1UL << 0)
#define SIE_SSIE                    (1UL << 1)
#define SIE_UTIE                    (1UL << 4)
#define SIE_STIE                    (1UL << 5)
#define SIE_UEIE                    (1UL << 8)
#define SIE_SEIE                    (1UL << 9)

#define SIP_USIP                    SIE_USIE
#define SIP_SSIP                    SIE_SSIE
#define SIP_UTIP                    SIE_UTIE
#define SIP_STIP                    SIE_STIE
#define SIP_UEIP                    SIE_UEIE
#define SIP_SEIP                    SIE_SEIE

#define HIE_VSSIE                   (1UL << 2)
#define HIE_VSTIE                   (1UL << 6)
#define HIE_VSEIE                   (1UL << 10)
#define HIE_SGEIE                   (1UL << 12)

#define HIP_VSSIP                   HIE_VSSIE
#define HIP_VSTIP                   HIE_VSTIE
#define HIP_VSEIP                   HIE_VSEIE
#define HIP_SGEIP                   HIE_SGEIE

#define IRQ_S_SOFT                  (1)
#define IRQ_S_TIMER                 (5)
#define IRQ_S_EXT                   (9)
#define SCAUSE_INT_BIT              (1UL << ((REGLEN * 8) - 1))
#define SCAUSE_CODE_MSK             (SCAUSE_INT_BIT - 1)
#define SCAUSE_CODE_USI             (0 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_SSI             (IRQ_S_SOFT | SCAUSE_INT_BIT)
#define SCAUSE_CODE_VSSI            (2 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_UTI             (4 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_STI             (IRQ_S_TIMER | SCAUSE_INT_BIT)
#define SCAUSE_CODE_VSTI            (6 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_UEI             (8 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_SEI             (IRQ_S_EXT | SCAUSE_INT_BIT)
#define SCAUSE_CODE_VSEI            (10 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_IAM             (0)
#define SCAUSE_CODE_IAF             (1)
#define SCAUSE_CODE_ILI             (2)
#define SCAUSE_CODE_BKP             (3)
#define SCAUSE_CODE_LAM             (4)
#define SCAUSE_CODE_LAF             (5)
#define SCAUSE_CODE_SAM             (6)
#define SCAUSE_CODE_SAF             (7)
#define SCAUSE_CODE_ECU             (8)
#define SCAUSE_CODE_ECS             (9)
#define SCAUSE_CODE_ECV             (10)
#define SCAUSE_CODE_IPF             (12)
#define SCAUSE_CODE_LPF             (13)
#define SCAUSE_CODE_SPF             (15)
#define SCAUSE_CODE_IGPF            (20)
#define SCAUSE_CODE_LGPF            (21)
#define SCAUSE_CODE_VRTI            (22)
#define SCAUSE_CODE_SGPF            (23)

#define HIDELEG_USI                 SIP_USIP
#define HIDELEG_SSI                 SIP_SSIP
#define HIDELEG_UTI                 SIP_UTIP
#define HIDELEG_STI                 SIP_STIP
#define HIDELEG_UEI                 SIP_UEIP
#define HIDELEG_SEI                 SIP_SEIP
#define HIDELEG_VSSI                HIP_VSSIP
#define HIDELEG_VSTI                HIP_VSTIP
#define HIDELEG_VSEI                HIP_VSEIP
#define HIDELEG_SGEI                HIP_SGEIP

#define HEDELEG_IAM                 (1ULL << 0)
#define HEDELEG_IAF                 (1ULL << 1)
#define HEDELEG_ILI                 (1ULL << 2)
#define HEDELEG_BKP                 (1ULL << 3)
#define HEDELEG_LAM                 (1ULL << 4)
#define HEDELEG_LAF                 (1ULL << 5)
#define HEDELEG_SAM                 (1ULL << 6)
#define HEDELEG_SAF                 (1ULL << 7)
#define HEDELEG_ECU                 (1ULL << 8)
#define HEDELEG_ECS                 (1ULL << 9)
#define HEDELEG_ECV                 (1ULL << 10)
#define HEDELEG_IPF                 (1ULL << 12)
#define HEDELEG_LPF                 (1ULL << 13)
#define HEDELEG_SPF                 (1ULL << 15)

#define MISA_H                      (1ULL << 7)

#define HSTATUS_VSBE                (1UL << 5)
#define HSTATUS_GVA                 (1UL << 6)
#define HSTATUS_SPV                 (1UL << 7)
#define HSTATUS_SPVP                (1UL << 8)
#define HSTATUS_HU                  (1UL << 9)
#define HSTATUS_VGEIN_OFF           (12)
#define HSTATUS_VGEIN_LEN           (12)
#define HSTATUS_VGEIN_MSK           (BIT_MASK(HSTATUS_VGEIN_OFF, HSTATUS_VGEIN_LEN))
#define HSTATUS_VTVM                (1UL << 20)
#define HSTATUS_VTW                 (1UL << 21)
#define HSTATUS_VTSR                (1UL << 22)
#define HSTATUS_VSXL_OFF            (32)
#define HSTATUS_VSXL_LEN            (2)
#define HSTATUS_VSXL_MSK            (BIT_MASK(HSTATUS_VSXL_OFF, HSTATUS_VSXL_LEN))
#define HSTATUS_VSXL_32             (1UL << HSTATUS_VSXL_OFF)
#define HSTATUS_VSXL_64             (2UL << HSTATUS_VSXL_OFF)

#define HENVCFG_FIOM                (1ULL << 0)
#define HENVCFG_CBIE_OFF            (4)
#define HSTATUS_CBIE_LEN            (2)
#define HSTATUS_CBIE_MSK            (BIT_MASK(HSTATUS_CBIE_OFF, HSTATUS_CBIE_LEN))
#define HENVCFG_CBCFE               (1ULL << 6)
#define HENVCFG_CBZE                (1ULL << 7)
#define HENVCFG_PBMTE               (1ULL << 62)
#define HENVCFG_STCE                (1ULL << 63)

#define HCOUNTEREN_CY               (1ULL << 0)
#define HCOUNTEREN_TM               (1ULL << 1)
#define HCOUNTEREN_IR               (1ULL << 2)

#define TINST_PSEUDO_STORE          (0x3020)
#define TINST_PSEUDO_LOAD           (0x3000)
#define TINST_INS_COMPRESSED(tinst) (!((tinst) & 0x2))
#define TINST_INS_SIZE(tinst)       (TINST_INS_COMPRESSED(tinst) ? 2 : 4)

#ifndef __ASSEMBLER__

#define CSRS_GEN_ACCESSORS_NAMED(csr_name, csr_id)                                        \
    static inline unsigned long csrs_##csr_name##_read(void)                              \
    {                                                                                     \
        unsigned long csr_value;                                                          \
        __asm__ volatile("csrr %0," XSTR(csr_id) : "=r"(csr_value)::"memory");            \
        return csr_value;                                                                 \
    }                                                                                     \
    static inline void csrs_##csr_name##_write(unsigned long csr_value)                   \
    {                                                                                     \
        __asm__ volatile("csrw " XSTR(csr_id) ",%0" ::"r"(csr_value) : "memory");         \
    }                                                                                     \
    static inline void csrs_##csr_name##_set(unsigned long csr_value)                     \
    {                                                                                     \
        __asm__ volatile("csrs " XSTR(csr_id) ",%0" ::"r"(csr_value) : "memory");         \
    }                                                                                     \
    static inline void csrs_##csr_name##_clear(unsigned long csr_value)                   \
    {                                                                                     \
        __asm__ volatile("csrc " XSTR(csr_id) ",%0" ::"r"(csr_value) : "memory");         \
    }                                                                                     \
    static inline unsigned long csrs_##csr_name##_swap(unsigned long csr_value)           \
    {                                                                                     \
        unsigned long reg_val;                                                            \
        __asm__ volatile("csrrw %0," XSTR(csr_id) ",%1" : "=r"(reg_val) : "rK"(csr_value) \
                         : "memory");                                                     \
        return reg_val;                                                                   \
    }

#define CSRS_GEN_ACCESSORS(csr) CSRS_GEN_ACCESSORS_NAMED(csr, csr)

#define CSRS_GEN_ACCESSORS_MERGED(csr_name, csrl, csrh)                                 \
    static inline unsigned long long csrs_##csr_name##_read(void)                       \
    {                                                                                   \
        return ((unsigned long long)csrs_##csrh##_read() << 32) | csrs_##csrl##_read(); \
    }                                                                                   \
    static inline void csrs_##csr_name##_write(unsigned long long csr_value)            \
    {                                                                                   \
        csrs_##csrl##_write((unsigned long)csr_value);                                  \
        csrs_##csrh##_write((unsigned long)(csr_value >> 32));                          \
    }                                                                                   \
    static inline void csrs_##csr_name##_set(unsigned long long csr_value)              \
    {                                                                                   \
        csrs_##csrl##_set((unsigned long)csr_value);                                    \
        csrs_##csrh##_set((unsigned long)(csr_value >> 32));                            \
    }                                                                                   \
    static inline void csrs_##csr_name##_clear(unsigned long long csr_value)            \
    {                                                                                   \
        csrs_##csrl##_clear((unsigned long)csr_value);                                  \
        csrs_##csrh##_clear((unsigned long)(csr_value >> 32));                          \
    }

CSRS_GEN_ACCESSORS(sstatus)
CSRS_GEN_ACCESSORS(sscratch)
CSRS_GEN_ACCESSORS(scause)
CSRS_GEN_ACCESSORS(stval)
CSRS_GEN_ACCESSORS(sepc)
CSRS_GEN_ACCESSORS(sie)
CSRS_GEN_ACCESSORS(sip)
CSRS_GEN_ACCESSORS(satp)
CSRS_GEN_ACCESSORS(time)
CSRS_GEN_ACCESSORS(fcsr)
CSRS_GEN_ACCESSORS_NAMED(hstatus, CSR_HSTATUS)
CSRS_GEN_ACCESSORS_NAMED(hgatp, CSR_HGATP)
CSRS_GEN_ACCESSORS_NAMED(htinst, CSR_HTINST)
CSRS_GEN_ACCESSORS_NAMED(htval, CSR_HTVAL)
CSRS_GEN_ACCESSORS_NAMED(hideleg, CSR_HIDELEG)
CSRS_GEN_ACCESSORS_NAMED(hedeleg, CSR_HEDELEG)
CSRS_GEN_ACCESSORS_NAMED(hcounteren, CSR_HCOUNTEREN)
CSRS_GEN_ACCESSORS_NAMED(vsstatus, CSR_VSSTATUS)
CSRS_GEN_ACCESSORS_NAMED(vstvec, CSR_VSTVEC)
CSRS_GEN_ACCESSORS_NAMED(vsscratch, CSR_VSSCRATCH)
CSRS_GEN_ACCESSORS_NAMED(vsepc, CSR_VSEPC)
CSRS_GEN_ACCESSORS_NAMED(vscause, CSR_VSCAUSE)
CSRS_GEN_ACCESSORS_NAMED(vstval, CSR_VSTVAL)
CSRS_GEN_ACCESSORS_NAMED(vsatp, CSR_VSATP)
CSRS_GEN_ACCESSORS_NAMED(hvip, CSR_HVIP)
CSRS_GEN_ACCESSORS_NAMED(vsie, CSR_VSIE)
CSRS_GEN_ACCESSORS_NAMED(hie, CSR_HIE)
CSRS_GEN_ACCESSORS_NAMED(siselect, CSR_SISELECT)
CSRS_GEN_ACCESSORS_NAMED(sireg, CSR_SIREG)
CSRS_GEN_ACCESSORS_NAMED(vsiselect, CSR_VSISELECT)
CSRS_GEN_ACCESSORS_NAMED(vsireg, CSR_VSIREG)
CSRS_GEN_ACCESSORS_NAMED(stopei, CSR_STOPEI)
CSRS_GEN_ACCESSORS_NAMED(stopi, CSR_STOPI)

#if (RV64)
CSRS_GEN_ACCESSORS_NAMED(stimecmp, CSR_STIMECMP)
CSRS_GEN_ACCESSORS_NAMED(vstimecmp, CSR_VSTIMECMP)
CSRS_GEN_ACCESSORS_NAMED(henvcfg, CSR_HENVCFG)
CSRS_GEN_ACCESSORS_NAMED(htimedelta, CSR_HTIMEDELTA)
CSRS_GEN_ACCESSORS_NAMED(spmpswitch, CSR_SPMPSWITCH0)
#else
CSRS_GEN_ACCESSORS_NAMED(henvcfgl, CSR_HENVCFG)
CSRS_GEN_ACCESSORS_NAMED(henvcfgh, CSR_HENVCFGH)
CSRS_GEN_ACCESSORS_MERGED(henvcfg, henvcfgl, henvcfgh)

CSRS_GEN_ACCESSORS_NAMED(htimedeltal, CSR_HTIMEDELTA)
CSRS_GEN_ACCESSORS_NAMED(htimedeltah, CSR_HTIMEDELTAH)
CSRS_GEN_ACCESSORS_MERGED(htimedelta, htimedeltal, htimedeltah)

CSRS_GEN_ACCESSORS_NAMED(stimecmpl, CSR_STIMECMP)
CSRS_GEN_ACCESSORS_NAMED(stimecmph, CSR_STIMECMPH)
CSRS_GEN_ACCESSORS_MERGED(stimecmp, stimecmpl, stimecmph)

CSRS_GEN_ACCESSORS_NAMED(vstimecmpl, CSR_VSTIMECMP)
CSRS_GEN_ACCESSORS_NAMED(vstimecmph, CSR_VSTIMECMPH)
CSRS_GEN_ACCESSORS_MERGED(vstimecmp, vstimecmpl, vstimecmph)

CSRS_GEN_ACCESSORS_NAMED(spmpswitch0, CSR_SPMPSWITCH0)
CSRS_GEN_ACCESSORS_NAMED(spmpswitch1, CSR_SPMPSWITCH1)
CSRS_GEN_ACCESSORS_MERGED(spmpswitch, spmpswitch0, spmpswitch1)
#endif

CSRS_GEN_ACCESSORS_NAMED(spmpcfg0, CSR_SPMPCFG0)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg1, CSR_SPMPCFG1)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg2, CSR_SPMPCFG2)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg3, CSR_SPMPCFG3)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg4, CSR_SPMPCFG4)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg5, CSR_SPMPCFG5)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg6, CSR_SPMPCFG6)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg7, CSR_SPMPCFG7)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg8, CSR_SPMPCFG8)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg9, CSR_SPMPCFG9)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg10, CSR_SPMPCFG10)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg11, CSR_SPMPCFG11)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg12, CSR_SPMPCFG12)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg13, CSR_SPMPCFG13)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg14, CSR_SPMPCFG14)
CSRS_GEN_ACCESSORS_NAMED(spmpcfg15, CSR_SPMPCFG15)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr0, CSR_SPMPADDR0)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr1, CSR_SPMPADDR1)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr2, CSR_SPMPADDR2)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr3, CSR_SPMPADDR3)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr4, CSR_SPMPADDR4)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr5, CSR_SPMPADDR5)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr6, CSR_SPMPADDR6)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr7, CSR_SPMPADDR7)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr8, CSR_SPMPADDR8)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr9, CSR_SPMPADDR9)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr10, CSR_SPMPADDR10)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr11, CSR_SPMPADDR11)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr12, CSR_SPMPADDR12)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr13, CSR_SPMPADDR13)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr14, CSR_SPMPADDR14)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr15, CSR_SPMPADDR15)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr16, CSR_SPMPADDR16)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr17, CSR_SPMPADDR17)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr18, CSR_SPMPADDR18)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr19, CSR_SPMPADDR19)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr20, CSR_SPMPADDR20)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr21, CSR_SPMPADDR21)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr22, CSR_SPMPADDR22)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr23, CSR_SPMPADDR23)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr24, CSR_SPMPADDR24)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr25, CSR_SPMPADDR25)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr26, CSR_SPMPADDR26)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr27, CSR_SPMPADDR27)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr28, CSR_SPMPADDR28)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr29, CSR_SPMPADDR29)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr30, CSR_SPMPADDR30)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr31, CSR_SPMPADDR31)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr32, CSR_SPMPADDR32)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr33, CSR_SPMPADDR33)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr34, CSR_SPMPADDR34)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr35, CSR_SPMPADDR35)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr36, CSR_SPMPADDR36)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr37, CSR_SPMPADDR37)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr38, CSR_SPMPADDR38)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr39, CSR_SPMPADDR39)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr40, CSR_SPMPADDR40)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr41, CSR_SPMPADDR41)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr42, CSR_SPMPADDR42)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr43, CSR_SPMPADDR43)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr44, CSR_SPMPADDR44)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr45, CSR_SPMPADDR45)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr46, CSR_SPMPADDR46)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr47, CSR_SPMPADDR47)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr48, CSR_SPMPADDR48)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr49, CSR_SPMPADDR49)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr50, CSR_SPMPADDR50)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr51, CSR_SPMPADDR51)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr52, CSR_SPMPADDR52)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr53, CSR_SPMPADDR53)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr54, CSR_SPMPADDR54)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr55, CSR_SPMPADDR55)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr56, CSR_SPMPADDR56)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr57, CSR_SPMPADDR57)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr58, CSR_SPMPADDR58)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr59, CSR_SPMPADDR59)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr60, CSR_SPMPADDR60)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr61, CSR_SPMPADDR61)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr62, CSR_SPMPADDR62)
CSRS_GEN_ACCESSORS_NAMED(spmpaddr63, CSR_SPMPADDR63)

#endif /* __ASSEMBLER__ */

#endif /* __ARCH_CSRS_H__ */
