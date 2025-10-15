/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cmath>
#include <cstdint>

namespace fpu {


int32_t fxp1714_rcpStep(int32_t a, int32_t b)
{
    // FIXME: We should not use floating-point here, we should implement a
    // softfloat equivalent

    // Input value is 2xtriArea with 15.16 precision
    double tmp = double(a) / double(1 << 16);

    double yn = double(b) / double(1 << 14);
    double fa = yn * tmp;
    uint32_t partial = uint32_t(fa * double(uint64_t(1) << 31));
    double unpartial = double(partial) / double(uint64_t(1) << 31);
    double result = yn * (2.0 - unpartial);
    return int32_t(result * double(1 << 14));
}


} // namespace fpu
