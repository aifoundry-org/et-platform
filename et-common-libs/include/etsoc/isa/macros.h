/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __MACROS
#define __MACROS

#ifdef __ASSEMBLER__ // when included in an assembly file

#define ASM_TEST_START \
    fence;             \
    lui a7, 0xDEAD0;   \
    csrw validation0, a7;

#define ASM_TEST_PASS     \
    fence;                \
    lui a7, 0x1FEED;      \
    csrw validation0, a7; \
    wfi;

#define ASM_TEST_FAIL     \
    fence;                \
    lui a7, 0x50BAD;      \
    csrw validation0, a7; \
    wfi;

#else // __ASSEMBLER__ --> when included in a C/C++ file

#define C_TEST_START                              \
    __asm__ __volatile__("fence		 \n"             \
                         "lui  a7, 0xDEAD0\n"     \
                         "csrw validation0, a7\n" \
                         :                        \
                         :                        \
                         : "a7");

#define C_TEST_PASS                               \
    __asm__ __volatile__("fence		\n"              \
                         "lui a7, 0x1FEED\n"      \
                         "csrw validation0, a7\n" \
                         "wfi\n"                  \
                         :                        \
                         :                        \
                         : "a7");

#define C_TEST_FAIL                               \
    __asm__ __volatile__("fence		\n"              \
                         "lui a7, 0x50BAD\n"      \
                         "csrw validation0, a7\n" \
                         "wfi\n"                  \
                         :                        \
                         :                        \
                         : "a7");

#endif // __ASSEMBLER__

#endif // ! __MACROS
