/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_MEMORY_ERROR_H
#define BEMU_MEMORY_ERROR_H

#include <cstdint>

namespace bemu {


// Basic class for signaling any type of memory error to the caller
struct memory_error {
    uint64_t addr;

    constexpr memory_error(uint64_t x) : addr(x) { }
    constexpr memory_error(const memory_error&) = default;
};


} // namespace bemu

#endif // BEMU_MEMORY_ERROR_H
