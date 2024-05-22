#include <platform.h>

struct platform_desc platform = {

    .cpu_num = 2,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x80200000,
            .size = 0x100000000 - 0x200000 /* almost 4GB */
        }
    },

    .arch = {
        .plic_base = 0xc000000,
    }

};
