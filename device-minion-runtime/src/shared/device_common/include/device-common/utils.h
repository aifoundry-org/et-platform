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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <inttypes.h>

#include "device-common/esr_defines.h"
#include "device-common/fcc.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

/*
 * General utils
 */

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

/*
 * RISC-V
 */

#define NOP   __asm__ __volatile__ ("nop\n");
#define FENCE __asm__ __volatile__ ("fence\n");
#define WFI   __asm__ __volatile__ ("wfi\n");

/*
 * CSR
 */

#define WAIT_TENSOR_LOAD_0     __asm__ __volatile__ ( "csrwi tensor_wait, 0\n" : : );
#define WAIT_TENSOR_LOAD_1     __asm__ __volatile__ ( "csrwi tensor_wait, 1\n" : : );
#define WAIT_TENSOR_LOAD_L2_0  __asm__ __volatile__ ( "csrwi tensor_wait, 2\n" : : );
#define WAIT_TENSOR_LOAD_L2_1  __asm__ __volatile__ ( "csrwi tensor_wait, 3\n" : : );
#define WAIT_PREFETCH_0        __asm__ __volatile__ ( "csrwi tensor_wait, 4\n" : : );
#define WAIT_PREFETCH_1        __asm__ __volatile__ ( "csrwi tensor_wait, 5\n" : : );
#define WAIT_CACHEOPS          __asm__ __volatile__ ( "csrwi tensor_wait, 6\n" : : );
#define WAIT_TENSOR_FMA        __asm__ __volatile__ ( "csrwi tensor_wait, 7\n" : : );
#define WAIT_TENSOR_STORE      __asm__ __volatile__ ( "csrwi tensor_wait, 8\n" : : );
#define WAIT_TENSOR_REDUCE     __asm__ __volatile__ ( "csrwi tensor_wait, 9\n" : : );
#define WAIT_TENSOR_QUANT      __asm__ __volatile__ ( "csrwi tensor_wait, 10\n" : : );

inline __attribute__((always_inline))__attribute__((always_inline)) uint64_t mcache_control_get(void)
{
    uint64_t ret;

    __asm__ __volatile__ (
        "csrr  %0, mcache_control\n"
        : "=r"(ret)
    );

    return ret;
}

inline __attribute__((always_inline)) void co_evict_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t val = ((use_tmask & 1) << 63) |
                   ((dst & 0x3)     << 58) |
                   ((set & 0xF)     << 14) |
                   ((way & 0x3)     << 6 ) |
                   ((num_lines & 0xF));

    __asm__ __volatile__ (
        "csrw 0x7f9, %0\n"
        : : "r" (val) : "memory"
    );
}

inline __attribute__((always_inline)) void co_unlock_sw(uint64_t way, uint64_t set)
{
    uint64_t val = ((way & 3)   << 55 ) |
                   ((set & 0xF)  << 6  );

    __asm__ __volatile__ (
        "csrw 0x7ff, %0\n"
        : : "r" (val)
    );
}

inline __attribute__((always_inline)) void tensorcooperation_write(uint64_t val)
{
    __asm__ __volatile__ (
        "csrw   0x804, %0\n"
        : : "r"(val)
    );
}


inline __attribute__((always_inline)) void tensormask_write(uint64_t val)
{
    __asm__ __volatile__ (
        "csrw   0x805, %0\n"
        : : "r"(val)
    );
}

inline __attribute__((always_inline)) void tensorerror_write(uint64_t val)
{
    __asm__ __volatile__ (
        "csrw   0x808, %0\n"
        : : "r"(val)
    );
}

inline __attribute__((always_inline)) uint64_t flbarrier(uint64_t barrier_num, uint64_t match)
{
    uint64_t ret;
    uint64_t flb_arg = (match << 5) | (barrier_num & 0x1F);

    __asm__ __volatile__ (
        "csrrw  %0, 0x820, %1\n"
        : "=r"(ret)
        : "r"(flb_arg)
    );

    return ret;
}

inline __attribute__((always_inline)) uint64_t flb(uint64_t barrier_num, uint64_t match)
{
    return flbarrier(barrier_num, match);
}

inline __attribute__((always_inline)) void fcc_consume(uint64_t fcc_reg)
{
    __asm__ __volatile__ (
        "csrw   fcc, %0\n"
        : : "r"(fcc_reg)
    );
}

inline __attribute__((always_inline))void fcc(uint64_t fcc_reg)
{
    fcc_consume(fcc_reg);
}

inline __attribute__((always_inline)) void stall(void)
{
  __asm__ __volatile__ ("csrw   stall, x0\n");
}

inline __attribute__((always_inline)) void tensorwait(uint64_t id)
{
    __asm__ __volatile__
      (
       "csrw  tensor_wait, %0\n"
       : : "r"(id)
       );

}

inline __attribute__((always_inline)) void portctrl0(uint64_t ctrl)
{
    __asm__ __volatile__ (
        "csrw   0x9cc, %0\n"
        : : "r"(ctrl)
        : "memory"
    );
}

inline __attribute__((always_inline)) void portctrl1(uint64_t ctrl)
{
    __asm__ __volatile__ (
        "csrw   0x9cd, %0\n"
        : : "r"(ctrl)
        : "memory"
    );
}

inline __attribute__((always_inline)) void portctrl2(uint64_t ctrl)
{
    __asm__ __volatile__ (
        "csrw   0x9ce, %0\n"
        : : "r"(ctrl)
        : "memory"
    );
}

inline __attribute__((always_inline)) void portctrl3(uint64_t ctrl)
{
    __asm__ __volatile__ (
        "csrw   0x9cf, %0\n"
        : : "r"(ctrl)
        : "memory"
    );
}

inline __attribute__((always_inline))__attribute__((always_inline)) uint64_t fccnb(void)
{
    uint64_t ret;

    __asm__ __volatile__ (
        "csrr  %0, fccnb\n"
        : "=r"(ret)
    );

    return ret;
}

/*
 * ESR
 */

// Sends an FCC credit
inline __attribute__((always_inline)) void fcc_send(uint32_t shire, uint32_t thread, uint32_t fcc_reg, uint64_t hart_mask)
{
    volatile uint64_t *fcc_credinc_addr = (uint64_t *)
        ESR_SHIRE(shire, FCC_CREDINC_0) + ((thread << 1) | fcc_reg);

    *fcc_credinc_addr = hart_mask;
}

inline __attribute__((always_inline)) void flbarrier_set(uint32_t shire, uint32_t barrier_num, uint64_t value)
{
    volatile uint64_t *flb_addr = (uint64_t *)
        ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier_num;

    *flb_addr = value;
}

/*
 * Other
 */

inline __attribute__((always_inline)) void riscv_fence(void)
{
  __asm__ __volatile__ ( "fence\n");
}

#endif

