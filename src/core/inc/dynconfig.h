/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __DYN_CONFIG_H__
#define __DYN_CONFIG_H__

#ifndef GENERATING_DEFS

#include <crossconhyp.h>
#include <vm.h>
#include <config.h>

extern uint8_t _dynconfig_end, _images_end;

#define DYN_CONFIG_HEADER_SIZE ((size_t) & _dynconfig_end)
#define DYN_CONFIG_SIZE        ((size_t) & _images_end)

#define DYN_VM_IMAGE(img_name, img_path)                                                                                         \
    extern uint8_t _##img_name##_vm_size;                                                                                        \
    extern uint8_t _##img_name##_vm_beg;                                                                                         \
    __asm__(".pushsection .vm_image_" XSTR(                                                                                      \
        img_name) ", \"a\"\n\t"                                                                                                  \
                  ".global _" XSTR(                                                                                              \
                      img_name) "_vm_beg\n\t"                                                                                    \
                                "_" XSTR(                                                                                        \
                                    img_name) "_vm_beg:\n\t"                                                                     \
                                              ".incbin " XSTR(                                                                   \
                                                  img_path) "\n\t"                                                               \
                                                            "_" XSTR(                                                            \
                                                                img_name) "_vm_end:\n\t"                                         \
                                                                          ".global _" XSTR(                                      \
                                                                              img_name) "_vm_"                                   \
                                                                                        "size\n\t"                               \
                                                                                        ".set "                                  \
                                                                                        "_" XSTR(img_name) "_vm_size,  (_" XSTR( \
                                                                                            img_name) "_vm_end - _" #img_name    \
                                                                                                      "_vm_beg)\n\t"             \
                                                                                                      ".popsection");

#define DYN_VM_IMAGE_OFFSET(img_name) ((paddr_t) & _##img_name##_img_beg)
#define DYN_VM_IMAGE_SIZE(img_name)   ((size_t) & _##img_name##_img_size)

#define DYN_CONFIG_HEADER \
    .config_header_size = DYN_CONFIG_HEADER_SIZE, .config_size = DYN_CONFIG_SIZE,

struct dynconfig {
    /* The of this struct aligned to page size */
    size_t config_header_size;
    /* The size of the full configuration binary, including VM images */
    size_t config_size;

    struct vm_config vm_cfg;
};

void dynconfig_init(struct dynconfig* dynconfig, paddr_t load_addr);

#endif /* GENERATING_DEFS */

#endif /* __DYN_CONFIG_H__ */
