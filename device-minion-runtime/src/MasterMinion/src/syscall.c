#include "syscall.h"
#include "cacheops.h"
#include "shire.h"

int syscall_handler(int syscall);

static void flush_l1_to_l2(void);
static void flush_l2_to_l3(void);
static void enable_l1_scratchpad(void);

// TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
// for now, keeping track of L1 scratchpad state internally to work around this bug
static bool l1_scratchpad_mode;

int syscall_handler(int syscall)
{
    switch (syscall)
    {
        case SYSCALL_FLUSH_L1_TO_L2:
            flush_l1_to_l2();
        break;

        case SYSCALL_FLUSH_L2_TO_L3:
            flush_l2_to_l3();
        break;

        case SYSCALL_ENABLE_L1_SCRATCHPAD:
            enable_l1_scratchpad();
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
static void flush_l1_to_l2(void)
{
    // The number of lines depends on the D1Split
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    //const unsigned int num_lines = (mcache_control_get() & 1U) ? 3 : 15;
    const unsigned int num_lines = l1_scratchpad_mode ? 3 : 15;

    excl_mode(1);                       // Write 1 into `EXCL_MODE` to gain exclusive access to the processor.
    evict_sw(0, 1, 0, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 0 to L2
    evict_sw(0, 1, 1, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 1 to L2
    evict_sw(0, 1, 2, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 2 to L2
    evict_sw(0, 1, 3, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 3 to L2
    WAIT_CACHEOPS
    excl_mode(0);                       // Write 0 into `EXCL_MODE` to release the hold of the processor.
}

static void flush_l2_to_l3(void)
{
    // TODO
}

// Can only be run from M-mode
// TODO FIXME BROKEN, CPU hangs on ret from this function
static void enable_l1_scratchpad(void)
{
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    //const unsigned int mcache_control = mcache_control_get();

    // If the dcache isn't already split with scratchpad enabled
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    //if (mcache_control & 0x3 != 0x3)
    if (!l1_scratchpad_mode)
    {
        // The number of lines depends on the D1Split
        // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
        //const unsigned int num_lines = mcache_control & 1U ? 3 : 15;
        const unsigned int num_lines = l1_scratchpad_mode ? 3 : 15;

        // Enable L1 split and scratchpad per PRM-8 Cache Control Extension section 1.1.3
        // "Changing configuration from shared to split" and "Enabling/Disabling the Scratchpad"
        excl_mode(1);                       // Write 1 into `EXCL_MODE` to gain exclusive access to the processor.
        evict_sw(0, 1, 0, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 0 to L2
        evict_sw(0, 1, 1, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 1 to L2
        evict_sw(0, 1, 2, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 2 to L2
        evict_sw(0, 1, 3, 0, num_lines, 0); // Use EvictSW to evict all cache lines from the L1 way 3 to L2
        WAIT_CACHEOPS
        mcache_control(1, 0, 0);            // Write 1 into mcache_control.D1Split
        WAIT_CACHEOPS
        mcache_control(1, 1, 0);            // Write 1 into mcache_control.SCPEnable
        WAIT_CACHEOPS
        excl_mode(0);                       // Write 0 into `EXCL_MODE` to release the hold of the processor.

        l1_scratchpad_mode = true;
    }
}
