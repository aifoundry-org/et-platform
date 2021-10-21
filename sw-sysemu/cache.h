/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_CACHE_H
#define BEMU_CACHE_H

#include <cstddef>
#include <iosfwd>
#include <stdexcept>

#include "state.h"

// Dcache configuration
#define L1D_NUM_SETS      16
#define L1D_NUM_WAYS      4
#define L1D_LINE_SIZE     64

// Scratchpad configuration
#define L1_SCP_NUM_SETS   12
#define L1_SCP_NUM_WAYS   L1D_NUM_WAYS
#define L1_SCP_ENTRIES    (L1_SCP_NUM_SETS * L1_SCP_NUM_WAYS)
#define L1_SCP_LINE_SIZE  L1D_LINE_SIZE

namespace bemu {


using cache_line_t = Packed<L1D_LINE_SIZE*8>;


// Map logical scratchpad lines (0->47) to L1 cache lines (0->64).
inline unsigned scp_index_to_cache_index(unsigned entry)
{
    static const unsigned scp_to_l1[48] = {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
    };
    return (entry >= 48) ? entry : scp_to_l1[entry];
}


inline unsigned shared_dcache_index(uint64_t paddr)
{
    return (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;
}


inline unsigned hart0_spltscp_dcache_index(uint64_t paddr)
{
    return L1_SCP_NUM_SETS + ((paddr / L1D_LINE_SIZE) % 2);
}


inline unsigned hart0_split_dcache_index(uint64_t paddr)
{
    return (paddr / L1D_LINE_SIZE) % (L1D_NUM_SETS / 2);
}


inline unsigned hart1_split_dcache_index(uint64_t paddr)
{
    return (L1D_NUM_SETS - 2) + ((paddr / L1D_LINE_SIZE) % 2);
}


inline unsigned dcache_index(uint64_t paddr, uint8_t mcache_control, unsigned thread_in_minion)
{
    switch (mcache_control) {
        case 0:
            return shared_dcache_index(paddr);
        case 1:
            return (thread_in_minion == 0)
                ? hart0_split_dcache_index(paddr)
                : hart1_split_dcache_index(paddr);
        case 3:
            return (thread_in_minion == 0)
                ? hart0_spltscp_dcache_index(paddr)
                : hart1_split_dcache_index(paddr);
        default:
            throw std::runtime_error("illegal mcache_control value");
    }
}


} // namespace bemu

#endif // BEMU_CACHE_H
