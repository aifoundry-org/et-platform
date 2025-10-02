/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
/***********************************************************************/
/*! \file include/etsoc/isa/utils.h
    \brief A C header that defines the functions to issue instructions.
*/
/***********************************************************************/
#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/fcc.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

/*
 * General utils
 */

#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))

/*
 * RISC-V
 */

#define NOP   __asm__ __volatile__("nop\n");
#define FENCE __asm__ __volatile__("fence\n");
#define WFI   __asm__ __volatile__("wfi\n");

/*
 * CSR
 */

/*! \def WAIT_TENSOR_LOAD_0
    \brief Tensor wait instruction for tensor load instruction to wait for data ready.
*/
#define WAIT_TENSOR_LOAD_0 __asm__ __volatile__("csrwi tensor_wait, 0\n" : :);

/*! \def WAIT_TENSOR_LOAD_1
    \brief Tensor wait instruction for tensor load instruction to wait for data ready.
*/
#define WAIT_TENSOR_LOAD_1 __asm__ __volatile__("csrwi tensor_wait, 1\n" : :);

/*! \def WAIT_TENSOR_LOAD_L2_0
    \brief Tensor wait instruction for tensor load l2scp instruction to wait for data ready.
*/
#define WAIT_TENSOR_LOAD_L2_0 __asm__ __volatile__("csrwi tensor_wait, 2\n" : :);

/*! \def WAIT_TENSOR_LOAD_L2_1
    \brief Tensor wait instruction for tensor load l2scp instruction to wait for data ready.
*/
#define WAIT_TENSOR_LOAD_L2_1 __asm__ __volatile__("csrwi tensor_wait, 3\n" : :);

/*! \def WAIT_PREFETCH_0
    \brief Tensor wait instruction for Prefetch cache management instruction to wait for data ready.
*/
#define WAIT_PREFETCH_0 __asm__ __volatile__("csrwi tensor_wait, 4\n" : :);

/*! \def WAIT_PREFETCH_1
    \brief Tensor wait instruction for Prefetch cache management instruction to wait for data ready.
*/
#define WAIT_PREFETCH_1 __asm__ __volatile__("csrwi tensor_wait, 5\n" : :);

/*! \def WAIT_CACHEOPS
    \brief Tensor wait instruction for Prefetch Any cache management instruction to wait for data ready.
*/
#define WAIT_CACHEOPS __asm__ __volatile__("csrwi tensor_wait, 6\n" : :);

/*! \def WAIT_TENSOR_FMA
    \brief Tensor wait instruction for tensor multiplication, reduction, or quantization to wait for data ready.
*/
#define WAIT_TENSOR_FMA __asm__ __volatile__("csrwi tensor_wait, 7\n" : :);

/*! \def WAIT_TENSOR_STORE
    \brief Tensor wait instruction for tensor store to wait for data ready.
*/
#define WAIT_TENSOR_STORE __asm__ __volatile__("csrwi tensor_wait, 8\n" : :);

/*! \def WAIT_TENSOR_REDUCE
    \brief Tensor wait instruction for tensor reduce to wait for data ready.
*/
#define WAIT_TENSOR_REDUCE __asm__ __volatile__("csrwi tensor_wait, 9\n" : :);

/*! \def WAIT_TENSOR_QUANT
    \brief Tensor wait instruction for tensor quant to wait for data ready.
*/
#define WAIT_TENSOR_QUANT __asm__ __volatile__("csrwi tensor_wait, 10\n" : :);

/*! \fn inline uint64_t mcache_control_get(void)
   \brief This function retuns mcache control register value.
   \return mcache control register value
   \tensorops Implementation of mcache_control_get api
*/
inline __attribute__((always_inline)) __attribute__((always_inline)) uint64_t mcache_control_get(
    void)
{
    uint64_t ret;

    __asm__ __volatile__("csrr  %0, mcache_control\n" : "=r"(ret));

    return ret;
}

/*! \fn inline void co_evict_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines)
   \brief This function Evicts a particular set and way in the L1 data cache
   \param use_tmask Use the tensor_mask CSR, If this bit is set, the tensor_mask register is used for this operation.
   \param dst Destination. Indicates the type of memory where the eviction will occur
   \param way Selects one of 4 ways in the L1 cache to be evicted.
   \param set Selects one of 16 sets in the L1 cache to be evicted
   \param num_lines Indicates the number of lines to be fetched from memory.
   \return none
   \tensorops Implementation of co_evict_sw api
*/
inline __attribute__((always_inline)) void co_evict_sw(
    uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t val = ((use_tmask & 1) << 63) | ((dst & 0x3) << 58) | ((set & 0xF) << 14) |
                   ((way & 0x3) << 6) | (num_lines & 0xF);

    __asm__ __volatile__("csrw 0x7f9, %0\n" : : "r"(val) : "memory");
}

/*! \fn inline void co_unlock_sw(uint64_t way, uint64_t set)
   \brief This function Unlocks a particular set and way in the L1 data cache
   \param way Selects one of 4 ways in the L1 cache to be evicted.
   \param set Selects one of 16 sets in the L1 cache to be evicted
   \return none
   \tensorops Implementation of co_unlock_sw api
*/
inline __attribute__((always_inline)) void co_unlock_sw(uint64_t way, uint64_t set)
{
    uint64_t val = ((way & 3) << 55) | ((set & 0xF) << 6);

    __asm__ __volatile__("csrw 0x7ff, %0\n" : : "r"(val));
}

/*! \fn inline void tensorcooperation_write(uint64_t val)
   \brief This function writes tensor coop register
   \param val value to write in tensor coop
   \return none
   \tensorops Implementation of tensorcooperation_write api
*/
inline __attribute__((always_inline)) void tensorcooperation_write(uint64_t val)
{
    __asm__ __volatile__("csrw   0x804, %0\n" : : "r"(val));
}

/*! \fn inline void tensormask_write(uint64_t val)
   \brief This function writes tensormask register
   \param val value to write in tensor mask
   \return none
   \tensorops Implementation of tensormask_write api
*/
inline __attribute__((always_inline)) void tensormask_write(uint64_t val)
{
    __asm__ __volatile__("csrw   0x805, %0\n" : : "r"(val));
}

/*! \fn inline void tensorerror_write(uint64_t val)
   \brief This function writes tensor error register
   \param val value to write in tensor mask
   \return none
   \tensorops Implementation of tensorerror_write api
*/
inline __attribute__((always_inline)) void tensorerror_write(uint64_t val)
{
    __asm__ __volatile__("csrw   0x808, %0\n" : : "r"(val));
}

/*! \fn inline uint64_t flbarrier(uint64_t barrier_num, uint64_t match)
   \brief This function executes FLBarrier instruction that allows one ET-Minion to "join" one of the eight FLB barriers. 
   \param barrier_num barrier number to use
   \param match match value
   \return none
   \tensorops Implementation of flbarrier api
*/
inline __attribute__((always_inline)) uint64_t flbarrier(uint64_t barrier_num, uint64_t match)
{
    uint64_t ret;
    uint64_t flb_arg = (match << 5) | (barrier_num & 0x1F);

    __asm__ __volatile__("csrrw  %0, 0x820, %1\n" : "=r"(ret) : "r"(flb_arg));

    return ret;
}

/*! \fn inline uint64_t flb(uint64_t barrier_num, uint64_t match)
   \brief This is a wrapper function for flbarrier api
   \param barrier_num barrier number to use
   \param match match value
   \return none
   \tensorops Implementation of flb api
*/
inline __attribute__((always_inline)) uint64_t flb(uint64_t barrier_num, uint64_t match)
{
    return flbarrier(barrier_num, match);
}

/*! \fn inline void fcc_consume(uint64_t fcc_reg)
   \brief This is a wrapper function to consume FCC
   \param fcc_reg FCC register ID
   \return none
   \tensorops Implementation of fcc_consume api
*/
inline __attribute__((always_inline)) void fcc_consume(uint64_t fcc_reg)
{
    __asm__ __volatile__("csrw   fcc, %0\n" : : "r"(fcc_reg));
}

/*! \fn inline void fcc(uint64_t fcc_reg)
   \brief This is a wrapper function for fcc_consume
   \param fcc_reg FCC register ID
   \return none
   \tensorops Implementation of fcc api
*/
inline __attribute__((always_inline)) void fcc(uint64_t fcc_reg)
{
    fcc_consume(fcc_reg);
}

/*! \fn inline void stall(void)
   \brief This will stall current execution
   \return none
   \tensorops Implementation of stall api
*/
inline __attribute__((always_inline)) void stall(void)
{
    __asm__ __volatile__("csrw   stall, x0\n");
}

/*! \fn void tensorwait(uint64_t id)
    \brief  This function will issue a tensorwait instruction on particular tensor id.
    \param id tensor id
    \return none
    \tensorops Implementation of tensorwait api
*/
inline __attribute__((always_inline)) void tensorwait(uint64_t id)
{
    __asm__ __volatile__("csrw  tensor_wait, %0\n" : : "r"(id));
}

/*! \fn void portctrl0(uint64_t ctrl)
    \brief  This function is a wrapper to execute port control instruction
    \param ctrl control register value
    \return none
    \tensorops Implementation of portctrl0 api
*/
inline __attribute__((always_inline)) void portctrl0(uint64_t ctrl)
{
    __asm__ __volatile__("csrw   0x9cc, %0\n" : : "r"(ctrl) : "memory");
}

/*! \fn void portctrl1(uint64_t ctrl)
    \brief  This function is a wrapper to execute port control instruction
    \param ctrl control register value
    \return none
    \tensorops Implementation of portctrl1 api
*/
inline __attribute__((always_inline)) void portctrl1(uint64_t ctrl)
{
    __asm__ __volatile__("csrw   0x9cd, %0\n" : : "r"(ctrl) : "memory");
}

/*! \fn void portctrl2(uint64_t ctrl)
    \brief  This function is a wrapper to execute port control instruction
    \param ctrl control register value
    \return none
    \tensorops Implementation of portctrl2 api
*/
inline __attribute__((always_inline)) void portctrl2(uint64_t ctrl)
{
    __asm__ __volatile__("csrw   0x9ce, %0\n" : : "r"(ctrl) : "memory");
}

/*! \fn void portctrl3(uint64_t ctrl)
    \brief  This function is a wrapper to execute port control instruction
    \param ctrl control register value
    \return none
    \tensorops Implementation of portctrl3 api
*/
inline __attribute__((always_inline)) void portctrl3(uint64_t ctrl)
{
    __asm__ __volatile__("csrw   0x9cf, %0\n" : : "r"(ctrl) : "memory");
}

/*! \fn void uint64_t fccnb(void)
    \brief  This function is a wrapper to read fccnb register
    \return fccnb value
    \tensorops Implementation of fccnb api
*/
inline __attribute__((always_inline)) __attribute__((always_inline)) uint64_t fccnb(void)
{
    uint64_t ret;

    __asm__ __volatile__("csrr  %0, fccnb\n" : "=r"(ret));

    return ret;
}

/*
 * ESR
 */

// Sends an FCC credit
/*! \fn void fcc_send(
    uint32_t shire, uint32_t thread, uint32_t fcc_reg, uint64_t hart_mask)
    \brief  This function will send FCC credit to a particular thread
    \param shire shire to send FCC
    \param thread thread id to send FCC
    \param fcc_reg fcc register
    \param hart_mask harts to send fcc credit
    \return none
    \syncops Implementation of fcc_send api
*/
inline __attribute__((always_inline)) void fcc_send(
    uint32_t shire, uint32_t thread, uint32_t fcc_reg, uint64_t hart_mask)
{
    volatile uint64_t *fcc_credinc_addr =
        (uint64_t *)ESR_SHIRE(shire, FCC_CREDINC_0) + ((thread << 1) | fcc_reg);

    *fcc_credinc_addr = hart_mask;
}

/*! \fn void flbarrier_set(
    uint32_t shire, uint32_t barrier_num, uint64_t value)
    \brief  The FLBarrier instruction allows one ET-Minion to "join" one of the eight FLB barriers.
    \param shire shire to send FCC
    \param barrier_num  barrier numbe
    \param value value to set in FLB
    \return none
    \syncops Implementation of flbarrier_set api
*/
inline __attribute__((always_inline)) void flbarrier_set(
    uint32_t shire, uint32_t barrier_num, uint64_t value)
{
    volatile uint64_t *flb_addr = (uint64_t *)ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier_num;

    *flb_addr = value;
}

/*
 * Other
 */

/*! \fn void riscv_fence(void)
    \brief  This function is a wrapper to execute fence instruction
    \return none
    \syncops Implementation of riscv_fence api
*/
inline __attribute__((always_inline)) void riscv_fence(void)
{
    __asm__ __volatile__("fence\n");
}

#ifdef __cplusplus
}
#endif

#endif
