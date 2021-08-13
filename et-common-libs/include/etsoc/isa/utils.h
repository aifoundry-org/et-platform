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

#include "esr_defines.h"
#include "fcc.h"

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

#endif

