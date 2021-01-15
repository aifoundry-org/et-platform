#include "shire.h"
#include "kernel.h"
#include "log.h"

typedef struct {
    shire_state_t shire_state;
    kernel_id_t kernel_id;
} shire_status_t;

// Local state
static shire_status_t shire_status[NUM_SHIRES] __attribute__((aligned(64))) = { 0 };
static uint64_t g_functional_shires __attribute__((aligned(64))) = 0;
static uint64_t booted_shires __attribute__((aligned(64))) = 0;

void set_functional_shires(uint64_t mask)
{
    g_functional_shires = mask;
}

uint64_t get_functional_shires(void)
{
    return g_functional_shires;
}

void update_shire_state(uint64_t shire, shire_state_t shire_state)
{
    const shire_state_t current_state = shire_status[shire].shire_state;

    // TODO FIXME this is hokey, clean up state handling.
    if (shire_state == SHIRE_STATE_COMPLETE) {
        shire_status[shire].shire_state = SHIRE_STATE_READY;
    } else {
        if (current_state != SHIRE_STATE_ERROR) {
            shire_status[shire].shire_state = shire_state;
            // Update mask of booted shires (coming from SHIRE_STATE_UNKNOWN)
            booted_shires |= 1ULL << shire;
        } else {
            // The only legal transition from ERROR state is to READY state
            if (shire_state == SHIRE_STATE_READY) {
                shire_status[shire].shire_state = shire_state;
            } else {
                log_write(LOG_LEVEL_ERROR, "Error illegal shire %d state transition from error\r\n",
                          shire);
            }
        }
    }
}

bool all_shires_booted(uint64_t shire_mask)
{
    return (booted_shires & shire_mask) == shire_mask;
}

bool all_shires_ready(uint64_t shire_mask)
{
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
        if (shire_mask & (1ULL << shire)) {
            if (shire_status[shire].shire_state != SHIRE_STATE_READY) {
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
