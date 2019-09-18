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

#ifndef __MACROS
#define __MACROS

#ifdef __ASSEMBLER__ // when included in an assembly file

#define ASM_TEST_START \
   fence	   ;\
   lui  a7, 0xDEAD0; \
   csrw validation0, a7;

#define ASM_TEST_PASS \
   fence	   ;\
   lui a7, 0x1FEED; \
   csrw validation0, a7; \
   wfi; \

#define ASM_TEST_FAIL \
   fence	   ;\
   lui a7, 0x50BAD; \
   csrw validation0, a7; \
   wfi; \

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

#else // __ASSEMBLER__ --> when included in a C/C++ file

#define C_TEST_START \
   __asm__ __volatile__ ( \
	 "fence		 \n"\
         "lui  a7, 0xDEAD0\n" \
         "csrw validation0, a7\n" \
         : : : "a7");

#define C_TEST_PASS \
   __asm__ __volatile__ ( \
	 "fence		\n" \
         "lui a7, 0x1FEED\n" \
         "csrw validation0, a7\n" \
         "wfi\n" \
         : : : "a7");

#define C_TEST_FAIL \
   __asm__ __volatile__ ( \
	 "fence		\n"\
         "lui a7, 0x50BAD\n" \
         "csrw validation0, a7\n" \
         "wfi\n" \
         : : : "a7");

#define NOP  __asm__ __volatile__ ("nop");
#define FENCE __asm__ __volatile__ ("fence" : : : "memory");
#define WFI __asm__ __volatile__ ("wfi");
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

#endif // __ASSEMBLER__

#endif // ! __MACROS
