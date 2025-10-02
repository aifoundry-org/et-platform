/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_UTILITY_H
#define BEMU_UTILITY_H

#include <cstdint>

namespace bemu {


template<size_t N>
inline uint64_t sext(uint64_t val)
{
    return (N >= 64) ? val : uint64_t((int64_t(val) << (64-N)) >> (64-N));
}


inline int32_t sext8_2(uint8_t val)
{
    return (val & 0x80) ? (0xffffff00 | val) : val;
}


inline uint64_t sextVA(uint64_t addr)
{
    // if bits addr[63:VA-1] are not all the same then set bits addr[63:VA] to
    // ~addr[VA-1], else leave as is
    int64_t sign = int64_t(addr) >> 47;
    return (sign == 0 || ~sign == 0)
            ? addr
            : uint64_t(int64_t(((addr << 15) & ~(1ll << 63)) |
                               (~(addr << 16) & (1ll << 63))) >> 15);
}


inline uint64_t zextPA(uint64_t addr)
{
    return addr & 0x000000ffffffffffull;
}


inline bool addr_is_size_aligned(uint64_t addr, size_t size)
{
    return !(addr % size);
}


} // namespace bemu

#endif // BEMU_UTILITY_H
