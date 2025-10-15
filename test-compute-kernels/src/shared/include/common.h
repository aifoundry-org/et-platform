/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
