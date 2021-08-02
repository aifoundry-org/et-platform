#include "mock_atomics.h"

uint64_t atomic_load_local_64(const uint64_t *addr)
{
    return addr[0];
}

uint32_t atomic_load_local_32(const uint32_t *addr)
{
    return addr[0];
}

uint16_t atomic_load_local_16(const uint16_t *addr)
{
    return addr[0];
}

uint8_t atomic_load_local_8(const uint8_t *addr)
{
    return addr[0];
}

uint64_t atomic_store_local_64(uint64_t *addr, uint64_t value)
{
    return addr[0] = value;
}

uint32_t atomic_store_local_32(uint32_t *addr, uint32_t value)
{
    return addr[0] = value;
}

uint16_t atomic_store_local_16(uint16_t *addr, uint16_t value)
{
    return addr[0] = value;
}

uint8_t atomic_store_local_8(uint8_t *addr, uint8_t value)
{
    return addr[0] = value;
}

void ETSOC_Memory_Write_Local_Atomic(const void *src, void *dest, size_t size)
{
    memcpy(dest, src, size);
}
