/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
