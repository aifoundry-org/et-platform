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

#include "memory/main_memory.h"

namespace bemu {


template<typename T> inline T pmemread(const Agent& agent, uint64_t paddr)
{
    extern MainMemory memory;
    T ret;
    memory.read(agent, paddr, sizeof(T), &ret);
    return ret;
}


inline void pmemread128(const Agent& agent, uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(agent, paddr, 16, result);
}


inline void pmemread256(const Agent& agent, uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(agent, paddr, 32, result);
}


inline void pmemread512(const Agent& agent, uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(agent, paddr, 64, result);
}


template <typename T>
inline void pmemwrite(const Agent& agent, uint64_t paddr, T data)
{
    extern MainMemory memory;
    memory.write(agent, paddr, sizeof(T), &data);
}


inline void pmemwrite128(const Agent& agent, uint64_t paddr, const void* source)
{
    extern MainMemory memory;
    memory.write(agent, paddr, 16, source);
}


inline void pmemwrite512(const Agent& agent, uint64_t paddr, const void* source)
{
    extern MainMemory memory;
    memory.write(agent, paddr, 64, source);
}


} // namespace bemu

#endif // BEMU_MEMOP_H
