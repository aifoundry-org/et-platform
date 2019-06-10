#ifndef BROADCAST_H
#define BROADCAST_H

#include "esr_defines.h"

#include <stdint.h>

inline uint64_t broadcast_encode_parameters(uint64_t pp, uint64_t region, uint64_t address)
{
    // pp argument is a raw field value

    // region argument comes from #define with both bit 32 and bits [21:17] set, e.g.
    // #define ESR_SHIRE_REGION 0x0100340000ULL // Shire ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010
    // so mask bits [21:17] via ESR_SREGION_EXT_MASK, then shift up ESR_SREGION_EXT_SHIFT bits less

    // address argument comes from #define with bits [16:0] e.g.
    // #define ESR_SHIRE_BROADCAST0 0x1FFF0
    // but ESR_BROADCAST_ESR_ADDR_MASK only wants bits [16:3] so shift up ESR_ESR_ID_SHIFT bits less

    return ((pp << ESR_BROADCAST_PROT_SHIFT) & ESR_BROADCAST_PROT_MASK) |
           (((region & ESR_SREGION_EXT_MASK) << (ESR_BROADCAST_ESR_SREGION_MASK_SHIFT - ESR_SREGION_EXT_SHIFT)) & ESR_BROADCAST_ESR_SREGION_MASK) |
           ((address << (ESR_BROADCAST_ESR_ADDR_SHIFT - ESR_ESR_ID_SHIFT)) & ESR_BROADCAST_ESR_ADDR_MASK);
}

#endif
