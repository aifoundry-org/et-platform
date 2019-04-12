#include "syscall.h"
#include "cacheops.h"
#include "shire.h"

#include <stdint.h>

int syscall_handler(int syscall);

static void evict_l1_to_l2(void);
static void evict_l2_to_l3(void);
static void init_l1(void);

int syscall_handler(int syscall)
{
    switch (syscall)
    {
        case SYSCALL_EVICT_L1_TO_L2:
            evict_l1_to_l2();
        break;

        case SYSCALL_EVICT_L2_TO_L3:
            evict_l2_to_l3();
        break;

        case SYSCALL_INIT_L1:
            init_l1();
        break;

        case SYSCALL_U_MODE:
            // drop to user mode
        break;

        case SYSCALL_S_MODE:
            // drop to supervisor mode if called from machine mode
        break;

        case SYSCALL_DRAIN_COALESCING_BUFFER:
            // todo
        break;

        default:
            // unhandled syscall!
        break;
    }

    return 0;
}

// Can only be run from M-mode
static void evict_l1_to_l2(void)
{
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    const uint64_t mcache_control_reg = mcache_control_get();

    excl_mode(1); // get exclusive access to the processor

    if ((mcache_control_reg & 0x3) == 3)
    {
        // Split mode with scratchpad enabled. Evict sets 12-15
        evict_sw(0, 1, 0, 12, 3, 0);
        evict_sw(0, 1, 1, 12, 3, 0);
        evict_sw(0, 1, 2, 12, 3, 0);
        evict_sw(0, 1, 3, 12, 3, 0);
    }
    else if ((mcache_control_reg & 0x3) == 1)
    {
        // Split mode with scratchpad disabled. Evict sets 0-7 and 14-15
        evict_sw(0, 1, 0, 0, 7, 0);
        evict_sw(0, 1, 1, 0, 7, 0);
        evict_sw(0, 1, 2, 0, 7, 0);
        evict_sw(0, 1, 3, 0, 7, 0);

        evict_sw(0, 1, 0, 14, 1, 0);
        evict_sw(0, 1, 1, 14, 1, 0);
        evict_sw(0, 1, 2, 14, 1, 0);
        evict_sw(0, 1, 3, 14, 1, 0);
    }
    else
    {
        // Shared mode. Evict all sets 0-15
        evict_sw(0, 1, 0, 0, 15, 0);
        evict_sw(0, 1, 1, 0, 15, 0);
        evict_sw(0, 1, 2, 0, 15, 0);
        evict_sw(0, 1, 3, 0, 15, 0);
    }

    WAIT_CACHEOPS
    FENCE

    excl_mode(0); // release exclusive access to the processor
}

static void evict_l2_to_l3(void)
{
    // TODO
}

// Can only be run from M-mode
static void init_l1(void)
{
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    const uint64_t mcache_control_reg = mcache_control_get();

    // If the cache has split or scratchpad enabled, reset to shared mode
    if ((mcache_control_reg & 0x3) != 0)
    {
        // Disable L1 split and scratchpad per PRM-8 Cache Control Extension section 1.1.3
        // "Changing configuration from Split back to Shared" and "Enabling/Disabling the Scratchpad"
        excl_mode(1); // get exclusive access to the processor

        if ((mcache_control_reg & 0x3) == 3)
        {
            // Split mode with scratchpad enabled
            // No need to unlock lines: we will turn D1Split off which resets all sets
            // Evict sets 12-15 (12-13 for HART 0, 14-15 for HART1)
            evict_sw(0, 1, 0, 12, 3, 0);
            evict_sw(0, 1, 1, 12, 3, 0);
            evict_sw(0, 1, 2, 12, 3, 0);
            evict_sw(0, 1, 3, 12, 3, 0);
        }
        else if ((mcache_control_reg & 0x3) == 1)
        {
            // Split mode with scratchpad disabled
            // No need to unlock lines: we will turn D1Split off which resets all sets
            // Evict sets 0-7 for HART0
            evict_sw(0, 1, 0, 0, 7, 0);
            evict_sw(0, 1, 1, 0, 7, 0);
            evict_sw(0, 1, 2, 0, 7, 0);
            evict_sw(0, 1, 3, 0, 7, 0);

            // Evict sets 14-15 for HART1
            evict_sw(0, 1, 0, 14, 1, 0);
            evict_sw(0, 1, 1, 14, 1, 0);
            evict_sw(0, 1, 2, 14, 1, 0);
            evict_sw(0, 1, 3, 14, 1, 0);
        }
        else
        {
            // This should never happen
            asm volatile ("ebreak");
        }

        WAIT_CACHEOPS
        FENCE

        // Comments in PRM-8 section 1.1.1:
        // "Missing specification of what happens when going from shared to split mode and vice versa. Do we clear the whole cache?
        //  The RTL implements the correct behaviour that should be specified here, which is to clear all sets when changing to/from shared mode."
        mcache_control(0, 0, 0); // Write 0 into mcache_control.D1Split

        excl_mode(0); // release exclusive access to the processor
    }
    else
    {
        // Cache is in shared mode
        // Unlock sets 0-15
        for (uint64_t set = 0; set < 16; set++)
        {
            for (uint64_t way = 0; way < 4; way++)
            {
                unlock_sw(way, set, 0);
            }
        }
    }
}
