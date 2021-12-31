/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_ESRS_H
#define BEMU_ESRS_H

#include <array>
#include <cstdint>
#include "emu_defines.h"
#include "agent.h"

namespace bemu {


// ESR region 'pp' field in bits [31:30]
#define ESR_REGION_PROT_MASK    0x00C0000000ULL // ESR Region Protection is defined in bits [31:30]
#define ESR_REGION_PROT_SHIFT   30              // Bits to shift to get the ESR Region Protection defined in bits [31:30]


// ESR region 'shireid' field in bits [29:22]
// The local shire has bits [29:22] = 8'b11111111
#define ESR_REGION_SHIRE_MASK   0x003FC00000ULL
#define ESR_REGION_LOCAL_SHIRE  0x003FC00000ULL
#define ESR_REGION_SHIRE_SHIFT  22


// ESR region 'hart' field in bits [19:12] (when 'subregion' is 2'b00)
#define ESR_REGION_HART_MASK    0x00000FF000ULL
#define ESR_REGION_HART_SHIFT   12


// ESR region 'neighborhood' field in bits [19:16] (when 'subregion' is 2'b01)
// The broadcast neighborhood has bits [19:16] == 4'b1111
#define ESR_REGION_NEIGH_MASK       0x00000F0000ULL
#define ESR_REGION_NEIGH_SHIFT      16
#define ESR_REGION_NEIGH_BROADCAST  0xF


// ESR region 'bank' field in bits [16:13] (when 'subregion' is 2'b11)
#define ESR_REGION_BANK_MASK    0x000001E000ULL
#define ESR_REGION_BANK_SHIFT   13


// ESR region 'ESR' field in bits [xx:3] (depends on 'subregion' and
// 'extregion' fields). The following masks have all the bits of the address
// enabled, except the ones specifiying the 'shire', 'neigh', 'hart', and
// 'bank' fields.
#define ESR_HART_ESR_MASK       0xFFC0300FFFULL
#define ESR_NEIGH_ESR_MASK      0xFFC030FFFFULL
#define ESR_CACHE_ESR_MASK      0xFFC03E1FFFULL
#define ESR_SHIRE_ESR_MASK      0xFFC03FFFFFULL
#define ESR_RBOX_ESR_MASK       0xFFC03FFFFFULL


// IOshire has a different mask from Minion shires
#define ESR_IOSHIRE_ESR_MASK    0xFFC0200FFFULL


// bits [21:20] and further limited by bits [19:12] depending on the region
#define ESR_SREGION_MASK        0x0100300000ULL


// The ESR Extended Region is defined by bits [21:17] (when 'subregion' is 2'b11)
#define ESR_SREGION_EXT_MASK    0x01003E0000ULL
#define ESR_SREGION_EXT_SHIFT   17


// Base addresses for the various ESR subregions
//  * Hart ESR Region is at region [21:20] == 2'b00
//  * Neigh ESR Region is at region [21:20] == 2'b01
//  * The ESR Region at region [21:20] == 2'b10 is reserved
//  * shire_cache ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b000
//  * RBOX ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b001
//  * shire_other ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010
#define ESR_REGION_BASE        0x0100000000ULL
#define ESR_REGION_SIZE        0x0100000000ULL
#define ESR_HART_REGION        0x0100000000ULL
#define ESR_NEIGH_REGION       0x0100100000ULL
#define ESR_RSRVD_REGION       0x0100200000ULL
#define ESR_CACHE_REGION       0x0100300000ULL
#define ESR_RBOX_REGION        0x0100320000ULL
#define ESR_SHIRE_REGION       0x0100340000ULL

// The message port subregion inside the Hart ESR region
#define ESR_HART_PORT_ADDR_VALID(x) (((x) & 0xF38) == 0x800)
#define ESR_HART_PORT_NUM_MASK      0xC0ULL
#define ESR_HART_PORT_NUM_SHIFT     6


// Helper macros to construct ESR addresses in the various subregions

#define ESR_HART(shire, hart, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(hart) << ESR_REGION_HART_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_NEIGH(shire, neigh, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(neigh) << ESR_REGION_NEIGH_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_CACHE(shire, bank, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(bank) << ESR_REGION_BANK_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_RBOX(shire, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_SHIRE(shire, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_ ## name))


// Hart ESR addresses
#define ESR_HART_U0             0x0100000000ULL /* PP = 0b00 */
#define ESR_HART_S0             0x0140000000ULL /* PP = 0b01 */
#define ESR_HART_M0             0x01C0000000ULL /* PP = 0b11 */


// Message Port addresses
#define ESR_PORT0               0x0100000800ULL /* PP = 0b00 */
#define ESR_PORT1               0x0100000840ULL /* PP = 0b00 */
#define ESR_PORT2               0x0100000880ULL /* PP = 0b00 */
#define ESR_PORT3               0x01000008c0ULL /* PP = 0b00 */


// Neighborhood ESR addresses
#define ESR_NEIGH_U0                0x0100100000ULL /* PP = 0b00 */
#define ESR_NEIGH_S0                0x0140100000ULL /* PP = 0b01 */
#define ESR_NEIGH_M0                0x01C0100000ULL /* PP = 0b11 */
#define ESR_DUMMY0                  0x0100100000ULL /* PP = 0b00 */
#define ESR_DUMMY1                  0x0100100008ULL /* PP = 0b00 */
#define ESR_MINION_BOOT             0x01C0100018ULL /* PP = 0b11 */
#define ESR_MPROT                   0x01C0100020ULL /* PP = 0b11 */
#define ESR_DUMMY2                  0x01C0100028ULL /* PP = 0b11 */
#define ESR_DUMMY3                  0x01C0100030ULL /* PP = 0b11 */
#define ESR_VMSPAGESIZE             0x01C0100038ULL /* PP = 0b11 */
#define ESR_IPI_REDIRECT_PC         0x0100100040ULL /* PP = 0b00 */
//#define ESR_HACTRL                      /* PP = 0b10 */
//#define ESR_HASTATUS0                   /* PP = 0b10 */
//#define ESR_HASTATUS1                   /* PP = 0b10 */
//#define ESR_AND_OR_TREEL0               /* PP = 0b10 */
#define ESR_PMU_CTRL                0x01C0100068ULL /* PP = 0b11 */
#define ESR_NEIGH_CHICKEN           0x01C0100070ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_CTL      0x01C0100078ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_INFO     0x01C0100080ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_ADDRESS  0x01C0100088ULL /* PP = 0b11 */
#define ESR_ICACHE_SBE_DBE_COUNTS   0x01C0100090ULL /* PP = 0b11 */
#define ESR_TEXTURE_CONTROL         0x0100108000ULL /* PP = 0b00 */
#define ESR_TEXTURE_STATUS          0x0100108008ULL /* PP = 0b00 */
#define ESR_TEXTURE_IMAGE_TABLE_PTR 0x0100108010ULL /* PP = 0b00 */


// shire_cache ESR addresses
#define ESR_CACHE_U0                      0x0100300000ULL /* PP = 0b00 */
#define ESR_CACHE_S0                      0x0140300000ULL /* PP = 0b01 */
#define ESR_CACHE_M0                      0x01C0300000ULL /* PP = 0b11 */
#define ESR_SC_L3_SHIRE_SWIZZLE_CTL       0x01C0300000ULL /* PP = 0b11 */
#define ESR_SC_REQQ_CTL                   0x01C0300008ULL /* PP = 0b11 */
#define ESR_SC_PIPE_CTL                   0x01C0300010ULL /* PP = 0b11 */
#define ESR_SC_L2_CACHE_CTL               0x01C0300018ULL /* PP = 0b11 */
#define ESR_SC_L3_CACHE_CTL               0x01C0300020ULL /* PP = 0b11 */
#define ESR_SC_SCP_CACHE_CTL              0x01C0300028ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_CTL             0x01C0300030ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_PHYSICAL_INDEX  0x01C0300038ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_DATA0           0x01C0300040ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_DATA1           0x01C0300048ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_ECC             0x01C0300050ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_CTL                0x01C0300058ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_INFO               0x01C0300060ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_ADDRESS            0x01C0300068ULL /* PP = 0b11 */
#define ESR_SC_SBE_DBE_COUNTS             0x01C0300070ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG_CTL             0x01C0300078ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG0                0x01C0300080ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG1                0x01C0300088ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG2                0x01C0300090ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG3                0x01C0300098ULL /* PP = 0b11 */
#define ESR_SC_ECO_CTL                    0x01C03000A0ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_CTL_STATUS         0x01C03000B8ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_CYC_CNTR           0x01C03000C0ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_P0_CNTR            0x01C03000C8ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_P1_CNTR            0x01C03000D0ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_P0_QUAL            0x01C03000D8ULL /* PP = 0b11 */
#define ESR_SC_PERFMON_P1_QUAL            0x01C03000E0ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_CTL_USER        0x0100300100ULL /* PP = 0b00 */
//#define ESR_SC_TRACE_ADDRESS_ENABLE     /* PP = 0b10 */
//#define ESR_SC_TRACE_ADDRESS_VALUE      /* PP = 0b10 */
//#define ESR_SC_TRACE_CTL                /* PP = 0b10 */


// RBOX ESR addresses
#define ESR_RBOX_U0             0x0100320000ULL /* PP = 0b00 */
#define ESR_RBOX_S0             0x0140320000ULL /* PP = 0b01 */
#define ESR_RBOX_M0             0x01C0320000ULL /* PP = 0b11 */
#define ESR_RBOX_CONFIG         0x0100320000ULL /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_PG      0x0100320008ULL /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_CFG     0x0100320010ULL /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_PG     0x0100320018ULL /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_CFG    0x0100320020ULL /* PP = 0b00 */
#define ESR_RBOX_STATUS         0x0100320028ULL /* PP = 0b00 */
#define ESR_RBOX_START          0x0100320030ULL /* PP = 0b00 */
#define ESR_RBOX_CONSUME        0x0100320038ULL /* PP = 0b00 */


// shire_other ESR addresses
#define ESR_SHIRE_U0                    0x0100340000ULL /* PP = 0b00 */
#define ESR_SHIRE_S0                    0x0140340000ULL /* PP = 0b01 */
#define ESR_SHIRE_M0                    0x01C0340000ULL /* PP = 0b11 */
#define ESR_MINION_FEATURE              0x01C0340000ULL /* PP = 0b11 */
#define ESR_SHIRE_CONFIG                0x01C0340008ULL /* PP = 0b11 */
#define ESR_THREAD1_DISABLE             0x01C0340010ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_BUILD_CONFIG    0x01C0340018ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_REVISION_ID     0x01C0340020ULL /* PP = 0b11 */
#define ESR_IPI_REDIRECT_TRIGGER        0x0100340080ULL /* PP = 0b00 */
#define ESR_IPI_REDIRECT_FILTER         0x01C0340088ULL /* PP = 0b11 */
#define ESR_IPI_TRIGGER                 0x01C0340090ULL /* PP = 0b11 */
#define ESR_IPI_TRIGGER_CLEAR           0x01C0340098ULL /* PP = 0b11 */
#define ESR_FCC_CREDINC_0               0x01003400C0ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_1               0x01003400C8ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_2               0x01003400D0ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_3               0x01003400D8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER0         0x0100340100ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER1         0x0100340108ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER2         0x0100340110ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER3         0x0100340118ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER4         0x0100340120ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER5         0x0100340128ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER6         0x0100340130ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER7         0x0100340138ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER8         0x0100340140ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER9         0x0100340148ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER10        0x0100340150ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER11        0x0100340158ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER12        0x0100340160ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER13        0x0100340168ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER14        0x0100340170ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER15        0x0100340178ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER16        0x0100340180ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER17        0x0100340188ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER18        0x0100340190ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER19        0x0100340198ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER20        0x01003401A0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER21        0x01003401A8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER22        0x01003401B0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER23        0x01003401B8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER24        0x01003401C0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER25        0x01003401C8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER26        0x01003401D0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER27        0x01003401D8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER28        0x01003401E0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER29        0x01003401E8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER30        0x01003401F0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER31        0x01003401F8ULL /* PP = 0b00 */
//#define ESR_AND_OR_TREEL1               /* PP = 0b10 */
#define ESR_MTIME_LOCAL_TARGET          0x01C0340218ULL /* PP = 0b11 */
#define ESR_SHIRE_POWER_CTRL            0x01C0340220ULL /* PP = 0b11 */
#define ESR_POWER_CTRL_NEIGH_NSLEEPIN   0x01C0340228ULL /* PP = 0b11 */
#define ESR_POWER_CTRL_NEIGH_ISOLATION  0x01C0340230ULL /* PP = 0b11 */
#define ESR_POWER_CTRL_NEIGH_NSLEEPOUT  0x01C0340238ULL /* PP = 0b11 */
#define ESR_THREAD0_DISABLE             0x01C0340240ULL /* PP = 0b11 */
#define ESR_SHIRE_ERROR_LOG             0x01C0340248ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_AUTO_CONFIG       0x01C0340250ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_CONFIG_DATA_0     0x01C0340258ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_CONFIG_DATA_1     0x01C0340260ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_CONFIG_DATA_2     0x01C0340268ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_CONFIG_DATA_3     0x01C0340270ULL /* PP = 0b11 */
#define ESR_SHIRE_PLL_READ_DATA         0x01C0340288ULL /* PP = 0b11 */
#define ESR_SHIRE_COOP_MODE             0x0140340290ULL /* PP = 0b01 */
#define ESR_SHIRE_CTRL_CLOCKMUX         0x01C0340298ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_RAM_CFG1        0x01C03402A0ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_RAM_CFG2        0x01C03402A8ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_RAM_CFG3        0x01C03402B0ULL /* PP = 0b11 */
#define ESR_SHIRE_CACHE_RAM_CFG4        0x01C03402B8ULL /* PP = 0b11 */
#define ESR_SHIRE_NOC_INTERRUPT_STATUS  0x01C03402C0ULL /* PP = 0b11 */
#define ESR_SHIRE_DLL_AUTO_CONFIG       0x01C03402C8ULL /* PP = 0b11 */
#define ESR_SHIRE_DLL_CONFIG_DATA_0     0x01C03402D0ULL /* PP = 0b11 */
#define ESR_SHIRE_DLL_CONFIG_DATA_1     0x01C03402D8ULL /* PP = 0b11 */
#define ESR_SHIRE_DLL_READ_DATA         0x01C03402E0ULL /* PP = 0b11 */
//#define ESR_TBOX_RBOX_DBG_RC            /* PP = 0b10 */
#define ESR_UC_CONFIG                   0x01403402E8ULL /* PP = 0b01 */
//#define ESR_SHIRE_CTRL_RESET_DBG        /* PP = 0b10 */
#define ESR_ICACHE_UPREFETCH            0x01003402F8ULL /* PP = 0b00 */
#define ESR_ICACHE_SPREFETCH            0x0140340300ULL /* PP = 0b01 */
#define ESR_ICACHE_MPREFETCH            0x01C0340308ULL /* PP = 0b11 */
#define ESR_CLK_GATE_CTRL               0x01C0340310ULL /* PP = 0b11 */
//#define ESR_DEBUG_CLK_GATE_CTRL         /* PP = 0b10 */
#define ESR_SHIRE_CHANNEL_ECO_CTL       0x01C0340340ULL /* PP = 0b11 */


// IOshire ESR addresses
#define ESR_PU_RVTIM_MTIME        0x01C0000000ULL /* PP = 0b11 */
#define ESR_PU_RVTIM_MTIMECMP     0x01C0000008ULL /* PP = 0b11 */


// Broadcast ESR addresses
#define ESR_BROADCAST_DATA      0x013FF5FFF0ULL /* PP = 0b00 */
#define ESR_UBROADCAST          0x013FF5FFF8ULL /* PP = 0b00 */
#define ESR_SBROADCAST          0x017FF5FFF8ULL /* PP = 0b01 */
#define ESR_MBROADCAST          0x01FFF5FFF8ULL /* PP = 0b11 */

#define ESR_SHIRE_RESET_MASK   0x1E00
// -----------------------------------------------------------------------------
// Neighborhood ESRs

struct neigh_esrs_t {
    uint64_t icache_err_log_info;
    uint64_t ipi_redirect_pc;
    uint64_t minion_boot;
    uint64_t texture_image_table_ptr;
    uint32_t dummy0;
    uint16_t icache_sbe_dbe_counts;
    uint16_t texture_control;
    uint16_t texture_status;
    uint8_t  icache_err_log_ctl;
    uint8_t  mprot;
    uint8_t  neigh_chicken;
    uint8_t  vmspagesize;
    bool     dummy2;
    bool     dummy3;
    bool     pmu_ctrl;

    void reset();
};


// -----------------------------------------------------------------------------
// shire_cache ESRs

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
        uint32_t sc_reqq_ctl;
        uint16_t sc_err_log_ctl;
        uint8_t  sc_eco_ctl;
        uint64_t sc_perfmon_ctl_status;
        uint64_t sc_perfmon_cyc_cntr;
        uint64_t sc_perfmon_p0_cntr;
        uint64_t sc_perfmon_p1_cntr;
        uint64_t sc_perfmon_p0_qual;
        uint64_t sc_perfmon_p1_qual;
    } bank[4]; // four banks

    void reset();
};


// -----------------------------------------------------------------------------
// shire_other ESRs

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
    uint16_t shire_power_ctrl;
    uint32_t power_ctrl_neigh_nsleepin;
    uint32_t power_ctrl_neigh_isolation;
    uint32_t shire_pll_auto_config;
    uint16_t shire_dll_auto_config;
    uint8_t  minion_feature;
    uint8_t  shire_ctrl_clockmux;
    bool     shire_coop_mode;
    bool     uc_config;
    uint16_t clk_gate_ctrl;
    uint8_t  shire_channel_eco_ctl;

    // this is a proxy for icache_{msu}prefetch
    bool     icache_prefetch_active;

    void reset(unsigned shireid);
};


// -----------------------------------------------------------------------------
// Broadcast ESRs

struct broadcast_esrs_t {
    uint64_t data;

    void reset() {}
};


} // namespace bemu

#endif // BEMU_ESRS_H
