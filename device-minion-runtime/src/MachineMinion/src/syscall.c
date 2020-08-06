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
#include "shire_cache.h"
#include "pmu.h"

#include <stdint.h>

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static int64_t ipi_trigger(uint64_t hart_mask, uint64_t shire_id);
static int64_t enable_thread1(uint64_t disable_mask, uint64_t enable_mask);

static int64_t pre_kernel_setup(uint64_t hart_enable_mask, uint64_t first_worker);
static int64_t post_kernel_cleanup(uint64_t thread_count);

static int64_t init_l1(void);
static inline void evict_all_l1_ways(uint64_t use_tmask, uint64_t dest_level, uint64_t set, uint64_t num_sets);
static int64_t evict_l1(uint64_t use_tmask, uint64_t dest_level);

static int64_t evict_l2(void);
static int64_t flush_l3(void);
static int64_t evict_l3(void);

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op);

static inline void l1_shared_to_split(uint64_t scp_en, uint64_t cacheop);
static inline void l1_split_to_shared(uint64_t scp_enabled, uint64_t cacheop);
static int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en);

static int64_t configure_pmcs(uint64_t reset_counters);
static int64_t sample_pmcs(uint64_t reset_counters);
static int64_t reset_pmcs(void);

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
        case SYSCALL_EVICT_L3_INT:
            ret = evict_l3();
            break;
        case SYSCALL_SHIRE_CACHE_BANK_OP_INT:
            ret = shire_cache_bank_op_with_params(arg1, arg2, arg3);
            break;
        case SYSCALL_EVICT_L1_INT:
            ret = evict_l1(arg1, arg2);
            break;
        case SYSCALL_CONFIGURE_PMCS_INT:
            ret = configure_pmcs(arg1);
            break;
        case SYSCALL_SAMPLE_PMCS_INT:
            ret = sample_pmcs(arg1);
            break;
        case SYSCALL_RESET_PMCS_INT:
            ret = reset_pmcs();
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
    // [SW-3478]: FIXME/TODO: Add per shire locking/mutex to avoid race conditions
    volatile uint64_t* const disable_thread1_ptr = (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, THREAD1_DISABLE);
    *disable_thread1_ptr = (*disable_thread1_ptr & ~enable_mask) | disable_mask;

    return 0;
}

// All the M-mode only work that needs to be done before a kernel launch
// to avoid the overhead of making multiple syscalls
static int64_t pre_kernel_setup(uint64_t thread1_enable_mask, uint64_t first_worker)
{
  if (get_hart_id() == 0) {
    printf_("%d%d            %dMachine minion pre_kernel %d\n",get_hart_id());
  }
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

    // Reset CacheOp values (CacheOp_RepRate = 0, CacheOp_Max = 8)
    mcache_control_reg = mcache_control_get();
    mcache_control(mcache_control_reg & 1, (mcache_control_reg >> 1) & 1, 0x200);

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

static int64_t evict_l2(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L2_EVICT); // Evict all L2 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return 0;
}

static int64_t flush_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L3_FLUSH); // Flush all L3 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return 0;
}

static int64_t evict_l3(void)
{
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);
    sc_idx_cop_sm_ctl_all_banks_go(THIS_SHIRE, SC_CACHEOP_OPCODE_L3_EVICT); // Evict all L3 indexes
    sc_idx_cop_sm_ctl_all_banks_wait_idle(THIS_SHIRE);

    return 0;
}

static int64_t shire_cache_bank_op_with_params(uint64_t shire, uint64_t bank, uint64_t op)
{
    switch (op) {
    case SC_CACHEOP_OPCODE_L2_FLUSH:
    case SC_CACHEOP_OPCODE_L2_EVICT:
    case SC_CACHEOP_OPCODE_L3_FLUSH:
    case SC_CACHEOP_OPCODE_L3_EVICT:
    case SC_CACHEOP_OPCODE_CB_INV:
        sc_cache_bank_op(shire, bank, op);
        return 0;
    default:
        return -1;
    }
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


// Configure PMCs
// conf_area point to the beginning of the memory buffer where the configuration values are stored
// reset_counters is a boolean that determines whether we reset / start counters after the configuration
// return 0 id all configuration succeeds, or negative number equal to the number of failed configurations.
static int64_t configure_pmcs(uint64_t reset_counters)
{
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t odd_hart = hart_id & 0x1;
    uint64_t program_neigh_harts = ((hart_id & 0xF) >= 0x8) && ((hart_id & 0xF) < 0xC);
    uint64_t program_sc_harts = ((hart_id & 0xF) == 0xC) || ((hart_id & 0xF) == 0xD);
    uint64_t program_ms_harts = ((hart_id & 0xF) == 0xF);
    int64_t ret = 0;

    // emizan:
    // We assume conf buffer is available -- hardcode it for now
    // It is not used so it does not matter
    uint64_t conf_buffer_addr = 0x8280000000ULL;
    uint64_t *conf_buffer = (uint64_t *) conf_buffer_addr;

    // minion counters: Each hart configures all counters so that we measure events for all the harts
    uint64_t *hart_minion_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id + PMU_MINION_COUNTERS_PER_HART * odd_hart;
    for (uint64_t i=0; i < PMU_MINION_COUNTERS_PER_HART; i++) {
        ret = ret + configure_neigh_event(*hart_minion_cfg_data, PMU_MHPMEVENT3 + i);
    }

    // neigh counters: Only one minion per neigh needs to configure the counters
    uint64_t *hart_neigh_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id + PMU_MINION_COUNTERS_PER_HART * 2
            + PMU_NEIGH_COUNTERS_PER_HART * odd_hart;
    if (program_neigh_harts) {
        ret = ret + configure_neigh_event(*hart_neigh_cfg_data, PMU_MHPMEVENT7);
        ret = ret + configure_neigh_event(*hart_neigh_cfg_data, PMU_MHPMEVENT8);
    }

    // sc location
    uint64_t *hart_sc_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id + PMU_MINION_COUNTERS_PER_HART * 2
            + PMU_NEIGH_COUNTERS_PER_HART * 2 + odd_hart;
    if (program_sc_harts) {
        // One bank per neigh
        ret = ret + configure_sc_event(shire_id, neigh_id, odd_hart, *hart_sc_cfg_data);
    }

    // Shire id's 0-7 just to simplify code initialize ms-id's 0-7.
    // TBD: Use shires closer to memshires
    // Currently last hart on each neigh of shires 0-7 programs memshire pref ctrl registers,
    uint64_t *hart_ms_cfg_data = conf_buffer + PMU_EVENT_MEMSHIRE_OFFSET + shire_id * PMU_MS_COUNTERS_PER_MS;
    if (program_ms_harts) {
        ret = ret + configure_ms_event(shire_id, neigh_id, *hart_ms_cfg_data);
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}

// Reset PMCs
// Each of harts 0-11 reset one neigh counter
// Harts 12-13 reset SC bank PMCs
// Hart 15 of neigh 0 of shires 0-7 resets MS PMCs.
static int64_t reset_pmcs(void)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t reset_sc_harts = ((hart_id & 0xF) == 0xC) || ((hart_id & 0xF) == 0xD);
    uint64_t reset_ms_harts = (shire_id < 8) && (neigh_id == 0) && ((hart_id & 0x1F) == 0x1F);

    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART) {
        ret = ret + reset_neigh_pmc(PMU_MHPMCOUNTER3 + neigh_minion_id);
    }

    if (reset_sc_harts) {
        reset_sc_pmcs(shire_id, neigh_id);
    }

    if (reset_ms_harts) {
        reset_ms_pmcs(shire_id);
    }

    return ret;
}


// Sample PMCs
// Each of harts 0-11 read one neigh PMC
// Harts 12-14 read shire cache PMCs
// Hart 15 of neighs 0-2, shires 0-7 read memshire PMCs
static int64_t sample_pmcs(uint64_t reset_counters)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x7;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t read_sc_harts = (((hart_id & 0xF) >= 0xC) && ((hart_id & 0xF) <= 0xE));
    uint64_t read_ms_harts = ((hart_id & 0xF) == 0xF) && (neigh_id < 3) && (shire_id < 8);

    // We assume log buffer is available.
    // Here give it a hard coded value
    // Should it be: MRT_TRACE_CONTROL_BASE + MRT_TRACE_CONTROL_SIZE + (HART_ID * SIZE PER HART)
    // Is the buffer a unit64, or should it be a structure ?
    uint64_t *log_buffer = (uint64_t *) (0x83C0000000ULL + hart_id * 0x20000);
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;

    // Minion and neigh PMCs
    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART) {
        uint64_t pmc_data = read_neigh_pmc(PMU_MHPMCOUNTER3 + neigh_minion_id);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *log_buffer = pmc_data;
        log_buffer++;
    }

    // SC PMCs
    if (read_sc_harts) {
        uint64_t pmc = (hart_id & 0xF) - 0xC;
        stop_sc_pmcs(shire_id, neigh_id);
        uint64_t pmc_data = sample_sc_pmcs(shire_id, neigh_id, pmc);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *log_buffer = pmc_data;
        log_buffer++;
    }

    // MS PMCs
    if (read_ms_harts) {
        stop_ms_pmcs(shire_id);
        uint64_t pmc_data = sample_ms_pmcs(shire_id, neigh_id);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *log_buffer = pmc_data;
        log_buffer++;
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}
