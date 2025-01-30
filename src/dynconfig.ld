/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

SECTIONS
{
    . = 0x0;

    .dynconfig : {
        KEEP(*.dynconfig)
    }
    .data : {
        KEEP(*(.data))
    }
    _dynconfig_end = ALIGN(CONSTANT(COMMONPAGESIZE));

    .vm_images : SUBALIGN(CONSTANT(COMMONPAGESIZE)) {
        KEEP(*(.vm_image*))
    }
    _images_end = ALIGN(CONSTANT(COMMONPAGESIZE));

}

