/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef TENSOR_H
#define TENSOR_H

#include <stdint.h>
#include <stdbool.h>

// Tensor instruction support
// Still work in progress
// I will be adding more of that as needed

// Tensor wait inputs
#define TENSOR_LOAD_WAIT_0 0
#define TENSOR_LOAD_WAIT_1 1
#define TENSOR_FMA_WAIT    7
#define TENSOR_STORE_WAIT  8
#define TENSOR_REDUCE_WAIT 9
#define TENSOR_QUANT_WAIT  10

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensorWait
//
//   Tensor wait instruction, input parameter defines what the wait is for
//
inline __attribute__((always_inline)) void tensor_wait(long id)
{
    __asm__ __volatile__(" csrw 0x830, %[id]\n" : : [id] "r"(id) : "memory");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_load
//
//   Tensor load instruction, support for all tensor load versions to L1 SCP (not L2 SCP)
//
inline void __attribute__((always_inline))
tensor_load(uint64_t use_tmask, uint64_t use_coop, uint64_t dst_start, uint64_t transformation,
            uint64_t use_tenb, uint64_t addr, uint64_t offset, uint64_t num_lines, uint64_t stride,
            uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) | ((use_coop & 1) << 62) |
                       ((transformation & 0x7) << 59) | ((dst_start & 0x3F) << 53) |
                       ((use_tenb & 0x1) << 52) | ((addr & 0xFFFFFFFFFFC0ULL)) |
                       ((offset & 0x3) << 4) | ((num_lines & 0xF));
    uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("add x31, zero, %[x31_enc]\n"
                         "csrw 0x83f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc)
                         : "x31");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_store
//
//   Tensor store instruction, support for tensor stores (coop or non-coop)
//   from the register file (not L1 SCP)
//
inline void __attribute__((always_inline))
tensor_store(uint64_t reg_stride, uint64_t start_reg, uint64_t cols, uint64_t rows, uint64_t addr,
             uint64_t coop_store, uint64_t stride)
{
    uint64_t csr_enc = ((reg_stride & 0x3) << 62) | ((start_reg & 0x1F) << 57) |
                       ((cols & 0x3) << 55) | ((addr & 0xFFFFFFFFFFF0)) | ((rows & 0xF) << 51) |
                       ((coop_store & 0x3) << 49);

    uint64_t x31_enc = (stride & 0xFFFFFFFFFF0UL);

    __asm__ __volatile__("add x31, zero, %[x31_enc]\n"
                         "csrw 0x87f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc)
                         : "x31");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_store_scp
//
//   Tensor store instruction, support for tensor stores from the L1 SCP
//
inline void __attribute__((always_inline))
tensor_store_scp(uint64_t entry_stride, uint64_t start_scp_entry, uint64_t rows, uint64_t addr,
                 uint64_t stride)
{
    uint64_t csr_enc = ((entry_stride & 0x3) << 62) | ((start_scp_entry & 0x3F) << 56) |
                       ((addr & 0xFFFFFFFFFFC0ULL)) | ((rows & 0xF) << 51) | (((uint64_t)1) << 48);
    uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0UL);

    __asm__ __volatile__("add x31, zero, %[x31_enc]\n"
                         "csrw 0x87f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc)
                         : "x31");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_fma
//
//   Tensor FMA instruction. Supports all flavors of FMA (IMA8A32, FMA16A32, FMA32)
//
inline void __attribute__((always_inline))
tensor_fma(bool use_tmask, uint64_t b_num_col, uint64_t a_num_rows, uint64_t a_num_cols,
           uint64_t offset, bool tenc_loc, bool tenb_unsigned, bool tena_unsigned, bool tenb_loc,
           uint64_t scp_loc_b, uint64_t scp_loc_a, uint64_t opcode, bool first_pass)
{
    uint64_t csr_enc =
        (((uint64_t)use_tmask & 1) << 63) | ((b_num_col & 0x3) << 55) | ((a_num_rows & 0xF) << 51) |
        ((a_num_cols & 0xF) << 47) | ((offset & 0xF) << 43) | (((uint64_t)tenc_loc & 1) << 23) |
        (((uint64_t)tena_unsigned & 1) << 22) | (((uint64_t)tenb_unsigned & 1) << 21) |
        (((uint64_t)tenb_loc & 1) << 20) | ((scp_loc_b & 0xFF) << 12) | ((scp_loc_a & 0xFF) << 4) |
        ((opcode & 0x7) << 1) | ((uint64_t)first_pass & 1);

    __asm__ __volatile__("csrw 0x801, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_reduce
//
//   Tensor Reduce instruction. Supports all flavors of Reduce (send, receive, broadcast, auto)
//   For broadcast and auto partnerID should be the depth
//
inline void __attribute__((always_inline))
tensor_reduce(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t partnerID,
              uint64_t action)
{
    uint64_t csr_enc = ((start_reg & 0x1F) << 57) | ((operation & 0xF) << 24) |
                       ((num_reg & 0xFF) << 16) | ((partnerID & 0x1FFF) << 3) | ((action & 0x3));

    __asm__ __volatile__("csrw 0x800, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_coop
//
//   Writes the Coop CSR register used by coop loads
//
inline void __attribute__((always_inline)) tensor_coop(uint64_t val)
{
    __asm__ __volatile__("csrw 0x804, %[val]\n" : : [val] "r"(val) :);
}

unsigned long get_tensor_error(void);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION get_tensor_error
//
//   Returns value of tensor error -- use it always after a tensor wait
//
inline unsigned long __attribute__((always_inline)) get_tensor_error()
{
    unsigned long error;

    __asm__ __volatile__("csrr %0, 0x808" : "=r"(error));

    return error;
}

#endif
