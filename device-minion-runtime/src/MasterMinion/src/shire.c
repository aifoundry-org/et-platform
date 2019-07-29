#include "shire.h"
#include "kernel.h"

typedef struct
{
    shire_state_t shire_state;
    kernel_id_t kernel_id;
} shire_status_t;

// Local state
static shire_status_t shire_status[33];

void update_shire_state(uint64_t shire, shire_state_t shire_state)
{
    // TODO FIXME this is hokey, clean up state handling.
    if (shire_state == SHIRE_STATE_COMPLETE)
    {
        shire_status[shire].shire_state = SHIRE_STATE_READY;
    }
    else
    {
        shire_status[shire].shire_state = shire_state;
    }
}

bool all_shires_ready(uint64_t shire_mask)
{
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        if (shire_mask & (1ULL << shire))
        {
            if (shire_status[shire].shire_state != SHIRE_STATE_READY)
            {
                return false;
            }
        }
    }

    return true;
}

bool all_shires_complete(uint64_t shire_mask)
{
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        if (shire_mask & (1ULL << shire))
        {
            if ((shire_status[shire].shire_state != SHIRE_STATE_COMPLETE) &&
                (shire_status[shire].shire_state != SHIRE_STATE_READY))
            {
                return false;
            }
        }
    }

    return true;
}

void set_shire_kernel_id(uint64_t shire, kernel_id_t kernel_id)
{
    shire_status[shire].kernel_id = kernel_id;
}

kernel_id_t get_shire_kernel_id(uint64_t shire)
{
    return shire_status[shire].kernel_id;
}
