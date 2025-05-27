/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) CROSSCON and Contributors. All rights reserved.
 */

#include <hypercall.h>

long void arch_hypercall(unsigned long id)
{
    struct vcpu* vcpu = cpu()->vcpu;

    list_foreach (vcpu->vm->hvc_list, struct hndl_hvc_node, node) {
        /* TODO: match range */
        hvc_handler_t handler = node->hndl_hvc.handler;
        if (handler != NULL) {
            if (handler(vcpu, id & 0xffff)) {
                /* ERROR("handler hvc failed (0x%x)", far); */
            }
        }
    }
}
