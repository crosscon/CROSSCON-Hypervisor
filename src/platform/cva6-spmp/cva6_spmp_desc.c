#include <platform.h>
#include <interrupts.h>

struct platform platform = {

    .cpu_num = 1,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x80400000,
            .size = 0x40000000 - 0x200000,
            .perms = MEM_RWX,
        }
    },

    .console = {
        .base = 0x10000000,
    },

    .arch = {
        #if (IRQC == PLIC)
            .irqc.plic.base = 0xc000000,
        #else
            .irqc.aia.aplic.base = 0xd000000,
            .irqc.aia.imsic.base = 0x28000000,
            .irqc.aia.imsic.num_msis = 63,
            .irqc.aia.imsic.num_guest_files = 3,
        #endif
    },
};
