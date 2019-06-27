/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MEMOP_H
#define BEMU_MEMOP_H

#include "memory/main_memory.h"

namespace bemu {


inline uint8_t pmemread8(uint64_t paddr)
{
    extern MainMemory memory;
    uint8_t ret;
    memory.read(paddr, 1, &ret);
    return ret;
}


inline uint16_t pmemread16(uint64_t paddr)
{
    extern MainMemory memory;
    uint16_t ret;
    memory.read(paddr, 2, &ret);
    return ret;
}


inline uint32_t pmemread32(uint64_t paddr)
{
    extern MainMemory memory;
    uint32_t ret;
    memory.read(paddr, 4, &ret);
    return ret;
}


inline uint64_t pmemread64(uint64_t paddr)
{
    extern MainMemory memory;
    uint64_t ret;
    memory.read(paddr, 8, &ret);
    return ret;
}


inline void pmemwrite8(uint64_t paddr, uint8_t data)
{
    extern MainMemory memory;
    memory.write(paddr, 1, &data);
}


inline void pmemwrite16(uint64_t paddr, uint16_t data)
{
    extern MainMemory memory;
    memory.write(paddr, 2, &data);
}


inline void pmemwrite32(uint64_t paddr, uint32_t data)
{
    extern MainMemory memory;
    memory.write(paddr, 4, &data);
}


inline void pmemwrite64(uint64_t paddr, uint64_t data)
{
    extern MainMemory memory;
    memory.write(paddr, 8, &data);
}


} // namespace bemu

#endif // BEMU_MEMOP_H
