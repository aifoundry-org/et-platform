/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_DECODE_H
#define BEMU_DECODE_H

#include <cstdint>

namespace bemu {

// Decode an instruction and return an opaque handler unique per opcode
extern uintptr_t decode(uint32_t bits);

} // namespace bemu

#endif // BEMU_DECODE_H
