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

#include "crosscon_soc.h"

void set_pg_rspace_entry(
    unsigned int* pg_csrs_base,
    unsigned char entry,
    unsigned char did,
    unsigned int from_addr,
    unsigned int to_addr,
    unsigned char allow_read,
    unsigned char allow_write)
{
    pg_csrs_base[PG_RAS_ADDR_FIRST_REG_ADDR + entry] = from_addr;
    pg_csrs_base[PG_RAS_ADDR_FIRST_REG_ADDR + entry + 1] = to_addr;
    pg_csrs_base[PG_RAS_ACC_PRM_FIRST_REG_ADDR + entry] = (unsigned int) (
        (1 << PG_RAS_ACC_PRM_REG_A_BIT) |
        (did << PG_RAS_ACC_PRM_REG_DID_LOW) |
        (allow_read << PG_RAS_ACC_PRM_REG_R_BIT) |
        (allow_write << PG_RAS_ACC_PRM_REG_W_BIT));
}

void clear_pg_rspace_entry(unsigned int* pg_csrs_base, unsigned char entry) {
    pg_csrs_base[PG_RAS_ACC_PRM_FIRST_REG_ADDR + entry] = (unsigned int) (0 << PG_RAS_ACC_PRM_REG_A_BIT);
}
