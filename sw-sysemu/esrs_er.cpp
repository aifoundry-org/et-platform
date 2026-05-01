/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "emu_defines.h"

#include <array>
#include <cassert>
#include <stdexcept>

#include "emu_gio.h"
#include "esrs.h"
#include "sysreg_error.h"
#include "system.h"
#include "memory/memory_error.h"

#include <hwinc/top.h>
#ifdef SYS_EMU
#include "checkers/mem_checker.h"
#include "sys_emu.h"

#define SYS_EMU_PTR agent.chip->emu()
#endif

namespace bemu {

// ESR region 'hart' field in bits [19:12] (when 'subregion' is 2'b00)
#define ESR_REGION_HART_MASK    0x00000FF000ull
#define ESR_REGION_HART_SHIFT   12

// ESR region 'neighborhood' field in bits [19:16] (when 'subregion' is 2'b01)
// The broadcast neighborhood has bits [19:16] == 4'b1111
#define ESR_REGION_NEIGH_MASK   0x000000F0000ull
#define ESR_REGION_NEIGH_SHIFT  16

// ESR region 'shireid' field in bits [30:24]
#define ESR_REGION_SHIRE_MASK   0x007F000000ull
#define ESR_REGION_SHIRE_SHIFT  24

// ESR region 'ESR' field masks
#define ESR_HART_ESR_MASK       0xFF80F00FFFull
#define ESR_NEIGH_ESR_MASK      0xFF80F0FFFFull
#define ESR_SHIRE_ESR_MASK      0xFF80FFFFFFull

// Subregion masks
#define ESR_SREGION_MASK        0x0080300000ull
#define ESR_SREGION_EXT_MASK    0x00803E0000ull

// Base addresses for ESR subregions
#define ESR_HART_REGION        ESR_REGION_BASE
#define ESR_NEIGH_REGION       0x0080100000ull
#define ESR_SHIRE_REGION       0x0080340000ull

// Debug mode (PP=2) data registers
#define ESR_NXDATA0             0x0080800780ull
#define ESR_NXDATA1             0x0080800788ull
#define ESR_AXDATA0             0x0080800790ull
#define ESR_AXDATA1             0x0080800798ull

// Neighborhood ESR addresses
#define ESR_DUMMY0                  0x0080D00000ull
#define ESR_DUMMY1                  0x0080D00008ull
#define ESR_MINION_BOOT             0x0080D00018ull
#define ESR_MPROT                   0x0080D00020ull
#define ESR_DUMMY2                  0x0080D00028ull
#define ESR_DUMMY3                  0x0080D00030ull
#define ESR_IPI_REDIRECT_PC         0x0080100040ull
#define ESR_PMU_CTRL                0x0080D00068ull
#define ESR_NEIGH_CHICKEN           0x0080D00070ull
#define ESR_ICACHE_ERR_LOG_CTL      0x0080D00078ull
#define ESR_ICACHE_ERR_LOG_INFO     0x0080D00080ull
#define ESR_ICACHE_ERR_LOG_ADDRESS  0x0080D00088ull
#define ESR_ICACHE_SBE_DBE_COUNTS   0x0080D00090ull
#define ESR_HACTRL                  0x008090FF80ull
#define ESR_HASTATUS0               0x008090FF88ull
#define ESR_HASTATUS1               0x008090FF90ull
#define ESR_AND_OR_TREE_L0          0x008090FF98ull

// shire_other ESR addresses
#define ESR_MINION_FEATURE              0x0080F40000ull
#define ESR_SHIRE_CONFIG                0x0080F40008ull
#define ESR_THREAD1_DISABLE             0x0080F40010ull
#define ESR_SHIRE_CACHE_BUILD_CONFIG    0x0080F40018ull
#define ESR_SHIRE_CACHE_REVISION_ID     0x0080F40020ull
#define ESR_IPI_REDIRECT_TRIGGER        0x0080340080ull
#define ESR_IPI_REDIRECT_FILTER         0x0080F40088ull
#define ESR_IPI_TRIGGER                 0x0080F40090ull
#define ESR_IPI_TRIGGER_CLEAR           0x0080F40098ull
#define ESR_FCC_CREDINC_0               0x00803400C0ull
#define ESR_FCC_CREDINC_1               0x00803400C8ull
#define ESR_FCC_CREDINC_2               0x00803400D0ull
#define ESR_FCC_CREDINC_3               0x00803400D8ull
#define ESR_FAST_LOCAL_BARRIER0         0x0080340100ull
#define ESR_FAST_LOCAL_BARRIER1         0x0080340108ull
#define ESR_FAST_LOCAL_BARRIER2         0x0080340110ull
#define ESR_FAST_LOCAL_BARRIER3         0x0080340118ull
#define ESR_FAST_LOCAL_BARRIER4         0x0080340120ull
#define ESR_FAST_LOCAL_BARRIER5         0x0080340128ull
#define ESR_FAST_LOCAL_BARRIER6         0x0080340130ull
#define ESR_FAST_LOCAL_BARRIER7         0x0080340138ull
#define ESR_FAST_LOCAL_BARRIER8         0x0080340140ull
#define ESR_FAST_LOCAL_BARRIER9         0x0080340148ull
#define ESR_FAST_LOCAL_BARRIER10        0x0080340150ull
#define ESR_FAST_LOCAL_BARRIER11        0x0080340158ull
#define ESR_FAST_LOCAL_BARRIER12        0x0080340160ull
#define ESR_FAST_LOCAL_BARRIER13        0x0080340168ull
#define ESR_FAST_LOCAL_BARRIER14        0x0080340170ull
#define ESR_FAST_LOCAL_BARRIER15        0x0080340178ull
#define ESR_FAST_LOCAL_BARRIER16        0x0080340180ull
#define ESR_FAST_LOCAL_BARRIER17        0x0080340188ull
#define ESR_FAST_LOCAL_BARRIER18        0x0080340190ull
#define ESR_FAST_LOCAL_BARRIER19        0x0080340198ull
#define ESR_FAST_LOCAL_BARRIER20        0x00803401A0ull
#define ESR_FAST_LOCAL_BARRIER21        0x00803401A8ull
#define ESR_FAST_LOCAL_BARRIER22        0x00803401B0ull
#define ESR_FAST_LOCAL_BARRIER23        0x00803401B8ull
#define ESR_FAST_LOCAL_BARRIER24        0x00803401C0ull
#define ESR_FAST_LOCAL_BARRIER25        0x00803401C8ull
#define ESR_FAST_LOCAL_BARRIER26        0x00803401D0ull
#define ESR_FAST_LOCAL_BARRIER27        0x00803401D8ull
#define ESR_FAST_LOCAL_BARRIER28        0x00803401E0ull
#define ESR_FAST_LOCAL_BARRIER29        0x00803401E8ull
#define ESR_FAST_LOCAL_BARRIER30        0x00803401F0ull
#define ESR_FAST_LOCAL_BARRIER31        0x00803401F8ull
#define ESR_MTIME                       0x0080F40200ull
#define ESR_MTIMECMP                    0x0080F40208ull
#define ESR_TIME_CONFIG                 0x0080F40210ull
#define ESR_MTIME_LOCAL_TARGET          0x0080F40218ull
#define ESR_THREAD0_DISABLE             0x0080F40240ull
#define ESR_SHIRE_COOP_MODE             0x0080740290ull
#define ESR_ICACHE_UPREFETCH            0x00803402F8ull
#define ESR_ICACHE_SPREFETCH            0x0080740300ull
#define ESR_ICACHE_MPREFETCH            0x0080F40308ull
#define ESR_CLK_GATE_CTRL               0x0080F40310ull
#define ESR_DMCTRL                      0x0080B5FF88ull
#define ESR_SM_CONFIG                   0x0080B5FF90ull
#define ESR_SM_TRIGGER                  0x0080B5FF98ull
#define ESR_SM_MATCH                    0x0080B5FFA8ull
#define ESR_SM_FILTER0                  0x0080B5FFB0ull
#define ESR_SM_FILTER1                  0x0080B5FFB8ull
#define ESR_SM_FILTER2                  0x0080B5FFC0ull
#define ESR_SM_DATA0                    0x0080B5FFC8ull
#define ESR_SM_DATA1                    0x0080B5FFD0ull


#define NEIGHID(pos)    ((pos) % EMU_NEIGH_PER_SHIRE)
#define MINION(hart)    ((hart) / EMU_THREADS_PER_MINION)
#define THREAD(hart)    ((hart) % EMU_THREADS_PER_MINION)


void neigh_esrs_t::debug_reset()
{
    hactrl = 0;
    hastatus0 = 0;
    hastatus1 = 0;
}


void neigh_esrs_t::warm_reset()
{
    ipi_redirect_pc = 0;
    pmu_ctrl = false;
}


void neigh_esrs_t::cold_reset()
{
    minion_boot = ERBIUM_TOP_BOOTROM_BASE;
    mprot = 0;
    dummy0 = 0;
    dummy2 = false;
    neigh_chicken = 0;
    icache_err_log_ctl = 0x06;
    icache_err_log_info = 0;
    icache_sbe_dbe_counts = 0;
}


void shire_cache_esrs_t::cold_reset()
{
    // No shire cache registers for Erbium
}


void shire_other_esrs_t::warm_reset()
{
    for (int i = 0; i < 32; ++i) {
        fast_local_barrier[i] = 0;
    }
    ipi_redirect_filter = 0xFFFF;
    ipi_trigger = 0;
    shire_coop_mode = false;
    icache_prefetch_active = false;
}


void shire_other_esrs_t::cold_reset(unsigned shireid)
{
    (void) shireid;
    minion_feature = 0x01;
    thread0_disable = 0xFE; // Only start minion 0 hart 0.
    thread1_disable = 0xFF;
    mtime_local_target = 0xFFFF;
    clk_gate_ctrl = 0;
    // sm_config = 0;
}


void mem_shire_esrs_t::cold_reset()
{
    // No mem shire registers for Erbium
}


uint64_t System::esr_read(const Agent& agent, uint64_t addr)
{
    unsigned shire = shireindex((addr & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    if (shire >= EMU_NUM_SHIRES) {
            WARN_AGENT(esrs, agent, "Read ESR for illegal shire S%u:0x%" PRIx64,
                       shire, addr);
            throw memory_error(addr);
    }
    
    uint64_t sregion = addr & ESR_SREGION_MASK;

    if (sregion == ESR_HART_REGION) {
        uint64_t esr = addr & ESR_HART_ESR_MASK;
        unsigned hart = (addr & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        switch (esr) {
        case ESR_ABSCMD:
        case ESR_NXPROGBUF0:
        case ESR_NXPROGBUF1:
        case ESR_AXPROGBUF0:
        case ESR_AXPROGBUF1:
            return 0;
        case ESR_NXDATA0:
        case ESR_AXDATA0:
            return cpu[hart].ddata0 & 0xFFFFFFFF;
        case ESR_NXDATA1:
        case ESR_AXDATA1:
            return cpu[hart].ddata0 >> 32;
        default:
            WARN_AGENT(esrs, agent, "Read unknown hart ESR S%u:M%u:T%u:0x%" PRIx64,
                       shireid(shire), MINION(hart), THREAD(hart), esr);
            throw memory_error(addr);
        }
    }

    if (sregion == ESR_NEIGH_REGION) {
        unsigned neigh = (addr & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr = addr & ESR_NEIGH_ESR_MASK;
        if (neigh >= EMU_NEIGH_PER_SHIRE) {
            WARN_AGENT(esrs, agent, "Read illegal neigh ESR S%u:N%u:0x%" PRIx64, shireid(shire), neigh, esr);
            throw memory_error(addr);
        }
        unsigned pos = neigh + EMU_NEIGH_PER_SHIRE * shire;
        switch (esr) {
        case ESR_DUMMY0:
            return neigh_esrs[pos].dummy0;
        case ESR_DUMMY1:
            return 0;
        case ESR_MINION_BOOT:
            return neigh_esrs[pos].minion_boot;
        case ESR_MPROT:
            return neigh_esrs[pos].mprot;
        case ESR_DUMMY2:
            return neigh_esrs[pos].dummy2;
        case ESR_DUMMY3:
            return 0;
        case ESR_IPI_REDIRECT_PC:
            return neigh_esrs[pos].ipi_redirect_pc;
        case ESR_PMU_CTRL:
            return neigh_esrs[pos].pmu_ctrl;
        case ESR_NEIGH_CHICKEN:
            return neigh_esrs[pos].neigh_chicken;
        case ESR_ICACHE_ERR_LOG_CTL:
            return neigh_esrs[pos].icache_err_log_ctl;
        case ESR_ICACHE_ERR_LOG_INFO:
            return neigh_esrs[pos].icache_err_log_info;
        case ESR_ICACHE_ERR_LOG_ADDRESS:
            return 0;
        case ESR_ICACHE_SBE_DBE_COUNTS:
            return neigh_esrs[pos].icache_sbe_dbe_counts;
        case ESR_HACTRL:
            return neigh_esrs[pos].hactrl;
        case ESR_HASTATUS0:
            return neigh_esrs[pos].hastatus0;
        case ESR_HASTATUS1:
            return neigh_esrs[pos].hastatus1;
        case ESR_AND_OR_TREE_L0:
            return calculate_andortree0(pos);
        }
        WARN_AGENT(esrs, agent, "Read unknown neigh ESR S%u:N%u:0x%" PRIx64, shireid(shire), neigh, esr);
        throw memory_error(addr);
    }

    uint64_t sregion_extra = addr & ESR_SREGION_EXT_MASK;

    if (sregion_extra == ESR_SHIRE_REGION) {
        uint64_t esr = addr & ESR_SHIRE_ESR_MASK;
        switch (esr) {
        case ESR_MINION_FEATURE:
            return shire_other_esrs[shire].minion_feature;
        case ESR_SHIRE_CONFIG:
            return 0; // Unsupported, return 0 anyways.
        case ESR_THREAD1_DISABLE:
            return shire_other_esrs[shire].thread1_disable;
        case ESR_SHIRE_CACHE_BUILD_CONFIG:
        case ESR_SHIRE_CACHE_REVISION_ID:
            return 0; // Unsupported, return 0 anyways.
        case ESR_IPI_REDIRECT_TRIGGER:
            return 0;
        case ESR_IPI_REDIRECT_FILTER:
            return shire_other_esrs[shire].ipi_redirect_filter;
        case ESR_IPI_TRIGGER:
            return shire_other_esrs[shire].ipi_trigger;
        case ESR_IPI_TRIGGER_CLEAR:
        case ESR_FCC_CREDINC_0:
        case ESR_FCC_CREDINC_1:
        case ESR_FCC_CREDINC_2:
        case ESR_FCC_CREDINC_3:
            return 0;
        case ESR_FAST_LOCAL_BARRIER0:
        case ESR_FAST_LOCAL_BARRIER1:
        case ESR_FAST_LOCAL_BARRIER2:
        case ESR_FAST_LOCAL_BARRIER3:
        case ESR_FAST_LOCAL_BARRIER4:
        case ESR_FAST_LOCAL_BARRIER5:
        case ESR_FAST_LOCAL_BARRIER6:
        case ESR_FAST_LOCAL_BARRIER7:
        case ESR_FAST_LOCAL_BARRIER8:
        case ESR_FAST_LOCAL_BARRIER9:
        case ESR_FAST_LOCAL_BARRIER10:
        case ESR_FAST_LOCAL_BARRIER11:
        case ESR_FAST_LOCAL_BARRIER12:
        case ESR_FAST_LOCAL_BARRIER13:
        case ESR_FAST_LOCAL_BARRIER14:
        case ESR_FAST_LOCAL_BARRIER15:
        case ESR_FAST_LOCAL_BARRIER16:
        case ESR_FAST_LOCAL_BARRIER17:
        case ESR_FAST_LOCAL_BARRIER18:
        case ESR_FAST_LOCAL_BARRIER19:
        case ESR_FAST_LOCAL_BARRIER20:
        case ESR_FAST_LOCAL_BARRIER21:
        case ESR_FAST_LOCAL_BARRIER22:
        case ESR_FAST_LOCAL_BARRIER23:
        case ESR_FAST_LOCAL_BARRIER24:
        case ESR_FAST_LOCAL_BARRIER25:
        case ESR_FAST_LOCAL_BARRIER26:
        case ESR_FAST_LOCAL_BARRIER27:
        case ESR_FAST_LOCAL_BARRIER28:
        case ESR_FAST_LOCAL_BARRIER29:
        case ESR_FAST_LOCAL_BARRIER30:
        case ESR_FAST_LOCAL_BARRIER31:
            return shire_other_esrs[shire].fast_local_barrier[(esr - ESR_FAST_LOCAL_BARRIER0)>>3];
        case ESR_MTIME:
#ifdef SYS_EMU
            return memory.rvtimer_read_mtime();
#else
            throw sysreg_error(esr);
#endif
        case ESR_MTIMECMP:
            return memory.rvtimer_read_mtimecmp();
        case ESR_TIME_CONFIG:
            return memory.rvtimer_read_time_config();
        case ESR_MTIME_LOCAL_TARGET:
            return shire_other_esrs[shire].mtime_local_target;
        case ESR_THREAD0_DISABLE:
            return shire_other_esrs[shire].thread0_disable;
        case ESR_SHIRE_COOP_MODE:
            return shire_other_esrs[shire].shire_coop_mode;
        case ESR_ICACHE_UPREFETCH:
            return read_icache_prefetch(Privilege::U, shire);
        case ESR_ICACHE_SPREFETCH:
            return read_icache_prefetch(Privilege::S, shire);
        case ESR_ICACHE_MPREFETCH:
            return read_icache_prefetch(Privilege::M, shire);
        case ESR_CLK_GATE_CTRL:
            return shire_other_esrs[shire].clk_gate_ctrl;
        case ESR_DMCTRL:
            return agent.chip->read_dmctrl();
        case ESR_SM_CONFIG:
            // Status Monitor not implemented.
            return 0;
        case ESR_SM_TRIGGER:
            // Status Monitor not implemented.
            return 0; // WARL
        case ESR_SM_MATCH:
        case ESR_SM_FILTER0:
        case ESR_SM_FILTER1:
        case ESR_SM_FILTER2:
        case ESR_SM_DATA0:
        case ESR_SM_DATA1:
            // Status Monitor not implemented.
            return 0;
        }
        WARN_AGENT(esrs, agent, "Read unknown shire_other ESR S%u:0x%" PRIx64, shireid(shire), esr);
        throw memory_error(addr);
    }

    WARN_AGENT(esrs, agent, "Read illegal ESR S%u:0x%" PRIx64, shireid(shire), addr);
    throw memory_error(addr);
}


void System::esr_write(const Agent& agent, uint64_t addr, uint64_t value)
{
    unsigned shire = shireindex((addr & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    if (shire >= EMU_NUM_SHIRES) {
            WARN_AGENT(esrs, agent, "Write ESR for illegal shire S%u:0x%" PRIx64,
                       shire, addr);
            throw memory_error(addr);
    }
    
    uint64_t sregion = addr & ESR_SREGION_MASK;

    if (sregion == ESR_HART_REGION) {
        uint64_t esr = addr & ESR_HART_ESR_MASK;
        unsigned hart = (addr & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        switch (esr) {
        case ESR_ABSCMD:
        case ESR_NXPROGBUF0:
        case ESR_NXPROGBUF1:
        case ESR_AXPROGBUF0:
        case ESR_AXPROGBUF1: {
            unsigned hartid = hart;
            if (cpu[hartid].in_progbuf()) {
                cpu[hartid].exit_progbuf(Hart::Progbuf::error);
            } else if (cpu[hartid].is_halted()) {
                cpu[hartid].write_progbuf(esr, value);
                if (esr < ESR_NXPROGBUF0 || esr > ESR_NXPROGBUF1) {
                    cpu[hartid].enter_progbuf();
                }
            }
            break;
        }
        case ESR_NXDATA0:
        case ESR_AXDATA0: {
            unsigned hartid = hart;
            cpu[hartid].ddata0 &= 0xFFFFFFFF00000000ull;
            cpu[hartid].ddata0 |= value & 0xFFFFFFFFull;
            if (esr == ESR_AXDATA0) cpu[hartid].enter_progbuf();
            LOG_AGENT(DEBUG, agent, "S%u:H%u:data0 = 0x%" PRIx32, shireid(shire), hart, uint32_t(value));
            break;
        }
        case ESR_NXDATA1:
        case ESR_AXDATA1: {
            unsigned hartid = hart + shire * EMU_THREADS_PER_SHIRE;
            cpu[hartid].ddata0 &= 0xFFFFFFFFull;
            cpu[hartid].ddata0 |= value << 32;
            LOG_AGENT(DEBUG, agent, "S%u:H%u:data1 = 0x%" PRIx32, shireid(shire), hart, uint32_t(value));
            if (esr == ESR_AXDATA1) cpu[hartid].enter_progbuf();
            break;
        }
        default:
            WARN_AGENT(esrs, agent, "Write unknown hart ESR S%u:M%u:T%u:0x%" PRIx64,
                       shireid(shire), MINION(hart), THREAD(hart), esr);
            throw memory_error(addr);
        }
        return;
    }

    if (sregion == ESR_NEIGH_REGION) {
        unsigned neigh = (addr & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr = addr & ESR_NEIGH_ESR_MASK;
        if (neigh >= EMU_NEIGH_PER_SHIRE) {
            WARN_AGENT(esrs, agent, "Write illegal neigh ESR S%u:N%u:0x%" PRIx64, shireid(shire), neigh, esr);
            throw memory_error(addr);
        }
        unsigned pos = neigh + EMU_NEIGH_PER_SHIRE * shire;
        switch (esr) {
        case ESR_DUMMY0:
            neigh_esrs[pos].dummy0 = uint32_t(value & 0xFFFFFFFF);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:dummy0 = 0x%" PRIx32,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].dummy0);
            break;
        case ESR_MINION_BOOT:
            neigh_esrs[pos].minion_boot = value & 0xFFFFFFFFFFFFull;
            LOG_AGENT(DEBUG, agent, "S%u:N%u:minion_boot = 0x%" PRIx64,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].minion_boot);
            break;
        case ESR_MPROT:
            neigh_esrs[pos].mprot = uint16_t(value & 0x1FF);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:mprot = 0x%" PRIx16,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].mprot);
            break;
        case ESR_DUMMY2:
            neigh_esrs[pos].dummy2 = bool(value & 1);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:dummy2 = 0x%x",
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].dummy2 ? 1 : 0);
            break;
        case ESR_IPI_REDIRECT_PC:
            neigh_esrs[pos].ipi_redirect_pc = value & 0xfffffffffffeull;
            LOG_AGENT(DEBUG, agent, "S%u:N%u:ipi_redirect_pc = 0x%" PRIx64,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].ipi_redirect_pc);
            break;
        case ESR_PMU_CTRL:
            neigh_esrs[pos].pmu_ctrl = bool(value & 1);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:pmu_ctrl = 0x%x",
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].pmu_ctrl ? 1 : 0);
            break;
        case ESR_NEIGH_CHICKEN:
            neigh_esrs[pos].neigh_chicken = uint8_t(value & 0x7F);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:neigh_chicken = 0x%" PRIx8,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].neigh_chicken);
            break;
        case ESR_ICACHE_ERR_LOG_CTL:
            neigh_esrs[pos].icache_err_log_ctl = uint8_t(value & 0x7);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:icache_err_log_ctl = 0x%" PRIx8,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].icache_err_log_ctl);
            break;
        case ESR_ICACHE_ERR_LOG_INFO:
            // Clear by setting bit 0 (`valid`) and matching `err_code` field.
            if ((value & 0x1) && ((value & 0xF0) == (neigh_esrs[pos].icache_err_log_info & 0xF0))) {
                neigh_esrs[pos].icache_err_log_info &= ~0x7ull;
            } else {
                // You can't write this register from CPU, but we allow it
                // for DV purposes, as we don't emulate the icache behaviour.
                neigh_esrs[pos].icache_err_log_info = value & ~0x8ull;
            }
            LOG_AGENT(DEBUG, agent, "S%u:N%u:icache_err_log_info = 0x%" PRIx64,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].icache_err_log_info);
            break;
        case ESR_ICACHE_ERR_LOG_ADDRESS:
            break;
        case ESR_ICACHE_SBE_DBE_COUNTS:
            neigh_esrs[pos].icache_sbe_dbe_counts = uint16_t(value & 0x7FF);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:icache_sbe_dbe_counts = 0x%" PRIx16,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].icache_sbe_dbe_counts);
            break;
        case ESR_HACTRL:
            neigh_esrs[pos].hactrl = uint32_t(value & 0xffffffff);
            LOG_AGENT(DEBUG, agent, "S%u:N%u:hactrl = 0x%" PRIx32,
                      shireid(shire), NEIGHID(pos), uint32_t(neigh_esrs[pos].hactrl));
            break;
        case ESR_HASTATUS1:
            neigh_esrs[pos].hastatus1 = value & 0xffffffff0000ull;
            LOG_AGENT(DEBUG, agent, "S%u:N%u:hastatus1 = 0x%" PRIx64,
                      shireid(shire), NEIGHID(pos), neigh_esrs[pos].hastatus1);
            break;
        default:
            WARN_AGENT(esrs, agent, "Write unknown neigh ESR S%u:N%u:0x%" PRIx64, shireid(shire), neigh, esr);
            throw memory_error(addr);
        }
        return;
    }

    uint64_t sregion_extra = addr & ESR_SREGION_EXT_MASK;

    if (sregion_extra == ESR_SHIRE_REGION) {
        uint64_t esr = addr & ESR_SHIRE_ESR_MASK;
        switch (esr) {
        case ESR_MINION_FEATURE:
            write_minion_feature(shire, uint8_t(value & 0x3F));
            LOG_AGENT(DEBUG, agent, "S%u:minion_feature = 0x%" PRIx8,
                      shireid(shire), shire_other_esrs[shire].minion_feature);
            return;
        case ESR_THREAD1_DISABLE:
            write_thread1_disable(shire, uint32_t(value & 0xFF));
            LOG_AGENT(DEBUG, agent, "S%u:thread1_disable = 0x%" PRIx32,
                      shireid(shire), shire_other_esrs[shire].thread1_disable);
            return;
        case ESR_IPI_REDIRECT_TRIGGER:
            LOG_AGENT(DEBUG, agent, "S%u:ipi_redirect_trigger = 0x%" PRIx64, shireid(shire), value);
            send_ipi_redirect(shire, value);
            return;
        case ESR_IPI_REDIRECT_FILTER:
            shire_other_esrs[shire].ipi_redirect_filter = value & 0xFFFF;
            LOG_AGENT(DEBUG, agent, "S%u:ipi_redirect_filter = 0x%" PRIx64,
                      shireid(shire), shire_other_esrs[shire].ipi_redirect_filter);
            return;
        case ESR_IPI_TRIGGER:
            shire_other_esrs[shire].ipi_trigger |= (value & 0xFFFF);
            LOG_AGENT(DEBUG, agent, "S%u:ipi_trigger = 0x%" PRIx64,
                      shireid(shire), shire_other_esrs[shire].ipi_trigger);
            raise_machine_software_interrupt(shire, value & 0xFFFF);
            return;
        case ESR_IPI_TRIGGER_CLEAR:
            LOG_AGENT(DEBUG, agent, "S%u:ipi_trigger_clear = 0x%" PRIx64, shireid(shire), value & 0xffff);
            shire_other_esrs[shire].ipi_trigger &= ~(value & 0xFFFF);
            clear_machine_software_interrupt(shire, value & 0xFFFF);
            return;
        case ESR_FCC_CREDINC_0:
            LOG_AGENT(DEBUG, agent, "S%u:fcc_credinc_0 = 0x%" PRIx64, shireid(shire), value & 0xff);
            write_fcc_credinc(0, shire, value & 0xFF);
            return;
        case ESR_FCC_CREDINC_1:
            LOG_AGENT(DEBUG, agent, "S%u:fcc_credinc_1 = 0x%" PRIx64, shireid(shire), value & 0xff);
            write_fcc_credinc(1, shire, value & 0xFF);
            return;
        case ESR_FCC_CREDINC_2:
            LOG_AGENT(DEBUG, agent, "S%u:fcc_credinc_2 = 0x%" PRIx64, shireid(shire), value & 0xff);
            write_fcc_credinc(2, shire, value & 0xFF);
            return;
        case ESR_FCC_CREDINC_3:
            LOG_AGENT(DEBUG, agent, "S%u:fcc_credinc_3 = 0x%" PRIx64, shireid(shire), value & 0xff);
            write_fcc_credinc(3, shire, value & 0xFF);
            return;
        case ESR_FAST_LOCAL_BARRIER0:
        case ESR_FAST_LOCAL_BARRIER1:
        case ESR_FAST_LOCAL_BARRIER2:
        case ESR_FAST_LOCAL_BARRIER3:
        case ESR_FAST_LOCAL_BARRIER4:
        case ESR_FAST_LOCAL_BARRIER5:
        case ESR_FAST_LOCAL_BARRIER6:
        case ESR_FAST_LOCAL_BARRIER7:
        case ESR_FAST_LOCAL_BARRIER8:
        case ESR_FAST_LOCAL_BARRIER9:
        case ESR_FAST_LOCAL_BARRIER10:
        case ESR_FAST_LOCAL_BARRIER11:
        case ESR_FAST_LOCAL_BARRIER12:
        case ESR_FAST_LOCAL_BARRIER13:
        case ESR_FAST_LOCAL_BARRIER14:
        case ESR_FAST_LOCAL_BARRIER15:
        case ESR_FAST_LOCAL_BARRIER16:
        case ESR_FAST_LOCAL_BARRIER17:
        case ESR_FAST_LOCAL_BARRIER18:
        case ESR_FAST_LOCAL_BARRIER19:
        case ESR_FAST_LOCAL_BARRIER20:
        case ESR_FAST_LOCAL_BARRIER21:
        case ESR_FAST_LOCAL_BARRIER22:
        case ESR_FAST_LOCAL_BARRIER23:
        case ESR_FAST_LOCAL_BARRIER24:
        case ESR_FAST_LOCAL_BARRIER25:
        case ESR_FAST_LOCAL_BARRIER26:
        case ESR_FAST_LOCAL_BARRIER27:
        case ESR_FAST_LOCAL_BARRIER28:
        case ESR_FAST_LOCAL_BARRIER29:
        case ESR_FAST_LOCAL_BARRIER30:
        case ESR_FAST_LOCAL_BARRIER31:
            shire_other_esrs[shire].fast_local_barrier[(esr - ESR_FAST_LOCAL_BARRIER0)>>3] = uint8_t(value);
            LOG_AGENT(DEBUG, agent, "S%u:fast_local_barrier%llu = 0x%" PRIx8,
                      shireid(shire), (esr - ESR_FAST_LOCAL_BARRIER0)>>3,
                      shire_other_esrs[shire].fast_local_barrier[(esr - ESR_FAST_LOCAL_BARRIER0)>>3]);
            return;
        case ESR_MTIME:
#ifdef SYS_EMU
            memory.rvtimer_write_mtime(agent, value);
            return;
#else
            throw sysreg_error(esr);
#endif
        case ESR_MTIMECMP:
            memory.rvtimer_write_mtimecmp(agent, value);
            return;
        case ESR_TIME_CONFIG:
            memory.rvtimer_write_time_config(agent, value);
            return;
        case ESR_MTIME_LOCAL_TARGET:
            shire_other_esrs[shire].mtime_local_target = uint16_t(value & 0xFFFF);
            LOG_AGENT(DEBUG, agent, "S%u:mtime_local_target = 0x%" PRIx16,
                      shireid(shire), shire_other_esrs[shire].mtime_local_target);
            return;
        case ESR_THREAD0_DISABLE:
            write_thread0_disable(shire, uint8_t(value & 0xFF));
            LOG_AGENT(DEBUG, agent, "S%u:thread0_disable = 0x%" PRIx8,
                      shireid(shire), uint8_t(shire_other_esrs[shire].thread0_disable));
            return;
        case ESR_SHIRE_COOP_MODE:
            LOG_AGENT(DEBUG, agent, "S%u:shire_coop_mode = 0x%x", shireid(shire), unsigned(value & 0x1));
            write_shire_coop_mode(shire, value);
            return;
        case ESR_ICACHE_UPREFETCH:
            LOG_AGENT(DEBUG, agent, "S%u:icache_uprefetch = 0x%" PRIx64, shireid(shire), uint64_t(value & 0xFFFFFFFFFFFFull));
            write_icache_prefetch(Privilege::U, shire, value & 0xffffffffffffull);
            return;
        case ESR_ICACHE_SPREFETCH:
            LOG_AGENT(DEBUG, agent, "S%u:icache_sprefetch = 0x%" PRIx64, shireid(shire), uint64_t(value & 0xFFFFFFFFFFFFull));
            write_icache_prefetch(Privilege::S, shire, value & 0xffffffffffffull);
            return;
        case ESR_ICACHE_MPREFETCH:
            LOG_AGENT(DEBUG, agent, "S%u:icache_mprefetch = 0x%" PRIx64, shireid(shire), uint64_t(value & 0xFFFFFFFFFFFFull));
            write_icache_prefetch(Privilege::M, shire, value & 0xffffffffffffull);
            return;
        case ESR_CLK_GATE_CTRL:
            shire_other_esrs[shire].clk_gate_ctrl = uint8_t(value & 0x5F);
            LOG_AGENT(DEBUG, agent, "S%u:clk_gate_ctrl = 0x%" PRIx16,
                      shireid(shire), shire_other_esrs[shire].clk_gate_ctrl);
            return;
        case ESR_DMCTRL:
            agent.chip->write_dmctrl(uint32_t(value & 0xF400000F));
            LOG_AGENT(DEBUG, agent, "S%u:dmctrl = 0x%" PRIx32,
                      shireid(shire), uint32_t(value & 0xF400000F));
            return;
        case ESR_SM_CONFIG:
        case ESR_SM_TRIGGER:
            // No-op - emulator has no low level state information.
            return;
        }
        WARN_AGENT(esrs, agent, "Write unknown shire_other ESR S%u:0x%" PRIx64, shireid(shire), esr);
        throw memory_error(addr);
    }

    WARN_AGENT(esrs, agent, "Write illegal ESR S%u:0x%" PRIx64, shireid(shire), addr);
    throw memory_error(addr);
}


void System::write_shire_coop_mode(unsigned shire, uint64_t value)
{
    assert(shire < EMU_NUM_SHIRES);
    value &= 1;
    shire_other_esrs[shire].shire_coop_mode = value;
    if (!value)
        finish_icache_prefetch(shire);
}


void System::write_thread0_disable(unsigned shire, uint32_t value)
{
    shire_other_esrs[shire].thread0_disable = value;
    recalculate_thread0_enable(shire);
}


void System::write_thread1_disable(unsigned shire, uint32_t value)
{
    shire_other_esrs[shire].thread1_disable = value;
    recalculate_thread1_enable(shire);
}


void System::write_minion_feature(unsigned shire, uint8_t value)
{
    shire_other_esrs[shire].minion_feature = value;
    recalculate_thread1_enable(shire);
}


void System::write_icache_prefetch(Privilege /*privilege*/, unsigned shire, uint64_t /*value*/)
{
    assert(shire <= EMU_NUM_COMPUTE_SHIRES);
#ifdef SYS_EMU
    (void)(shire);
#else
    if (!shire_other_esrs[shire].icache_prefetch_active) {
        bool active = shire_other_esrs[shire].shire_coop_mode;
        shire_other_esrs[shire].icache_prefetch_active = active;
    }
#endif
}


uint64_t System::read_icache_prefetch(Privilege /*privilege*/, unsigned shire) const
{
    assert(shire <= EMU_NUM_COMPUTE_SHIRES);
#ifdef SYS_EMU
    (void) shire;
    // NB: Prefetches finish instantaneously in sys_emu
    return 1;
#else
    // External signal returns idle status of prefetch FSM: 1 = idle, 0 = busy
    return !shire_other_esrs[shire].icache_prefetch_active;
#endif
}


void System::finish_icache_prefetch(unsigned shire)
{
    assert(shire <= EMU_NUM_COMPUTE_SHIRES);
#ifndef SYS_EMU
    shire_other_esrs[shire].icache_prefetch_active = 0;
#else
    (void) shire;
#endif
}


} // namespace bemu
