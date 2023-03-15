#include <platform.h>

#define KB(x) ((x)*1024UL)
#define MB(x) (KB(x)*1024UL)
#define GB(x) (MB(x)*1024UL)

struct platform_desc platform = {

    .cpu_num = 1,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = GB(2) + MB(2), /* offset for SBI(?) */
            .size = GB(4) - MB(2)
        }
    },

    .arch = {
        .plic_base = 0xc000000,
    }

};
