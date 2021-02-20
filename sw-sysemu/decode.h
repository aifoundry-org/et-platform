/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DECODE_H
#define BEMU_DECODE_H

#include <cstdint>

namespace bemu {

// Decode an instruction and return an opaque handler unique per opcode
extern uintptr_t decode(uint32_t bits);

} // namespace bemu

#endif // BEMU_DECODE_H
