/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef IMSIC_H
#define IMSIC_H

#include <crossconhyp.h>
#include <platform.h>

#define IMSIC_MAX_INTERRUPTS (PLAT_IMSIC_MAX_INTERRUPTS)

/**
 * @brief Initializes the IMSIC
 *
 *        The function initializes the IMSIC by configuring its registers and mapping the S-lvl
 *        interrupt file. It sets every intp as triggerable, disables all interrupts, enables
 *        interrupt delivery, and maps the S-lvl interrupt file in memory.
 *
 */
void imsic_init(void);

/**
 * @brief Check if a given interrupt is pending for the cpu that calls the function
 *
 * @param intp_id the interrupt to check
 * @return true the interrupt is pending
 * @return false the interrupt is not pending
 */
bool imsic_get_pend(irqid_t intp_id);

/**
 * @brief Clear the pending bit of a given interrupt
 *
 * @param intp_id interrupt ID
 */
void imsic_clr_pend(irqid_t intp_id);

/**
 * @brief enables a given interrupt for the IMSIC that executes the function
 *
 * @param intp_id the interrupt to enable
 */
void imsic_set_enbl(irqid_t intp_id);

/**
 * @brief Sends an MSI to the specified CPU with the specified IPI ID.
 *
 *        The function sends an MSI to the specified CPU by setting the seteipnum_le register in the
 *        IMSIC. The seteipnum_le register is used to specify the ID of the interrupt being sent.
 *        Only little endian is supported.
 *
 * @param target_cpu The ID of the target CPU
 * @param msi_id The MSI ID to be sent
 */
void imsic_send_msi(cpuid_t target_cpu, irqid_t msi_id);

/**
 * @brief Sends an MSI to the guest file of specified CPU with the specified IPI ID.
 *
 *        The function sends an MSI to the target CPU's guest file by setting the seteipnum_le
 *        register in the IMSIC. The seteipnum_le register is used to specify the ID of the
 *        interrupt being sent. Only little endian is supported. This function assume a single guest
 *        interrupt file per hart.
 *
 * @param target_cpu The ID of the target CPU
 * @param msi_id The MSI ID to be injected in the guest interrupt file
 */
void imsic_send_guest_msi(cpuid_t target_cpu, size_t guest_index, irqid_t msi_id);

/**
 * @brief Handles interrupts in the IMSIC.
 *
 *        The function handles interrupts in the IMSIC by looping through
 *        all pending interrupts and calling the interrupts_handle()
 *        function to handle each one. If an interrupt is handled by the
 *        hypervisor, the function writes to the STOPEI CSR to clear the
 *        interrupt. Otherwise, the Guest cleans it.
 *
 */
void imsic_handle(void);

/**
 * @brief Allocate an MSI ID for hypervisor use
 *
 * @return irqid_t the allocated MSI ID
 */
irqid_t imsic_allocate_msi(void);

/**
 * @brief Allocate a guest interrupt file
 *
 * @return ssize_t the ID of the allocated guest interrupt file (as an index in hstatus.VGEIN) if
 * available, and a negative value if not.
 */
ssize_t imsic_alloc_guest_int_file(void);

#endif // IMSIC_H
