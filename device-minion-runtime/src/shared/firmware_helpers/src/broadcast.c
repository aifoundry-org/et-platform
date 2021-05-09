#include "broadcast.h"
#include "device-common/esr_defines.h"
#include "device-common/hart.h"

inline uint64_t broadcast_encode_parameters(uint64_t pp, uint64_t region, uint64_t esr_id)
{
    // pp argument is a raw field value

    // region argument comes from #define with both bit 32 and bits [21:17] set, e.g.
    // #define ESR_SHIRE_REGION 0x0100340000ULL // Shire ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010
    // so mask bits [21:17] via ESR_SREGION_EXT_MASK, then shift up ESR_SREGION_EXT_SHIFT bits less

    // esr_id argument comes from #define _REGNO
    // so bits [x:3]

    return ((pp << ESR_BROADCAST_PROT_SHIFT) & ESR_BROADCAST_PROT_MASK) |
           (((region & ESR_SREGION_EXT_MASK)
             << (ESR_BROADCAST_ESR_SREGION_MASK_SHIFT - ESR_SREGION_EXT_SHIFT)) &
            ESR_BROADCAST_ESR_SREGION_MASK) |
           ((esr_id << ESR_BROADCAST_ESR_ADDR_SHIFT) & ESR_BROADCAST_ESR_ADDR_MASK);
}

int64_t broadcast_with_parameters(uint64_t value, uint64_t shire_mask, uint64_t parameters)
{
    // privilege of write to BROADCAST1 ESR must match privilege encoded in parameters
    const uint64_t priv = (parameters & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT;

    volatile uint64_t *const broadcast_esr_ptr =
        (volatile uint64_t *)ESR_SHIRE_PROT_ADDR(PRV_U, THIS_SHIRE, ESR_SHIRE_BROADCAST0);
    volatile uint64_t *const broadcast_req_ptr =
        (volatile uint64_t *)ESR_SHIRE_PROT_ADDR(priv, THIS_SHIRE, ESR_SHIRE_BROADCAST1);

    *broadcast_esr_ptr = value;
    *broadcast_req_ptr = parameters | (shire_mask & ESR_BROADCAST_ESR_SHIRE_MASK);

    return 0;
}

int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t priv, uint64_t region,
                  uint64_t esr_id)
{
    // privilege of write to BROADCAST1 ESR must match privilege encoded in parameters
    const uint64_t parameters = broadcast_encode_parameters(priv, region, esr_id);

    return broadcast_with_parameters(value, shire_mask, parameters);
}
