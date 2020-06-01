#include "syscall_internal.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "exception.h"
#include "fcc.h"
#include "flb.h"
#include "hal_device.h"
#include "hart.h"
#include "layout.h"
#include "printf.h"

#include <stdint.h>

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static int64_t ipi_trigger(uint64_t hart_mask, uint64_t shire_id);
static int64_t enable_thread1(uint64_t disable_mask, uint64_t enable_mask);

static int64_t pre_kernel_setup(uint64_t hart_enable_mask, uint64_t first_worker);
static int64_t post_kernel_cleanup(uint64_t thread_count);

static int64_t init_l1(void);
static inline void evict_all_l1_ways(uint64_t use_tmask, uint64_t dest_level, uint64_t set, uint64_t num_sets);
static int64_t evict_l1(uint64_t use_tmask, uint64_t dest_level);

static int64_t evict_l2_start(void);
static int64_t evict_l2_wait(void);
static int64_t evict_l2(void);

static int64_t flush_l3(void);
static int64_t evict_l3(void);

static inline void sc_idx_cop_sm_ctl_all_banks_go(uint64_t opcode);
static inline void sc_idx_cop_sm_ctl_all_banks_wait_idle(void);
static inline void sc_idx_cop_sm_ctl_go(volatile uint64_t* const addr, uint64_t opcode);
static inline void sc_idx_cop_sm_ctl_wait_idle(volatile const uint64_t * const idx_cop_sm_ctl_ptr);

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op);

static inline void l1_shared_to_split(uint64_t scp_en, uint64_t cacheop);
static inline void l1_split_to_shared(uint64_t scp_enabled, uint64_t cacheop);
static int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en);

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t ret;
    volatile const uint64_t * const mtime_reg = (volatile const uint64_t * const)(R_PU_RVTIM_BASEADDR);

    switch (number) {
        case SYSCALL_BROADCAST_INT:
            ret = broadcast_with_parameters(arg1, arg2, arg3);
            break;
        case SYSCALL_IPI_TRIGGER_INT:
            ret = ipi_trigger(arg1, arg2);
            break;
        case SYSCALL_ENABLE_THREAD1_INT:
            ret = enable_thread1(arg1, arg2);
            break;
        case SYSCALL_PRE_KERNEL_SETUP_INT:
            ret = pre_kernel_setup(arg1, arg2);
            break;
        case SYSCALL_POST_KERNEL_CLEANUP_INT:
            ret = post_kernel_cleanup(arg1);
            break;
        case SYSCALL_GET_MTIME_INT:
            ret = (int64_t)*mtime_reg;
            break;
        case SYSCALL_CACHE_CONTROL_INT:
            ret = set_l1_cache_control(arg1, arg2);
            break;
        case SYSCALL_FLUSH_L3_INT:
            ret = flush_l3();
            break;
        case SYSCALL_SHIRE_CACHE_BANK_OP_INT:
            ret = shire_cache_bank_op_with_params(arg1, arg2, arg3);
            break;
        case SYSCALL_EVICT_L1_INT:
            ret = evict_l1(arg1, arg2);
            break;
        default:
            ret = -1; // unhandled syscall! Ignoring for now.
            break;
    }

    return ret;
}

// hart_mask = bit 0 in the mask corresponds to hart 0 in the shire (minion 0, thread 0), bit 1 to hart 1 (minion 0, thread 1) and bit 63 corresponds to hart 63 (minion 31, thread 1).
// shire_id = shire to send the credit to, 0-32 or 0xFF for "this shire"
static int64_t ipi_trigger(uint64_t hart_mask, uint64_t shire_id)
{
    volatile uint64_t* const ipi_trigger_ptr = (volatile uint64_t *)ESR_SHIRE(shire_id, IPI_TRIGGER);
    *ipi_trigger_ptr = hart_mask;

    return 0;
}

// disable_mask is a bitmask to DISABLE a thread1: bit 0 = minion 0, bit 31 = minion 31
// enable_mask is a bitmask to ENABLE a thread1: bit 0 = minion 0, bit 31 = minion 31
static int64_t enable_thread1(uint64_t disable_mask, uint64_t enable_mask)
{
    volatile uint64_t* const disable_thread1_ptr = (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, THREAD1_DISABLE);
    *disable_thread1_ptr = (*disable_thread1_ptr & ~enable_mask) | disable_mask;

    return 0;
}

// All the M-mode only work that needs to be done before a kernel launch
// to avoid the overhead of making multiple syscalls
static int64_t pre_kernel_setup(uint64_t thread1_enable_mask, uint64_t first_worker)
{
    // First worker HART in the shire
    if ((get_hart_id() % 64U) == first_worker)
    {
        enable_thread1(0, thread1_enable_mask);
    }

    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        // Disable L1 split and scratchpad. Unlocks and evicts all lines.
        init_l1();
    }

    // First minion in each neighborhood
    if (get_hart_id() % 8 == 0U)
    {
        // Invalidate shared L1 I-cache
        asm volatile ("csrw cache_invalidate, 1");
    }

    return 0;
}

// All the M-mode only work that needs to be done after a kernel returns
// to avoid the overhead of making multiple syscalls
static int64_t post_kernel_cleanup(uint64_t thread_count)
{
    bool result;

    // Thread 0 in each minion evicts L1
    if (get_thread_id() == 0)
    {
        evict_l1(0, to_L2);
    }

    // Wait for all L1 evicts to complete before evicting L2
    WAIT_FLB(thread_count, 30, result);

    // Last thread in shire to join barrier evicts L2
    // A full L2 evict includes flushing the coalescing buffer
    if (result)
    {
        evict_l2();
        evict_l3();
    }

    return 0;
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
            evict_all_l1_ways(0, to_L2, 0, 15);

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
            evict_all_l1_ways(0, to_L2, 0, 13);

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


// evict all 4 ways of a given group of L1 sets up to dest_level
static inline void evict_all_l1_ways(uint64_t use_tmask, uint64_t dest_level, uint64_t set, uint64_t num_sets)
{
    evict_sw(use_tmask, dest_level, 0, set, num_sets, 0);
    evict_sw(use_tmask, dest_level, 1, set, num_sets, 0);
    evict_sw(use_tmask, dest_level, 2, set, num_sets, 0);
    evict_sw(use_tmask, dest_level, 3, set, num_sets, 0);
}

static int64_t evict_l1_all(uint64_t use_tmask, uint64_t dest_level)
{
    int64_t rv;
    const uint64_t mcache_control_reg = mcache_control_get();

    if ((mcache_control_reg & 0x3) == 3)
    {
        // Split mode with scratchpad enabled. Evict sets 12-15
        evict_all_l1_ways(use_tmask, dest_level, 12, 3);

    }
    else if ((mcache_control_reg & 0x3) == 1)
    {
        // Split mode with scratchpad disabled. Evict sets 0-7 and 14-15
        evict_all_l1_ways(use_tmask, dest_level, 0, 7);
        evict_all_l1_ways(use_tmask, dest_level, 14, 1);
    }
    else if ((mcache_control_reg & 0x3) == 0)
    {
        // Shared mode. Evict all sets 0-15
        evict_all_l1_ways(use_tmask, dest_level, 0, 15);
    }
    else
    {
        rv = -1;
    }

    WAIT_CACHEOPS
    FENCE

    return rv;
}

static int64_t evict_l1(uint64_t use_tmask, uint64_t dest_level)
{
    int64_t rv;

    // Gain exclusive access to the processor
    excl_mode(1);

    // Evict all cache lines from the L1 to a given level
    rv = evict_l1_all(use_tmask, dest_level);

    // Release the hold of the processor
    excl_mode(0);

    return rv;
}

static int64_t evict_l2_start(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle();
    sc_idx_cop_sm_ctl_all_banks_go(3); // Opcode = L2_Evict (Evicts all L2 indexes)

    return 0;
}

static int64_t evict_l2_wait(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle();

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

static int64_t flush_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle();
    sc_idx_cop_sm_ctl_all_banks_go(5); // Opcode = L3_Flush (Flushes all L3 indexes)
    sc_idx_cop_sm_ctl_all_banks_wait_idle();

    return 0;
}

static int64_t evict_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle();
    sc_idx_cop_sm_ctl_all_banks_go(6); // Opcode = L3_Evict (Evicts all L3 indexes)
    sc_idx_cop_sm_ctl_all_banks_wait_idle();

    return 0;
}

static inline void sc_idx_cop_sm_ctl_all_banks_go(uint64_t opcode)
{
    // Broadcast to all 4 banks
    volatile uint64_t* const addr = (volatile uint64_t *)ESR_CACHE(THIS_SHIRE, 0xF, SC_IDX_COP_SM_CTL);
    sc_idx_cop_sm_ctl_go(addr, opcode);
}

static inline void sc_idx_cop_sm_ctl_all_banks_wait_idle(void)
{
    for (uint64_t i = 0; i < 4; i++)
    {
        volatile uint64_t* const addr = (volatile uint64_t *)ESR_CACHE(THIS_SHIRE, i, SC_IDX_COP_SM_CTL);
        sc_idx_cop_sm_ctl_wait_idle(addr);
    }
}

static inline void sc_idx_cop_sm_ctl_go(volatile uint64_t* const addr, uint64_t opcode)
{
    *addr = (1ULL           << 0) | // Go bit = 1
            ((opcode & 0xF) << 8);  // Opcode
}

static inline void sc_idx_cop_sm_ctl_wait_idle(volatile const uint64_t* const addr)
{
    uint64_t state;

    do {
        state = (*addr >> 24) & 0xFF;
    } while (state != 4);
}

static inline void shire_cache_bank_op(uint64_t shire, uint64_t bank, uint64_t op)
{
    volatile uint64_t* const addr = (volatile uint64_t *)ESR_CACHE(shire, bank, SC_IDX_COP_SM_CTL);

    sc_idx_cop_sm_ctl_wait_idle(addr);
    sc_idx_cop_sm_ctl_go(addr, op);
    sc_idx_cop_sm_ctl_wait_idle(addr);
}

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op)
{
    // XXX: Here we might want to forbid some harmful operations or shire(s)
    shire_cache_bank_op(shire, bank, op);

    return 0;
}

// change L1 configuration from Shared to Split, optionally with Scratchpad enabled
static inline void l1_shared_to_split(uint64_t scp_en, uint64_t cacheop)
{
    // Evict all cache lines from the L1 to L2
    evict_l1_all(0, to_L2);

    // Wait for all evictions to complete
    WAIT_CACHEOPS

    // Change L1 Dcache to split mode
    mcache_control(1, 0, cacheop);

    if (scp_en)
    {
        // Wait for the L1 cache to change its configuration
        FENCE;

        // Change L1 Dcache to split mode and SCP enabled
        mcache_control(1, 1, cacheop);
    }
}

// change L1 configuration from Split to Shared
static inline void l1_split_to_shared(uint64_t scp_enabled, uint64_t cacheop)
{
    if (scp_enabled)
    {
        // Evict sets 12-15
        evict_all_l1_ways(0, to_L2, 12, 4);
    }
    else
    {
        // Evict sets 0-7 and 14-15
        evict_all_l1_ways(0, to_L2, 0, 8);
        evict_all_l1_ways(0, to_L2, 14, 2);
    }

    // Wait for all evictions to complete
    WAIT_CACHEOPS

    // Change L1 Dcache to shared mode
    mcache_control(0, 0, cacheop);
}

static int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en)
{
    uint64_t mcache_control_reg;
    uint64_t cur_split, cur_scp_en, cur_cacheop;

    // Can't enable the SCP without splitting
    if (scp_en && !d1_split)
    {
        return -1;
    }

    // Gain exclusive access to the processor
    excl_mode(1);

    mcache_control_reg = mcache_control_get();
    cur_split = mcache_control_reg & 1;
    cur_scp_en = (mcache_control_reg >> 1) & 1;
    cur_cacheop = mcache_control_reg & 0x7DC; // CacheOp_RepRate and CacheOp_Max

    if (!cur_split)
    {
        // It is shared and we want to split
        if (d1_split)
        {
            l1_shared_to_split(scp_en, cur_cacheop);
        }
    }
    else
    {
        // It is split (maybe with SCP on) and we want shared mode
        if (!d1_split)
        {
            l1_split_to_shared(cur_scp_en, cur_cacheop);
        }
    }

    // Release the hold of the processor
    excl_mode(0);

    return 0;
}
