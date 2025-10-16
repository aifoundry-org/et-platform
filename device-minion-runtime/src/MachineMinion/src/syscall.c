/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#include <stdint.h>
#include <stdbool.h>

/* minion_bl */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>
#include <transports/mm_cm_iface/broadcast.h>

/* etsoc_hal */
#include <hwinc/hal_device.h>
#include <hwinc/etsoc_neigh_esr.h>

/* minion_rt_helpers */
#include "common_utils.h"
#include "syscall_internal.h"
#include "shire_cache.h"
#include "minion_cfg.h"

/* Global variable to cleanup threads in post kernel launch phase */
static spinlock_t Kernel_Launch_Thread_Cleanup[NUM_SHIRES] = { 0 };

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* Cache Operations */
static void cache_ops_evict_sw(uint64_t csr_enc);
static void cache_ops_flush_sw(uint64_t csr_enc);
static void cache_ops_lock_sw(uint64_t csr_enc);
static void cache_ops_unlock_sw(uint64_t csr_enc);
static void cache_ops_cache_invalidate(uint64_t csr_enc);

static int64_t enable_thread1(uint64_t disable_mask, uint64_t enable_mask);

static int64_t pre_kernel_setup(uint64_t hart_enable_mask, uint64_t first_worker);
static int64_t post_kernel_cleanup(uint64_t thread_count);

static int64_t init_l1(void);
static inline void evict_all_l1_ways(
    uint64_t use_tmask, uint64_t dest_level, uint64_t set, uint64_t num_sets);
static int64_t evict_l1(uint64_t use_tmask, uint64_t dest_level);

static int64_t evict_l2(void);
static int64_t flush_l3(void);
static int64_t evict_l3(void);
static int64_t evict_l1_l2_all(void);

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op);

static inline void l1_shared_to_split(
    uint64_t scp_en, uint64_t cacheop_reprate, uint64_t cacheop_max);
static inline void l1_split_to_shared(
    uint64_t scp_enabled, uint64_t cacheop_reprate, uint64_t cacheop_max);
static int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en);

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t ret = SYSCALL_INTERNAL_SUCCESS;

    /* List of syscalls handled in fast-path (assembly):
     *   SYSCALL_IPI_TRIGGER_INT(uint64_t hart_mask, uint64_t shire_id)
     *   SYSCALL_BROADCAST_INT(uint64_t value, uint64_t shire_mask, uint64_t parameters)
     */

    switch (number)
    {
        case SYSCALL_ENABLE_THREAD1_INT:
            ret = enable_thread1(arg1, arg2);
            break;
        case SYSCALL_PRE_KERNEL_SETUP_INT:
            ret = pre_kernel_setup(arg1, arg2);
            break;
        case SYSCALL_POST_KERNEL_CLEANUP_INT:
            ret = post_kernel_cleanup(arg1);
            break;
        case SYSCALL_CACHE_CONTROL_INT:
            ret = set_l1_cache_control(arg1, arg2);
            break;
        case SYSCALL_FLUSH_L3_INT:
            ret = flush_l3();
            break;
        case SYSCALL_EVICT_L3_INT:
            ret = evict_l3();
            break;
        case SYSCALL_SHIRE_CACHE_BANK_OP_INT:
            ret = shire_cache_bank_op_with_params(arg1, arg2, arg3);
            break;
        case SYSCALL_CONFIGURE_COMPUTE_MINION:
            ret = configure_compute_minion(arg1, arg2);
            break;
        case SYSCALL_DISABLE_NEIGH:
            disable_neigh(arg1);
            break;
        case SYSCALL_ENABLE_NEIGH:
            ret = enable_neigh(arg1);
            break;
        case SYSCALL_UPDATE_MINION_PLL_FREQUENCY:
            ret = dynamic_minion_pll_frequency_update(arg1);
            break;
        case SYSCALL_CACHE_OPS_EVICT_SW_INT:
            cache_ops_evict_sw(arg1);
            break;
        case SYSCALL_CACHE_OPS_FLUSH_SW_INT:
            cache_ops_flush_sw(arg1);
            break;
        case SYSCALL_CACHE_OPS_LOCK_SW_INT:
            cache_ops_lock_sw(arg1);
            break;
        case SYSCALL_CACHE_OPS_UNLOCK_SW_INT:
            cache_ops_unlock_sw(arg1);
            break;
        case SYSCALL_CACHE_OPS_INVALIDATE_INT:
            cache_ops_cache_invalidate(arg1);
            break;
        case SYSCALL_CACHE_OPS_EVICT_L1_INT:
            ret = evict_l1(arg1, arg2);
            break;
        case SYSCALL_CACHE_OPS_EVICT_WHOLE_L1_L2_INT:
            ret = evict_l1_l2_all();
            break;
        case SYSCALL_PMC_SC_SAMPLE_INT:
            ret = (int64_t)sample_sc_pmcs(arg1, arg2, arg3);
            break;
        case SYSCALL_PMC_MS_SAMPLE_INT:
            ret = (int64_t)sample_ms_pmcs(arg1, arg2);
            break;
        case SYSCALL_PMC_SC_SAMPLE_ALL_INT:
            ret = sample_sc_pmcs_all(arg1, arg2, (shire_pmc_cnt_t *)arg3);
            break;
        case SYSCALL_PMC_MS_SAMPLE_ALL_INT:
            ret = sample_ms_pmcs_all(arg1, (shire_pmc_cnt_t *)arg2);
            break;
        default:
            ret = SYSCALL_INTERNAL_INVALID_ID;
            break;
    }

    return ret;
}

/************************************************************************
*
*   FUNCTION
*
*       cache_ops_evict_sw
*
*   DESCRIPTION
*
*       Local fn helper to write to evict SW CSR.
*
*   INPUTS
*
*       csr_enc        Value to be written to CSR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cache_ops_evict_sw(uint64_t csr_enc)
{
    __asm__ __volatile__("csrw 0x7f9, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

/************************************************************************
*
*   FUNCTION
*
*       cache_ops_flush_sw
*
*   DESCRIPTION
*
*       Local fn helper to write to flush SW CSR.
*
*   INPUTS
*
*       csr_enc        Value to be written to CSR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cache_ops_flush_sw(uint64_t csr_enc)
{
    __asm__ __volatile__("csrw 0x7fb, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

/************************************************************************
*
*   FUNCTION
*
*       cache_ops_lock_sw
*
*   DESCRIPTION
*
*       Local fn helper to write to lock SW CSR.
*
*   INPUTS
*
*       csr_enc        Value to be written to CSR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cache_ops_lock_sw(uint64_t csr_enc)
{
    __asm__ __volatile__("csrw 0x7fd, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

/************************************************************************
*
*   FUNCTION
*
*       cache_ops_unlock_sw
*
*   DESCRIPTION
*
*       Local fn helper to write to unlock SW CSR.
*
*   INPUTS
*
*       csr_enc        Value to be written to CSR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cache_ops_unlock_sw(uint64_t csr_enc)
{
    __asm__ __volatile__("csrw 0x7ff, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

/************************************************************************
*
*   FUNCTION
*
*       cache_ops_cache_invalidate
*
*   DESCRIPTION
*
*       Local fn helper to write to cache invalidate CSR.
*
*   INPUTS
*
*       csr_enc        Value to be written to CSR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cache_ops_cache_invalidate(uint64_t csr_enc)
{
    __asm__ __volatile__("csrw 0x7d0, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

// disable_mask is a bitmask to DISABLE a thread1: bit 0 = minion 0, bit 31 = minion 31
// enable_mask is a bitmask to ENABLE a thread1: bit 0 = minion 0, bit 31 = minion 31
static int64_t enable_thread1(uint64_t disable_mask, uint64_t enable_mask)
{
    volatile uint64_t *const disable_thread1_ptr =
        (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, THREAD1_DISABLE);
    *disable_thread1_ptr = (*disable_thread1_ptr & ~enable_mask) | disable_mask;

    return SYSCALL_INTERNAL_SUCCESS;
}

// All the M-mode only work that needs to be done before a kernel launch
// to avoid the overhead of making multiple syscalls
static int64_t pre_kernel_setup(uint64_t thread1_enable_mask, uint64_t first_worker)
{
    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        // Disable L1 split and scratchpad. Unlocks and evicts all lines.
        init_l1();
    }

    // First worker HART in the shire
    if ((get_hart_id() % 64U) == first_worker)
    {
        enable_thread1(0, thread1_enable_mask);
    }

    // First minion in each neighborhood
    if (get_hart_id() % 8 == 0U)
    {
        // Invalidate shared L1 I-cache
        asm volatile("csrw cache_invalidate, 1");
    }

    /* Reset Minion Neighborhood PMCs. Details of which hart resets which PMC can be found in PMU component */
    reset_minion_neigh_pmcs_all();

    return SYSCALL_INTERNAL_SUCCESS;
}

// All the M-mode only work that needs to be done after a kernel returns
// to avoid the overhead of making multiple syscalls
static int64_t post_kernel_cleanup(uint64_t thread_count)
{
    const uint32_t shire_id = get_shire_id();

    /* Thread 0 in each minion evicts L1 */
    if (get_thread_id() == 0)
    {
        evict_l1(0, to_L2);
    }

    /* Last thread in shire evicts L2. A full L2 evict includes flushing the coalescing buffer */
    if (atomic_add_local_32(&Kernel_Launch_Thread_Cleanup[shire_id].flag, 1U) == (thread_count - 1))
    {
        /* Reset the thread counter */
        init_local_spinlock(&Kernel_Launch_Thread_Cleanup[shire_id], 0);

        /* Evict L2 */
        evict_l2();
    }

    return SYSCALL_INTERNAL_SUCCESS;
}

static inline void unlock_required_sw(void)
{
    /* Unlock sets 14-15: enabling scratchpad will reset sets 0-13 */
    for (uint64_t set = 14; set < 16; set++)
    {
        for (uint64_t way = 0; way < 4; way++)
        {
            unlock_sw(way, set);
        }
    }
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

            mcache_control(1, 0, 0, 0); // Enable split mode
            allSetsReset = true;        // No need to subsequently unlock lines:
                                        // enabling split mode reset all sets
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
                unlock_required_sw();
            }

            // Evict sets 0-13 for HART0
            evict_all_l1_ways(0, to_L2, 0, 13);

            WAIT_CACHEOPS
            FENCE

            mcache_control(1, 1, 0, 0); // Enable scratchpad
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
                unlock_sw(way, set);
            }
        }
    }

    // Reset CacheOp values (CacheOp_RepRate = 0, CacheOp_Max = 8)
    mcache_control_reg = mcache_control_get();
    mcache_control(mcache_control_reg & 1, (mcache_control_reg >> 1) & 1, 0, 8);

    excl_mode(0); // release exclusive access to the processor

    return rv;
}

// evict all 4 ways of a given group of L1 sets up to dest_level
static inline void evict_all_l1_ways(
    uint64_t use_tmask, uint64_t dest_level, uint64_t set, uint64_t num_sets)
{
    evict_sw(use_tmask, dest_level, 0, set, num_sets);
    evict_sw(use_tmask, dest_level, 1, set, num_sets);
    evict_sw(use_tmask, dest_level, 2, set, num_sets);
    evict_sw(use_tmask, dest_level, 3, set, num_sets);
}

static int64_t evict_l1_all(uint64_t use_tmask, uint64_t dest_level)
{
    int64_t rv = 0;
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

static int64_t evict_l2(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L2_EVICT); // Evict all L2 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return SYSCALL_INTERNAL_SUCCESS;
}

static int64_t evict_l1_l2_all(void)
{
    int64_t rv;

    rv = evict_l1(0, to_L2);
    if (rv == SYSCALL_INTERNAL_SUCCESS)
    {
        rv = evict_l2();
    }

    return rv;
}

static int64_t flush_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L3_FLUSH); // Flush all L3 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return SYSCALL_INTERNAL_SUCCESS;
}

static int64_t evict_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L3_EVICT); // Evict all L3 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return SYSCALL_INTERNAL_SUCCESS;
}

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op)
{
    switch (op)
    {
        case SC_CACHEOP_OPCODE_L2_FLUSH:
        case SC_CACHEOP_OPCODE_L2_EVICT:
        case SC_CACHEOP_OPCODE_L3_FLUSH:
        case SC_CACHEOP_OPCODE_L3_EVICT:
        case SC_CACHEOP_OPCODE_CB_INV:
            sc_cache_bank_op(shire, bank, op);
            return SYSCALL_INTERNAL_SUCCESS;
        default:
            return -1;
    }
}

// change L1 configuration from Shared to Split, optionally with Scratchpad enabled
static inline void l1_shared_to_split(
    uint64_t scp_en, uint64_t cacheop_reprate, uint64_t cacheop_max)
{
    // Evict all cache lines from the L1 to L2
    evict_l1_all(0, to_L2);

    // Wait for all evictions to complete
    WAIT_CACHEOPS

    // Change L1 Dcache to split mode
    mcache_control(1, 0, cacheop_reprate, cacheop_max);

    if (scp_en)
    {
        // Wait for the L1 cache to change its configuration
        FENCE

        // Change L1 Dcache to split mode and SCP enabled
        mcache_control(1, 1, cacheop_reprate, cacheop_max);
    }
}

// change L1 configuration from Split to Shared
static inline void l1_split_to_shared(
    uint64_t scp_enabled, uint64_t cacheop_reprate, uint64_t cacheop_max)
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
    mcache_control(0, 0, cacheop_reprate, cacheop_max);
}

static int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en)
{
    uint64_t mcache_control_reg;
    uint64_t cur_split;
    uint64_t cur_scp_en;
    uint64_t cur_cacheop_reprate;
    uint64_t cur_cacheop_max;

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
    cur_cacheop_reprate = (mcache_control_reg >> 2) & 0x7;
    cur_cacheop_max = (mcache_control_reg >> 6) & 0x1F;

    if (!cur_split)
    {
        // It is shared and we want to split
        if (d1_split)
        {
            l1_shared_to_split(scp_en, cur_cacheop_reprate, cur_cacheop_max);
        }
    }
    else
    {
        // It is split (maybe with SCP on) and we want shared mode
        if (!d1_split)
        {
            l1_split_to_shared(cur_scp_en, cur_cacheop_reprate, cur_cacheop_max);
        }
    }

    // Release the hold of the processor
    excl_mode(0);

    return SYSCALL_INTERNAL_SUCCESS;
}
