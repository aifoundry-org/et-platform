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

// Tensor instruction support
// Still work in progress
// I will be adding more of that as needed

// Tensor wait inputs
#define TENSOR_LOAD_WAIT_0 0
#define TENSOR_FMA_WAIT 7
#define TENSOR_STORE_WAIT 8
#define TENSOR_REDUCE_WAIT 9
#define TENSOR_QUANT_WAIT 10

//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensorWait
//
//   Tensor wait instruction, input parameter defines what the wait is for    
//
inline __attribute__((always_inline)) void tensorWait(long id) {
    __asm__ __volatile__
        (
         " csrw 0x830, %[id]\n"
         :
         : [id] "r" (id)
         : "memory"
        );
}


//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_load
//
//   Tensor load instruction, support for all tensor load versions to L1 SCP (not L2 SCP)
//
inline void __attribute__((always_inline)) tensor_load (uint64_t use_tmask,
                                                        uint64_t use_coop,
                                                        uint64_t dst_start,
                                                        uint64_t transformation,
                                                        uint64_t use_tenb,
                                                        uint64_t addr,
                                                        uint64_t offset,
                                                        uint64_t num_lines,
                                                        uint64_t stride,
                                                        uint64_t id)
{
   uint64_t csr_enc = ((use_tmask & 1) << 63)         |
                      ((use_coop & 1)  << 62)         |
                      ((transformation & 0x7) << 59)  |
                      ((dst_start & 0x3F) << 53)      |
                      ((use_tenb & 0x1) << 52)        |
                      ((addr & 0xFFFFFFFFFFC0ULL))    |
                      ((offset & 0x3) << 4)           |
                      ((num_lines & 0xF));
   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
         "add x31, zero, %[x31_enc]\n"
         "csrw 0x83f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
         : "x31"
   );
}


//-------------------------------------------------------------------------------------------------
//
// FUNCTION tensor_store
//
//   Tensor store instruction, support for tensor stores (coop or non-coop)
//   from the register file (not L1 SCP)
//
inline void __attribute__((always_inline)) tensor_store(uint64_t reg_stride,
                                                         uint64_t start_reg,
                                                         uint64_t cols,
                                                         uint64_t Arows,
                                                         uint64_t addr,
                                                         uint64_t coop_store,
                                                         uint64_t stride)
{
   uint64_t csr_enc = ((reg_stride     & 0x3 ) << 62) |
                      ((start_reg      & 0x1F) << 57) |
                      ((cols           & 0x3 ) << 55) |
                      ((addr & 0xFFFFFFFFFFF0)      ) |
                      ((Arows          & 0xF ) << 51) |
                      ((coop_store     & 0x3 ) << 49);

   uint64_t x31_enc = (stride & 0xFFFFFFFFFF0UL);

   __asm__ __volatile__ (
         "add x31, zero, %[x31_enc]\n"
         "csrw 0x87f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
         : "x31"
   );
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

   __asm__ __volatile__ (
         "csrr %0, 0x808"
         : "=r" (error)
         );

   return error;
}


#endif


