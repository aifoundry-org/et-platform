/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_STATE_H
#define BEMU_STATE_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>

#include "fpu/fpu_types.h"

namespace bemu {


// -----------------------------------------------------------------------------
// A packed type can be accessed as unsigned or signed, bytes, halfwords,
// words, double-words, and as half, single, and double precision
// floating-point values.

template<size_t Nbits>
union Packed {
    std::array<uint8_t,Nbits/8>    u8;
    std::array<uint16_t,Nbits/16>  u16;
    std::array<uint32_t,Nbits/32>  u32;
    std::array<uint64_t,Nbits/64>  u64;
    std::array<int8_t,Nbits/8>     i8;
    std::array<int16_t,Nbits/16>   i16;
    std::array<int32_t,Nbits/32>   i32;
    std::array<int64_t,Nbits/64>   i64;
    std::array<float10_t,Nbits/16> f10;
    std::array<float11_t,Nbits/16> f11;
    std::array<float16_t,Nbits/16> f16;
    std::array<float32_t,Nbits/32> f32;
};


// -----------------------------------------------------------------------------
// Register types

enum : unsigned {
    XLEN = 64,
    FLEN = 32,

    VLEN  = 256,
    VLENB = (VLEN/8),
    VLENH = (VLEN/16),
    VLENW = (VLEN/32),
    VLEND = (VLEN/64),

    MLEN  = (VLEN/32),
    MLENW = (VLEN/32),

    NXREGS = 32,
    NFREGS = 32,
    NMREGS = 8
};


using freg_t = Packed<VLEN>;
using mreg_t = std::bitset<MLEN>;


static_assert(VLEN == 256, "Only 256-bit vectors supported");


} // namespace bemu

#endif // BEMU_STATE_H
