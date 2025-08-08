/*
 * Copyright (c) 2014-2024 Beyond Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BEYOND SEMICONDUCTOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BEYOND SEMICONDUCTOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CROSSCON_SOC_H__
#define __CROSSCON_SOC_H__

// Arty-A7 100T CPU clock
#define CPU_CLK 25000000

//
// PG configuration interfaces base addresses
//

// Available only to BA51-H and no other master connected to the AHB
// interconnect.

// BA51's DID register
#define PG_ADD_SIG_DRV_ADR      0x23100000
// QMEM's PG configuration registers base address
#define QMEM_PG_CSR_ADR         0x23200000
// APB subsystem's PG configuration registers base address
#define APB_SUB_PG_CSR_ADR      0x23300000
// SRAM subsystem's PG configuration registers base address
#define SRAM_PG_CSR_ADR         0x23400000

// Interrupt raised by PG if access is violated.
#define PG_IRQ 12

//
// Base addresses of slaves connected to AHB interconnect.
//

// QMEM base address
#define QMEM_BASE_ADR                   0x00000000
// UART 0 base address
#define UART_0_BASE_ADR                 0x10700000
// UART 1 base address
#define UART_1_BASE_ADR                 0x11000000
// SRAM base addressk
#define SRAM_BASE_ADR                   0x20000000
// SRAM PG lock-release interface base address
#define SRAM_PG_LR_BASE_ADR             0x21000000
// AES-GCM base address
#define AES_GCM_CFG_BASE_ADR            0x24000000

// UART 0 interrupt request number
#define UART_0_IRQ 10
// UART 1 interrupt request number
#define UART_1_IRQ 13

//
// PG configuration registers
//

// PG's configuration register
#define PG_CFG_REG_ADDR 1

// (Read-write register) Set to 1, when PG raised interrupt. Write 1 to clear the interrupt.
#define PG_INTR_SRC_REG_ADDR 2
// (Read-only register): When interrupt was raised by PG, set to a domain that raised the interrupt.
#define PG_INTR_CAUSE_DOMAIN_REG_ADDR 3
// (Read-only register): When interrupt was raised by PG, set to a domain that raised the interrupt.
#define PG_INTR_CAUSE_ADDR_REG_ADDR 4

//
// PG lock-release arbitration mode interface registers
//

// PG_LOCK_REG: (Read-only register) Read from PG_LOCK_REG to lock the SRAM.
//  If domain is holding the lock, 1 is returned; otherwise 0.
#define PG_LOCK_REG 0
// PG_RELEASE_REG: (Read-only register) If read by a domain that holds the
// lock, the protected module is released and reset.
#define PG_RELEASE_REG 1

//
// PG's restricted address space mode configuration registers
//

// Number of available address ranges. Needs to be adjusted to the actual
// number of ranges supported by a specific instance of PG.
#define PG_RAS_NUM_OF_RANGES 8
// The address of the first range address register
#define PG_RAS_ADDR_FIRST_REG_ADDR 0x10
// The address of the first range access permission registers
#define PG_RAS_ACC_PRM_FIRST_REG_ADDR (PG_RAS_ADDR_FIRST_REG_ADDR + PG_RAS_NUM_OF_RANGES + 1)
                                                                                
// The range address permission field indices
// Read permission index
#define PG_RAS_ACC_PRM_REG_R_BIT 0
// Write permission index
#define PG_RAS_ACC_PRM_REG_W_BIT 1
// Start and end of the DID field
#define PG_RAS_ACC_PRM_REG_DID_SIZE 29
#define PG_RAS_ACC_PRM_REG_DID_LOW 2
#define PG_RAS_ACC_PRM_REG_DID_HIGH (PG_RAS_ACC_PRM_REG_DID_LOW + PG_RAS_ACC_PRM_REG_DID_SIZE)
 // Range active index - HIGH when the range has a valid address and permissions.
#define PG_RAS_ACC_PRM_REG_A_BIT 31

//
// AES-GCM accelerator configuration registers
//

// State register
#define AES_GCM_STATE_REG 0
// Encryption / decryption registers
#define AES_GCM_ENCRYPT_REG 1
// Key length register
#define AES_GCM_KEYL_REG 2
// Key register
#define AES_GCM_KEY_REG 3
// Initialization vector register
#define AES_GCM_IV_REG 11
// Address length
#define AES_GCM_A_LEN_REG 14
// Data length
#define AES_GCM_D_LEN_REG 15
// Address where the input data buffer is located
#define AES_GCM_IN_D_ADDR_REG 16
// Address where the output data buffer is located
#define AES_GCM_OUT_D_ADDR_REG 17

// State register fields
// Needs to be set to HIGH when the master has provided the initialization parameters.
#define AES_GCM_STATE_INIT_FLAG 0
// (Write-only) HIGH when input data is ready.
#define AES_GCM_STATE_IN_D_RDY_FLAG 1
// (Read-only) HIGH when output data is ready.
#define AES_GCM_STATE_OUT_D_RDY_FLAG 2
// (Read-only) HIGH when tag is ready.
#define AES_GCM_STATE_TAG_RDY_FLAG 3

//
// Helper functions
//

// Set address range entry of PG in restricted address space mode.
void set_pg_rspace_entry(
    unsigned int* pg_csrs_base,
    unsigned char entry,
    unsigned char did,
    unsigned int from_addr,
    unsigned int to_addr,
    unsigned char allow_read,
    unsigned char allow_write);

// Clear address range entry of PG in restricted address space mode.
void clear_pg_rspace_entry(unsigned int* pg_csrs_base, unsigned char entry);

#endif /* __CROSSCON_SOC_H__ */
