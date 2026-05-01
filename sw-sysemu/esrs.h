/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_ESRS_H
#define BEMU_ESRS_H

#include <array>
#include <cstdint>
#include "emu_defines.h"
#include "agent.h"

#if EMU_ERBIUM
#include <hwinc/top.h>
#endif

namespace bemu {


#if EMU_ERBIUM

// ERBIUM ESR memory map:
//   bit 31: always 1
//   bits 30-24: always 0
//   bits 23-22: PP (privilege) field
//   bits 21-0: address within ESR space

// ESR region 'pp' field in bits [23:22] - used by pma_et.cpp
#define ESR_REGION_PROT_MASK    0x00C00000ull
#define ESR_REGION_PROT_SHIFT   22

// Base address and size of ESR region - used by memory/sysreg_region.h
// Size: 24 bits of address space (including PP) -> 0x01000000 (16MB)
#define ESR_REGION_BASE         ERBIUM_TOP_CPU_REGISTERS_BASE
#define ESR_REGION_SIZE         0x01000000ull

// Hart ESR addresses used by processor.cpp for debug/program buffer
// Base 0x80000000 + PP=2 (debug) at bits 23:22 (0x00800000) + offset
#define ESR_AXPROGBUF0          0x808007A0ull
#define ESR_AXPROGBUF1          0x808007A8ull
#define ESR_NXPROGBUF0          0x808007B0ull
#define ESR_NXPROGBUF1          0x808007B8ull
#define ESR_ABSCMD              0x808007C0ull

#elif EMU_ETSOC1

// ESR region 'pp' field in bits [31:30] - used by pma_et.cpp
#define ESR_REGION_PROT_MASK    0x00C0000000ull
#define ESR_REGION_PROT_SHIFT   30

// Base address and size of ESR region - used by memory/sysreg_region.h
#define ESR_REGION_BASE         0x0100000000ull
#define ESR_REGION_SIZE         0x0100000000ull

// Hart ESR addresses used by processor.cpp for debug/program buffer
#define ESR_AXPROGBUF0          0x01800007A0ull
#define ESR_AXPROGBUF1          0x01800007A8ull
#define ESR_NXPROGBUF0          0x01800007B0ull
#define ESR_NXPROGBUF1          0x01800007B8ull
#define ESR_ABSCMD              0x01800007C0ull

#endif


// -----------------------------------------------------------------------------
// Minshire neighborhood ESRs

struct neigh_esrs_t {
    uint64_t icache_err_log_info;
    uint64_t ipi_redirect_pc;
    uint64_t minion_boot;
    uint64_t texture_image_table_ptr;
    uint64_t hactrl;
    uint64_t hastatus0;
    uint64_t hastatus1;
    uint32_t dummy0;
    uint16_t icache_sbe_dbe_counts;
    uint16_t texture_control;
    uint16_t texture_status;
    uint8_t  icache_err_log_ctl;
    uint16_t mprot;
    uint8_t  neigh_chicken;
    uint8_t  vmspagesize;
    bool     dummy2;
    bool     dummy3;
    bool     pmu_ctrl;

    void debug_reset();
    void warm_reset();
    void cold_reset();
};


// -----------------------------------------------------------------------------
// Minshire shire_cache ESRs

struct shire_cache_esrs_t {
    struct {
        uint64_t sc_err_log_info;
        //uint64_t sc_idx_cop_sm_ctl;
        uint64_t sc_idx_cop_sm_data0;
        uint64_t sc_idx_cop_sm_data1;
        uint64_t sc_idx_cop_sm_ecc;
        uint64_t sc_idx_cop_sm_physical_index;
        uint64_t sc_l2_cache_ctl;
        uint64_t sc_l3_cache_ctl;
        uint64_t sc_l3_shire_swizzle_ctl;
        uint64_t sc_pipe_ctl;
        uint64_t sc_reqq_debug0;
        uint64_t sc_reqq_debug1;
        uint64_t sc_reqq_debug2;
        uint64_t sc_reqq_debug3;
        uint64_t sc_reqq_debug_ctl;
        uint64_t sc_sbe_dbe_counts;
        uint64_t sc_scp_cache_ctl;
        uint64_t sc_perfmon_ctl_status;
        uint64_t sc_perfmon_cyc_cntr;
        uint64_t sc_perfmon_p0_cntr;
        uint64_t sc_perfmon_p1_cntr;
        uint64_t sc_perfmon_p0_qual;
        uint64_t sc_perfmon_p1_qual;
        uint32_t sc_reqq_ctl;
        uint16_t sc_err_log_ctl;
        uint8_t  sc_eco_ctl;
    } bank[4]; // four banks

    void debug_reset() {}
    void warm_reset() {}
    void cold_reset();
};


// -----------------------------------------------------------------------------
// Minshire shire_other ESRs

struct shire_other_esrs_t {
    uint8_t  fast_local_barrier[32];
    //uint64_t fcc_credinc[4];
    //uint64_t icache_mprefetch;
    //uint64_t icache_sprefetch;
    //uint64_t icache_uprefetch;
    uint64_t ipi_redirect_filter;
    uint64_t ipi_trigger;
    uint64_t shire_pll_config_data[4];
    uint64_t shire_dll_config_data_0;
    uint64_t shire_dll_config_data_1;
    uint64_t shire_cache_ram_cfg1;
    uint64_t shire_cache_ram_cfg3;
    uint64_t shire_cache_ram_cfg4;
    uint32_t shire_cache_ram_cfg2;
    uint32_t shire_config;
    uint32_t thread0_disable;
    uint32_t thread1_disable;
    uint32_t mtime_local_target;
    uint32_t power_ctrl_neigh_nsleepin;
    uint32_t power_ctrl_neigh_isolation;
    uint32_t shire_pll_auto_config;
    uint16_t shire_dll_auto_config;
    uint16_t shire_power_ctrl;
    uint16_t clk_gate_ctrl;
    uint8_t  minion_feature;
    uint8_t  shire_ctrl_clockmux;
    uint8_t  shire_channel_eco_ctl;
    bool     shire_coop_mode;
    bool     uc_config;
    bool     icache_prefetch_active; // proxy for icache_{msu}prefetch

    void debug_reset() {}

    void warm_reset();
    void cold_reset(unsigned shireid);
};


// -----------------------------------------------------------------------------
// Broadcast ESRs

struct broadcast_esrs_t {
    uint64_t data;

    void debug_reset() {}
    void warm_reset() {}
    void cold_reset() {}
};

// -----------------------------------------------------------------------------
// MEM Shire ESRs
struct mem_shire_esrs_t {
    uint64_t status;
    uint64_t int_en;
    uint64_t perf_ctrl_status;

    void debug_reset() {}
    void warm_reset() {}
    void cold_reset();
};

} // namespace bemu

#endif // BEMU_ESRS_H
