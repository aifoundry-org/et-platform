/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MEMOP_H
#define BEMU_MEMOP_H

#include "memory/main_memory.h"

namespace bemu {

template <typename T>
inline T pmemread(uint64_t paddr)
{

    static_assert(std::is_same<T, uint8_t>::value == true   ||
                  std::is_same<T, uint16_t>::value == true  ||
                  std::is_same<T, uint32_t>::value == true  ||
                  std::is_same<T, uint64_t>::value == true, "ERROR: Only uint8_t, uint16_t, uint32_t and uint64_t loads are performed by this function.");


    extern MainMemory memory;
    T ret;
    memory.read(paddr, sizeof(T), &ret);
    return ret;
}



inline void pmemread128(uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(paddr, 16, result);
}


inline void pmemread256(uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(paddr, 32, result);
}


inline void pmemread512(uint64_t paddr, void* result)
{
    extern MainMemory memory;
    memory.read(paddr, 64, result);
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


inline void pmemwrite128(uint64_t paddr, const void* source)
{
    extern MainMemory memory;
    memory.write(paddr, 16, source);
}


inline void pmemwrite512(uint64_t paddr, const void* source)
{
    extern MainMemory memory;
    memory.write(paddr, 64, source);
}


} // namespace bemu

#endif // BEMU_MEMOP_H
