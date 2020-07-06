/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MEMOP_H
#define BEMU_MEMOP_H

#include <cstdint>
#include "memory/main_memory.h"

namespace bemu {


template<typename T>
inline void pmemread(uint64_t paddr, T* data)
{
    extern MainMemory memory;
    memory.read(Noagent{}, paddr, sizeof(T), data);
}


template <typename T>
inline void pmemwrite(uint64_t paddr, const T* data)
{
    extern MainMemory memory;
    memory.write(Noagent{}, paddr, sizeof(T), data);
}


} // namespace bemu

#endif // BEMU_MEMOP_H
