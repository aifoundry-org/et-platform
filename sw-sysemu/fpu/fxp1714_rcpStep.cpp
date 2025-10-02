/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
