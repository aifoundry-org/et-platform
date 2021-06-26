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

#ifndef SHIRE_H
#define SHIRE_H

/// Mask of minions working as sync in master shire
#define SYNC_MINIONS_MASK     0xFFFF0000ULL
/// Number of threads working as sync
#define SYNC_THREADS          32
/// Number of minions per shire in the target SoC
#define SOC_MINIONS_PER_SHIRE 32

#include <stdint.h>

// Function that returns shire id
inline __attribute__((always_inline)) __attribute__ ((const))  uint64_t get_hart_id()
{
    uint64_t ret;
    __asm__ __volatile__ (
        "csrr %[ret], hartid\n" // u-mode hartid shadow
        : [ret] "=r" (ret)
    );
    return ret;
}

// Function that returns shire id
inline __attribute__((always_inline)) uint64_t get_shire_id()
{
    return get_hart_id() >> 6;
}

inline __attribute__((always_inline)) uint64_t get_neigh_id()
{
    return (get_hart_id() >> 4) & 3;
}

// Function that returns minion id
inline __attribute__((always_inline)) uint64_t get_minion_id()
{
    return get_hart_id() >> 1;
}

// functions to deal with CSR regs
inline __attribute__((always_inline)) uint64_t get_thread_id()
{
    return get_hart_id() & 1;
}

#endif // ! SHIRE_H
