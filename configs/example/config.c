#include <config.h>

struct vm_config freertos = {
    .image = VM_IMAGE_LOADED(0x20000, 0x20000, 0xd00),
    .entry = 0x00020000,

    .type = 0,

    .platform = {
        .cpu_num = 1,
        .region_num = 4,
        .regions =  (struct vm_mem_region[]) {
                {
                    .base = 0x20010000, //SRAM1
                    .size = 0x7000
                },
                {
                    .base = 0x00020000,
                    .size = 0x18000
                },
                {
                    .base = 0x40013000,
                    .size = 0x00040000
                },
                {
                    /**< 192MHz Free Running OScillator (FRO) Control register, offset: 0x10 */
                    .base = 0x9FC00u,
                    .size = 0x400
                }
                // {
                //     /* Power management */
                //     .base = 0x40020000,
                //     .size = 0x1000
                // },
                // {
                //     /**< 192MHz Free Running OScillator (FRO) Control register, offset: 0x10 */
                //     .base = 0x40013000,
                //     .size = 0x100
                // },
                // {
                //     /**< 192MHz Free Running OScillator (FRO) Control register, offset: 0x10 */
                //     .base = 0x9FC00u,
                //     .size = 0x400
                // },
                // {
                //     /**< 192MHz Free Running OScillator (FRO) Control register, offset: 0x10 */
                //     .base = 40034000,
                //     .size = 0x1000
                // }
                },
        .ipc_num = 1,
        .ipcs = (struct ipc[]) {
            {
                .base = 0x20017000,
                .size = 0x1000,
                .shmem_id = 0,
                .interrupt_num = 1,
                .interrupts = (irqid_t[]) {78}
            },
        },
        .dev_num = 2,
        .devs =  (struct vm_dev_region[]) {
            {
                /* Flexcomm Interface 2 (USART2) */
                .pa = 0x40088000,
                .va = 0x40088000,
                .size = 0x1000,
                .interrupt_num = 1,
                .interrupts = (irqid_t[]) {16+16}
            },
            {
                /* SYSCON + IOCON */
                .pa = 0x40000000,
                .va = 0x40000000,
                .size = 0x2000,
            }
        }
    }
};

struct config config = {

    CONFIG_HEADER
    .shmemlist_size = 1,
    .shmemlist = (struct shmem[]) {
        [0] = { .base = 0x20017000, .size = 0x1000,},
    },
    .vmlist_size = 1,
    .vmlist = (struct vm_config*[]) {
        &freertos
    }
};