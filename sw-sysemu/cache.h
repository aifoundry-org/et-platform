/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

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

//namespace bemu {


typedef Packed<L1D_LINE_SIZE*8> cache_line_t;


// Map logical scratchpad lines (0->47) to L1 cache lines (0->64).
inline unsigned scp_index_to_cache_index(unsigned entry)
{
    const static unsigned scp_to_l1[48] = {
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

inline unsigned dcache_index(uint64_t paddr, uint32_t mcache_control, uint32_t thread, uint32_t threads_per_minion)
{
    unsigned set;
    switch (mcache_control)
    {
        case 0:
            set = shared_dcache_index(paddr);
            break;
        case 1:
            set = (thread % threads_per_minion)
                    ? hart1_split_dcache_index(paddr)
                    : hart0_split_dcache_index(paddr);
            break;
        case 3:
            set = (thread % threads_per_minion)
                    ? hart1_split_dcache_index(paddr)
                    : hart0_spltscp_dcache_index(paddr);
            break;
        default:
            throw std::runtime_error("illegal mcache_control value");
    }
    return set;
}

//} // namespace bemu

#endif // BEMU_CACHE_H
