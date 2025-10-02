/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _IO_H_
#define _IO_H_

#include <stdint.h>

static inline uint8_t ioread8(uintptr_t addr)
{
    uint8_t val;
    asm volatile("lb %0, %1\n" : "=r"(val) : "m"(*(const volatile uint8_t *)addr));
    return val;
}

static inline void iowrite8(uintptr_t addr, uint8_t val)
{
    asm volatile("sb %1, %0\n" : "=m"(*(volatile uint8_t *)addr) : "r"(val));
}

static inline uint16_t ioread16(uintptr_t addr)
{
    uint16_t val;
    asm volatile("lh %0, %1\n" : "=r"(val) : "m"(*(const volatile uint16_t *)addr));
    return val;
}

static inline void iowrite16(uintptr_t addr, uint16_t val)
{
    asm volatile("sh %1, %0\n" : "=m"(*(volatile uint16_t *)addr) : "r"(val));
}

static inline uint32_t ioread32(uintptr_t addr)
{
    uint32_t val;
    asm volatile("lw %0, %1\n" : "=r"(val) : "m"(*(const volatile uint32_t *)addr));
    return val;
}

static inline void iowrite32(uintptr_t addr, uint32_t val)
{
    asm volatile("sw %1, %0\n" : "=m"(*(volatile uint32_t *)addr) : "r"(val));
}

static inline uint64_t ioread64(uintptr_t addr)
{
    uint64_t val;
    asm volatile("ld %0, %1\n" : "=r"(val) : "m"(*(const volatile uint64_t *)addr));
    return val;
}

static inline void iowrite64(uintptr_t addr, uint64_t val)
{
    asm volatile("sd %1, %0\n" : "=m"(*(volatile uint64_t *)addr) : "r"(val));
}

static inline void iormw32(uintptr_t addr, uint32_t modifier)
{
    uint32_t value;
    asm volatile("lw %0, %1\n" : "=r"(value) : "m"(*(const volatile uint32_t *)addr));
    value |= modifier;
    asm volatile("sw %1, %0\n" : "=m"(*(volatile uint32_t *)addr) : "r"(value));
}

static inline void memcpy256(uintptr_t dest_addr, uintptr_t src_addr)
{
    asm volatile(
        "flq2 f0, 0(%[src_addr])\n"
        "fsq2 f0, 0(%[dest_addr])\n"
        :
        : [src_addr] "r" (src_addr),  [dest_addr] "r" (dest_addr)
    );
}

#endif
