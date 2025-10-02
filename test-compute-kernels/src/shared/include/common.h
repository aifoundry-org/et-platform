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

#ifndef COMMON_H
#define COMMON_H

#define STRINGIFY(N) #N

#ifdef __clang__
#define PRAGMA_UNROLL_LOOP(N) _Pragma(STRINGIFY(clang loop unroll_count(N)))
#else
#define PRAGMA_UNROLL_LOOP(N) _Pragma(STRINGIFY(GCC unroll N))
#endif

//-------------------------------------------------------------------------------------------------
//
// FUNCTION store
//
//   Regular 64b RISCV store (sd instruction)
//
inline __attribute__((always_inline)) void store(unsigned long addr, unsigned long value)
{
    __asm__ __volatile__("  sd %[value], 0(%[addr])\n"
                         :
                         : [addr] "r"(addr), [value] "r"(value)
                         : "memory");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION store
//
//   Regular 64b RISCV load (ld instruction)
//
inline __attribute__((always_inline)) void load(unsigned long addr, unsigned long value)
{
    __asm__ __volatile__("  ld %[value], 0(%[addr])\n"
                         : [value] "+&r"(value)
                         : [addr] "r"(addr)
                         : "memory");
}

#endif
