/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SYSREG_ERROR_H
#define BEMU_SYSREG_ERROR_H

namespace bemu {


// Basic class for signaling any type of system register error (such as
// accessing an existing but unimplemented register) to the caller
struct sysreg_error {
    unsigned long long addr;

    constexpr sysreg_error() : addr(0) { }
    constexpr sysreg_error(unsigned long long x) : addr(x) { }
    constexpr sysreg_error(const sysreg_error&) = default;
};


} // namespace bemu

#endif // BEMU_SYSREG_ERROR_H
