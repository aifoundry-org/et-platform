#ifndef __CACHE_FLUSH_OPS_H__
#define __CACHE_FLUSH_OPS_H__

#include <stdint.h>
#include <stdlib.h>

#define UNUSED_DATA_REGION 0x40000000 // we will use the SP ROM as the unused data region
#define SET_MASK           0x3F0u
#define CACHE_LINE_MASK    0x3Fu

// this will flush the entire data cache
static inline void l1_data_cache_flush_all(void)
{
    uint64_t temp;
    volatile const uint64_t *addr = (uint64_t *)UNUSED_DATA_REGION;
    temp = addr[0x00];
    temp = addr[0x08];
    temp = addr[0x10];
    temp = addr[0x18];
    temp = addr[0x20];
    temp = addr[0x28];
    temp = addr[0x30];
    temp = addr[0x38];
    temp = addr[0x40];
    temp = addr[0x48];
    temp = addr[0x50];
    temp = addr[0x58];
    temp = addr[0x60];
    temp = addr[0x68];
    temp = addr[0x70];
    temp = addr[0x78];
    temp = addr[0x80];
    temp = addr[0x88];
    temp = addr[0x90];
    temp = addr[0x98];
    temp = addr[0xA0];
    temp = addr[0xA8];
    temp = addr[0xB0];
    temp = addr[0xB8];
    temp = addr[0xC0];
    temp = addr[0xC8];
    temp = addr[0xD0];
    temp = addr[0xD8];
    temp = addr[0xE0];
    temp = addr[0xE8];
    temp = addr[0xF0];
    temp = addr[0xF8];
    temp = addr[0x100];
    temp = addr[0x108];
    temp = addr[0x110];
    temp = addr[0x118];
    temp = addr[0x120];
    temp = addr[0x128];
    temp = addr[0x130];
    temp = addr[0x138];
    temp = addr[0x140];
    temp = addr[0x148];
    temp = addr[0x150];
    temp = addr[0x158];
    temp = addr[0x160];
    temp = addr[0x168];
    temp = addr[0x170];
    temp = addr[0x178];
    temp = addr[0x180];
    temp = addr[0x188];
    temp = addr[0x190];
    temp = addr[0x198];
    temp = addr[0x1A0];
    temp = addr[0x1A8];
    temp = addr[0x1B0];
    temp = addr[0x1B8];
    temp = addr[0x1C0];
    temp = addr[0x1C8];
    temp = addr[0x1D0];
    temp = addr[0x1D8];
    temp = addr[0x1E0];
    temp = addr[0x1E8];
    temp = addr[0x1F0];
    temp = addr[0x1F8];
    (void)temp;
}

// this will flush a single data cache line (64 bytes) that contains the specified address
static inline void l1_data_cache_flush_line(const void *address)
{
    volatile uint64_t *addr;
    uint64_t temp;
    uint64_t a = (uint64_t)address;
    a = a & SET_MASK; // isolate the set number
    a = UNUSED_DATA_REGION | a; // use the SP ROM address as the unused data region
    addr = (uint64_t *)a;
    temp = addr[0x000];
    temp = addr[0x080];
    temp = addr[0x100];
    temp = addr[0x180];
    (void)temp;
}

// this will flush all data cache lines that contain the specified region
// this might be useful if the region to flush is smaller than 1024 bytes
static inline void l1_data_cache_flush_region(const void *address, size_t size)
{
    uint32_t set;
    uint64_t a;
    uint64_t a_end;

    if (size > 0) {
        set = 0;
        a = (uint64_t)address;
        a_end = a + size - 1;
        a = a & (~CACHE_LINE_MASK);
        a_end = a_end & (~CACHE_LINE_MASK);

        while (a <= a_end) {
            l1_data_cache_flush_line((const void *)a);
            a += 64;
            set++;
            if (set >= 16) {
                break;
            }
        }
    }
}

#endif
