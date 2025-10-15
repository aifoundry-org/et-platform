/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
