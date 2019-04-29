#include "syscall.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "shire.h"
#include "sync.h"

#include <stdint.h>

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static int64_t pre_kernel_setup(uint64_t arg1);
static int64_t post_kernel_cleanup(void);

static inline void idx_cop_sm_ctl_wait_idle(volatile const uint64_t * const idx_cop_sm_ctl_addr);
static inline void drain_coalescing_buffer(uint64_t shire, uint64_t bank);
static int64_t drain_coalescing_buffer_with_params(uint64_t params, uint64_t hart_mask);

static int64_t evict_l1(void);
static int64_t init_l1(void);

static int64_t evict_l2_start(void);
static int64_t evict_l2_wait(void);
static int64_t evict_l2(void);

static int64_t enable_thread1(uint64_t hart_disable_mask);
static int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t parameters);
static int64_t ipi_trigger(uint64_t value, uint64_t shire_id);

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
            rv = -1; // unhandled syscall! Igoring for now.
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

    // TODO FIXME not draining coalescing buffer for now, see SW-280
    // const uint64_t hart_id = get_hart_id();

    // First HART in each neighborhood drains the neighborhood's coalescing buffer
    // if (hart_id % 16U == 0U)
    // {
    //     drain_coalescing_buffer(THIS_SHIRE, hart_id / 16U);
    // }

    // Thread 0 in each minion evicts the L1 cache
    if (get_thread_id() == 0U)
    {
        evict_l1();
    }

    // Wait for all L1 evicts to complete before evicting L2
    WAIT_FLB(64, 31, result);

    // Last thread to join barrier evicts L2
    if (result)
    {
        evict_l2();
    }

    return 0;
}

static inline void idx_cop_sm_ctl_wait_idle(volatile const uint64_t * const idx_cop_sm_ctl_addr)
{
    uint64_t state;

    do {
        state = (*idx_cop_sm_ctl_addr >> 24) & 0xFF;
    } while (state != 4);
}

static inline void drain_coalescing_buffer(uint64_t shire, uint64_t bank)
{
    volatile uint64_t* const idx_cop_sm_ctl_addr = (uint64_t*)ESR_CACHE(3, shire, bank, IDX_COP_SM_CTL);

    idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_addr);

    *idx_cop_sm_ctl_addr = (1 << 0) | // Go bit = 1
                           (10 << 8); // Opcode = CB_Inv (Coalescing buffer invalidate)

    idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_addr);
}

static int64_t drain_coalescing_buffer_with_params(uint64_t params, uint64_t hart_mask)
{
    const unsigned int before_fcc_consume = (params >> 0) & 1;
    const unsigned int before_fcc_reg = (params >> 1) & 1;
    const unsigned int drain_shire = (params >> 2) & 0xFF;
    const unsigned int drain_bank = (params >> 10) & 3;
    const unsigned int after_flb = (params >> 12) & 1;
    const unsigned int after_flb_num = (params >> 13) & 0x1F;
    const unsigned int after_flb_match = (params >> 18) & 0xFF;
    const unsigned int after_fcc_send = (params >> 26) & 1;
    const unsigned int after_fcc_shire = (params >> 27) & 0xFF;
    const unsigned int after_fcc_reg = (params >> 35) & 1;
    const unsigned int after_fcc_thread = (params >> 36) & 1;
    unsigned int flb_last = 0;

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
            rv = -1;
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

    return rv;
}

static int64_t evict_l2_start(void)
{
    for (uint64_t i = 0; i < 4; i++) {
        volatile uint64_t* const idx_cop_sm_ctl_addr = (uint64_t*)ESR_CACHE(3, 0xFF, i, IDX_COP_SM_CTL);

        idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_addr);

        *idx_cop_sm_ctl_addr = (1 << 0) | // Go bit = 1
                               (3 << 8);  // Opcode = L2_Evict (Evicts all L2 indexes)
    }

    return 0;
}

static int64_t evict_l2_wait(void)
{
    for (uint64_t i = 0; i < 4; i++) {
        volatile const uint64_t* const idx_cop_sm_ctl_addr = (uint64_t*)ESR_CACHE(3, 0xFF, i, IDX_COP_SM_CTL);

        idx_cop_sm_ctl_wait_idle(idx_cop_sm_ctl_addr);
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

static int64_t enable_thread1(uint64_t hart_disable_mask)
{
    // Set 1 to DISABLE a thread 1: bit 0 = minion 0, bit 31 = minion 31
    *(volatile uint64_t* const)(0x1C0340010ULL) = hart_disable_mask;

    return 0;
}

static int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t parameters)
{
    // parameters:
    // pp (bits 35:34)
    // region (bits 33:32)
    // address (bits 16:0)
    uint64_t pp      = (parameters >> 34) & 0x3;
    uint64_t region  = (parameters >> 32) & 0x3;
    uint64_t address = parameters & 0x1FFFF;

    volatile uint64_t* const broadcast_esr_addr = (uint64_t*)0x013ff5fff0;

    *broadcast_esr_addr = value;

    volatile uint64_t* const broadcast_req_addr = (uint64_t*)(0x013ff5fff8 | (pp << 30));

    *broadcast_req_addr = (((pp & 0x03)         << 59) |
                           ((region & 0x03)     << 57) |
                           ((address & 0x1ffff) << 40) |
                           (shire_mask & 0xFFFFFFFF  ));

    return 0;
}

static int64_t ipi_trigger(uint64_t value, uint64_t shire_id)
{
    shire_id &= 0x3F;

    volatile uint64_t* const ipi_trigger_addr = (uint64_t*)(
        (1ULL << 32)     + // ESR region
        (3ULL << 30)     + // PP bits = m-mode
        (shire_id << 22) + // Going to shire_id
        (0x1AULL << 17)  + // Shire other ESRs
        (0x12ULL << 3)     // IPI trigger
    );

    *ipi_trigger_addr = value;

    return 0;
}
