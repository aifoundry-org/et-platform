/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __MACROS
#define __MACROS

#ifdef __ASSEMBLER__ // when included in an assembly file

#define ASM_TEST_START

#define ASM_TEST_PASS slti x0, x0, 0x7fe;
#define ASM_TEST_FAIL slti x0, x0, 0x7ff;

#define FENCE fence;

#define WAIT_TENSOR_LOAD_0     csrwi 0x830, 0;
#define WAIT_TENSOR_LOAD_1     csrwi 0x830, 1;
#define WAIT_TENSOR_LOAD_L2_0  csrwi 0x830, 2;
#define WAIT_TENSOR_LOAD_L2_1  csrwi 0x830, 3;
#define WAIT_PREFETCH_0        csrwi 0x830, 4;
#define WAIT_PREFETCH_1        csrwi 0x830, 5;
#define WAIT_CACHEOPS          csrwi 0x830, 6;
#define WAIT_TENSOR_FMA        csrwi 0x830, 7;
#define WAIT_TENSOR_STORE      csrwi 0x830, 8;
#define WAIT_TENSOR_REDUCE     csrwi 0x830, 9;
#define WAIT_TENSOR_QUANT      csrwi 0x830, 10;

#else // __ASSEMBLER__ --> when included in a C/C++ file

#define C_TEST_START

#define C_TEST_PASS __asm__ __volatile__ ( "slti x0, x0, 0x7fe\n" ::: );
#define C_TEST_FAIL __asm__ __volatile__ ( "slti x0, x0, 0x7ff\n" ::: );

#define NOP  __asm__ __volatile__ ("nop\n");
#define FENCE __asm__ __volatile__ ("fence\n");
#define WFI __asm__ __volatile__ ("wfi\n");
#define WAIT_TENSOR_LOAD_0     __asm__ __volatile__ ( "csrwi 0x830, 0\n" : : );
#define WAIT_TENSOR_LOAD_1     __asm__ __volatile__ ( "csrwi 0x830, 1\n" : : );
#define WAIT_TENSOR_LOAD_L2_0  __asm__ __volatile__ ( "csrwi 0x830, 2\n" : : );
#define WAIT_TENSOR_LOAD_L2_1  __asm__ __volatile__ ( "csrwi 0x830, 3\n" : : );
#define WAIT_PREFETCH_0        __asm__ __volatile__ ( "csrwi 0x830, 4\n" : : );
#define WAIT_PREFETCH_1        __asm__ __volatile__ ( "csrwi 0x830, 5\n" : : );
#define WAIT_CACHEOPS          __asm__ __volatile__ ( "csrwi 0x830, 6\n" : : );
#define WAIT_TENSOR_FMA        __asm__ __volatile__ ( "csrwi 0x830, 7\n" : : );
#define WAIT_TENSOR_STORE      __asm__ __volatile__ ( "csrwi 0x830, 8\n" : : );
#define WAIT_TENSOR_REDUCE     __asm__ __volatile__ ( "csrwi 0x830, 9\n" : : );
#define WAIT_TENSOR_QUANT      __asm__ __volatile__ ( "csrwi 0x830, 10\n" : : );
#define STALL                  __asm__ __volatile__ ( "csrw stall, x0\n" : : );
#define CLEAR_TENSOR_ERROR     __asm__ __volatile__ ( "csrwi 0x808, 0" : : );
#define MCACHE_CONTROL(x1, x2, x3, x4) __asm__ __volatile__("csrw 0x7e0, %0\n" : : "r"(((x1 & 0x1F) << 6) | ((x2 & 0x7) << 2) | ((x3 & 0x1) << 1) | ((x4 & 0x1) << 0)) : "x31");
#define EXCL_MODE(val) __asm__ __volatile__("csrw 0x7d3, %[csr_enc]\n" : : [csr_enc] "r"(val) : "x31"); 
#endif // __ASSEMBLER__

#endif // ! __MACROS

