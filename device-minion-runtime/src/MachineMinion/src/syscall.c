#include "syscall.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "fcc.h"
#include "flb.h"
#include "printf.h"
#include "shire.h"

#include <stdint.h>

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static int64_t pre_kernel_setup(uint64_t arg1);
static int64_t post_kernel_cleanup(void);

static inline void idx_cop_sm_ctl_wait_idle(volatile const uint64_t * const idx_cop_sm_ctl_ptr);
static inline void drain_coalescing_buffer(uint64_t shire, uint64_t bank);
static int64_t drain_coalescing_buffer_with_params(uint64_t params, uint64_t hart_mask);

static int64_t evict_l1(void);
static int64_t init_l1(void);

static int64_t evict_l2_start(void);
static int64_t evict_l2_wait(void);
static int64_t evict_l2(void);

static int64_t enable_thread1(uint64_t hart_disable_mask);
static int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t parameters);
static int64_t ipi_trigger(uint64_t shire_id, uint64_t hart_mask);

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t rv;

    switch (number)
    {
        case SYSCALL_PRE_KERNEL_SETUP:
            rv = pre_kernel_setup(arg1);
        break;

        case SYSCALL_POST_KERNEL_CLEANUP:
            rv = post_kernel_cleanup();
        break;

        case SYSCALL_EVICT_L1:
            rv = evict_l1();
        break;

        case SYSCALL_EVICT_L2:
            rv = evict_l2();
        break;

        case SYSCALL_EVICT_L2_WAIT:
            rv = evict_l2_wait();
        break;

        case SYSCALL_INIT_L1:
            rv = init_l1();
        break;

        case SYSCALL_DRAIN_COALESCING_BUFFER:
            rv = drain_coalescing_buffer_with_params(arg1, arg2);
        break;

        case SYSCALL_ENABLE_THREAD1:
            rv = enable_thread1(arg1);
        break;

        case SYSCALL_BROADCAST:
            rv = broadcast(arg1, arg2, arg3);
        break;

        case SYSCALL_IPI_TRIGGER:
            rv = ipi_trigger(arg1, arg2);
        break;

        case SYSCALL_RETURN_FROM_KERNEL:
            rv = -1; // supervisor firmware should never ask the machine code to do this
        break;

        default:
            rv = -1; // unhandled syscall! Ignoring for now.
        break;
    }

    return rv;
}

// All the M-mode only work that needs to be done before a kernel launch
// to avoid the overhead of making multiple syscalls
static int64_t pre_kernel_setup(uint64_t arg1)
{
    // First HART in the shire
    if (get_hart_id() % 64U == 0U)
    {
        enable_thread1(arg1);
    }

    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        // Disable L1 split and scratchpad. Unlocks and evicts all lines.
        init_l1();
    }

    return 0;
}

// All the M-mode only work that needs to be done after a kernel returns
// to avoid the overhead of making multiple syscalls
static int64_t post_kernel_cleanup(void)
{
    bool result;

    // Thread 0 in each minion evicts the L1 cache
    if (get_thread_id() == 0U)
    {
        evict_l1();
    }

    // Wait for all L1 evicts to complete before evicting L2
    WAIT_FLB(64, 31, result);

    // Last thread to join barrier evicts L2
    // A full L2 evict includes flushing the coalescing buffer
    if (result)
    {
        evict_l2();
    }

    return 0;
}

static inline void idx_cop_sm_ctl_wait_idle(volatile const uint64_t * const idx_cop_sm_ctl_ptr)
{
    uint64_t state;

    do {
        state = (*idx_cop_sm_ctl_ptr >> 24) & 0xFF;
    } while (state != 4);
}

static inline void drain_coalescing_buffer(uint64_t shire, uint64_t bank)
{
    volatile uint64_t* const idx_cop_sm_ctl_ptr = ESR_CACHE(PRV_M, shire, bank, IDX_COP_SM_CTL);

    idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_ptr);

    *idx_cop_sm_ctl_ptr = (1ULL << 0) | // Go bit = 1
                          (10ULL << 8); // Opcode = CB_Inv (Coalescing buffer invalidate)

    idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_ptr);
}

static int64_t drain_coalescing_buffer_with_params(uint64_t params, uint64_t hart_mask)
{
    const uint64_t before_fcc_consume = (params >> 0) & 1;
    const uint64_t before_fcc_reg = (params >> 1) & 1;
    const uint64_t drain_shire = (params >> 2) & 0xFF;
    const uint64_t drain_bank = (params >> 10) & 3;
    const uint64_t after_flb = (params >> 12) & 1;
    const uint64_t after_flb_num = (params >> 13) & 0x1F;
    const uint64_t after_flb_match = (params >> 18) & 0xFF;
    const uint64_t after_fcc_send = (params >> 26) & 1;
    const uint64_t after_fcc_shire = (params >> 27) & 0xFF;
    const uint64_t after_fcc_reg = (params >> 35) & 1;
    const uint64_t after_fcc_thread = (params >> 36) & 1;
    uint64_t flb_last = 0;

    // Wait until we receive a FCC credit from the compute minions
    if (before_fcc_consume)
    {
        wait_fcc(before_fcc_reg);
    }

    // Drain the coalescing buffer of shire cache bank
    drain_coalescing_buffer(drain_shire, drain_bank);

    // Wait until all the other C.B. drainer threads are done
    if (after_flb)
    {
         WAIT_FLB(after_flb_match, after_flb_num, flb_last);
    }

    // Send the FCC to sync minions so they know the layer is done
    if (after_fcc_send && flb_last)
    {
        SEND_FCC(after_fcc_shire, after_fcc_thread, after_fcc_reg, hart_mask);
    }

    return 0;
}

static int64_t evict_l1(void)
{
    int64_t rv;
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
    else if ((mcache_control_reg & 0x3) == 0)
    {
        // Shared mode. Evict all sets 0-15
        evict_sw(0, 1, 0, 0, 15, 0);
        evict_sw(0, 1, 1, 0, 15, 0);
        evict_sw(0, 1, 2, 0, 15, 0);
        evict_sw(0, 1, 3, 0, 15, 0);
    }
    else
    {
        rv = -1;
    }

    WAIT_CACHEOPS
    FENCE

    excl_mode(0); // release exclusive access to the processor

    return rv;
}

static int64_t init_l1(void)
{
    int64_t rv = 0;

    uint64_t mcache_control_reg = mcache_control_get();

    // Enable L1 split and scratchpad per PRM-8 Cache Control Extension section 1.1.3
    // "Changing configuration Shared to Split" and "Enabling/Disabling the Scratchpad"
    excl_mode(1); // get exclusive access to the processor

    // If the cache is not in split mode with scratchpad enabled, change configuration to split mode with scratchpad enabled
    if ((mcache_control_reg & 0x3) != 3)
    {
        bool allSetsReset = false;

        // If split isn't enabled, enable it
        if ((mcache_control_reg & 0x3) == 0)
        {
            // Shared mode
            // Comments in PRM-8 section 1.1.1:
            // "Missing specification of what happens when going from shared to
            //  split mode and vice versa. Do we clear the whole cache?
            //  The RTL implements the correct behaviour that should be specified here,
            //  which is to clear all sets when changing to/from shared mode."

            // No need to unlock lines: we will set D1Split which resets all sets
            // evict all sets (0-15 dynamically shared between HART0 and HART1)
            evict_sw(0, 1, 0, 0, 15, 0);
            evict_sw(0, 1, 1, 0, 15, 0);
            evict_sw(0, 1, 2, 0, 15, 0);
            evict_sw(0, 1, 3, 0, 15, 0);

            WAIT_CACHEOPS
            FENCE

            mcache_control(1, 0, 0); // Enable split mode
            allSetsReset = true; // No need to subsequently unlock lines - enabling split mode reset all sets
        }

        mcache_control_reg = mcache_control_get();

        // If scratchpad isn't enabled, enable it
        if ((mcache_control_reg & 0x3) == 1)
        {
            // Split mode with scratchpad disabled
            // PRM-8 section 1.1.1:
            // "When D1Split is 1 and the SCPEnable changes value (0->1 or 1->0),
            //  sets 0-13 in the cache are unconditionally invalidated, i.e.,
            //  all the cache contents for those 14 sets are lost.
            //  If there were any soft- or hard- locked lines within those cache lines,
            //  those locks are cleared. Additionally, all sets 0-13 are zeroed out.
            //  If software wishes to preserve the contents of sets 0-13, it should use
            //  some of the Evict/Flush instructions before enabling/disabling the scratchpad.""

            if (!allSetsReset)
            {
                // Unlock sets 14-15: enabling scratchpad will reset sets 0-13
                for (uint64_t set = 14; set < 16; set++)
                {
                    for (uint64_t way = 0; way < 4; way++)
                    {
                        unlock_sw(way, set, 0);
                    }
                }
            }

            // Evict sets 0-13 for HART0
            evict_sw(0, 1, 0, 0, 13, 0);
            evict_sw(0, 1, 1, 0, 13, 0);
            evict_sw(0, 1, 2, 0, 13, 0);
            evict_sw(0, 1, 3, 0, 13, 0);

            WAIT_CACHEOPS
            FENCE

            mcache_control(1, 1, 0); // Enable scratchpad
        }
    }
    else
    {
        // Split mode with scratchpad enabled
        // Unlock sets 12-15 (12-13 for HART 0, 14-15 for HART1)
        for (uint64_t set = 12; set < 16; set++)
        {
            for (uint64_t way = 0; way < 4; way++)
            {
                unlock_sw(way, set, 0);
            }
        }
    }

    excl_mode(0); // release exclusive access to the processor

    return rv;
}

static int64_t evict_l2_start(void)
{
    for (uint64_t i = 0; i < 4; i++)
    {
        volatile uint64_t* const idx_cop_sm_ctl_ptr = ESR_CACHE(PRV_M, 0xFF, i, IDX_COP_SM_CTL);
        idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_ptr);
    }

    // Broadcast L2 evict to all 4 banks
    volatile uint64_t* const idx_cop_sm_ctl_ptr = ESR_CACHE(PRV_M, 0xFF, 0xF, IDX_COP_SM_CTL);

    *idx_cop_sm_ctl_ptr = (1ULL << 0) | // Go bit = 1
                          (3ULL << 8);  // Opcode = L2_Evict (Evicts all L2 indexes)

    return 0;
}

static int64_t evict_l2_wait(void)
{
    for (uint64_t i = 0; i < 4; i++)
    {
        volatile const uint64_t* const idx_cop_sm_ctl_ptr = ESR_CACHE(PRV_M, 0xFF, i, IDX_COP_SM_CTL);
        idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_ptr);
    }

    return 0;
}

static int64_t evict_l2(void)
{
    int64_t rv;

    rv = evict_l2_start();

    if (rv >= 0)
    {
        rv = evict_l2_wait();
    }

    return rv;
}

// hart_disable_mask is a bitmask to DISABLE a thread1: bit 0 = minion 0, bit 31 = minion 31
static int64_t enable_thread1(uint64_t hart_disable_mask)
{
    volatile uint64_t* const enable_thread1_ptr = ESR_SHIRE(PRV_M, THIS_SHIRE, THREAD1_DISABLE);
    *enable_thread1_ptr = hart_disable_mask;

    return 0;
}

static int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t parameters)
{
    // privilege of write to BROADCAST1 ESR must match privilege encoded in parameters
    const uint64_t priv = (parameters & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT;

    volatile uint64_t* const broadcast_esr_ptr = ESR_SHIRE(PRV_U, THIS_SHIRE, BROADCAST0);
    volatile uint64_t* const broadcast_req_ptr = ESR_SHIRE(priv,  THIS_SHIRE, BROADCAST1);

    *broadcast_esr_ptr = value;
    *broadcast_req_ptr = parameters | (shire_mask & ESR_BROADCAST_ESR_SHIRE_MASK);

    return 0;
}

// hart_mask = bit 0 in the mask corresponds to hart 0 in the shire (minion 0, thread 0), bit 1 to hart 1 (minion 0, thread 1) and bit 63 corresponds to hart 63 (minion 31, thread 1).
// shire_id = shire to send the credit to, 0-32 or 0xFF for "this shire"
static int64_t ipi_trigger(uint64_t shire_id, uint64_t hart_mask)
{
    volatile uint64_t* const ipi_trigger_ptr = ESR_SHIRE(PRV_M, shire_id, IPI_TRIGGER);
    *ipi_trigger_ptr = hart_mask;

    return 0;
}
