/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SVCPROC_REGION_H
#define BEMU_SVCPROC_REGION_H

#include <algorithm>
#include <array>
#include <cstdint>
#include "literals.h"
#include "memory/dense_region.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "memory/sparse_region.h"
#include "devices/uart.h"
#ifdef SYS_EMU
#include "devices/cru.h"
#include "devices/DW_apb_timers.h"
#include "devices/efuse.h"
#include "devices/i2c.h"
#include "devices/pcie_apb_subsys.h"
#include "devices/pcie_esr.h"
#include "devices/plic.h"
#include "devices/pll.h"
#include "devices/shire_lpddr.h"
#include "devices/spi.h"
#include "devices/spio_rvtimer_region.h"
#include "devices/pvtc.h"
#endif

namespace bemu {


template<unsigned long long Base, unsigned long long N>
struct SvcProcRegion : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    static_assert(N == 1_GiB, "bemu::SvcProcRegion has illegal size");

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        sp_rom_base          = 0x00000000,
        sp_sram_base         = 0x00400000,
        sp_plic_base         = 0x10000000,
        sp_i2c0_base         = 0x12020000,
        sp_spi0_base         = 0x12021000,
        sp_uart0_base        = 0x12022000,
        sp_timer_base        = 0x12025000,
        sp_efuse_base        = 0x12026000,
        sp_cru_base          = 0x12028000,
        sp_rvtim_base        = 0x12100000,
        pvtc0_base           = 0x14000000,
        pvtc1_base           = 0x14010000,
        pvtc2_base           = 0x14020000,
        pvtc3_base           = 0x14030000,
        pvtc4_base           = 0x14040000,
        sp_i2c1_base         = 0x14050000,
        sp_spi1_base         = 0x14051000,
        sp_uart1_base        = 0x14052000,
        pll0_base            = 0x14053000,
        pll1_base            = 0x14054000,
        pll2_base            = 0x14055000,
        pll3_base            = 0x14056000,
        pll4_base            = 0x14057000,
        pcie_esr_base        = 0x18200000,
        pcie_pllp0_base      = 0x18201000,
        pcie_apb_subsys_base = 0x18400000,
        shire_lpddr_base     = 0x20000000,
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            default_value(result, n, agent.chip->memory_reset_value, pos);
            return;
        }
        elem->read(agent, pos - elem->first(), n, result);
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(pos, n);
        if (elem) {
            try {
                elem->write(agent, pos - elem->first(), n, source);
            } catch (const memory_error&) {
                throw memory_error(first() + pos);
            }
        }
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(pos, n);
        if (!elem)
            throw std::runtime_error("bemu::SvcProcRegion::init()");
        elem->init(agent, pos - elem->first(), n, source);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // Members
    DenseRegion   <sp_rom_base, 128_KiB, false>  sp_rom{};
    SparseRegion  <sp_sram_base, 1_MiB, 64_KiB>  sp_sram{};
    SP_PLIC       <sp_plic_base,    32_MiB>      sp_plic{};
    Uart          <sp_uart0_base,  4_KiB>        spio_uart0{};
    Uart          <sp_uart1_base,  4_KiB>        spio_uart1{};
#ifdef SYS_EMU
    DW_apb_timers <sp_timer_base,    4_KiB>      sp_timer{};
    Efuse         <sp_efuse_base,    8_KiB>      sp_efuse{};
    Cru           <sp_cru_base,      4_KiB>      sp_cru{};
    I2c           <sp_i2c0_base,     4_KiB, 0>   sp_i2c0{};
    Spi           <sp_spi0_base,     4_KiB, 0>   sp_spi0{};
    I2c           <sp_i2c1_base,     4_KiB, 1>   sp_i2c1{};
    Spi           <sp_spi1_base,     4_KiB, 1>   sp_spi1{};
    SpioRVTimerRegion <sp_rvtim_base,4_KiB>      sp_rvtim{};
    PVTC          <pvtc0_base,       64_KiB, 0>  pvtc0{};
    PVTC          <pvtc1_base,       64_KiB, 1>  pvtc1{};
    PVTC          <pvtc2_base,       64_KiB, 2>  pvtc2{};
    PVTC          <pvtc3_base,       64_KiB, 3>  pvtc3{};
    PVTC          <pvtc4_base,       64_KiB, 4>  pvtc4{};
    PLL           <pll0_base,        4_KiB, 0>   pll0{};
    PLL           <pll1_base,        4_KiB, 1>   pll1{};
    PLL           <pll2_base,        4_KiB, 2>   pll2{};
    PLL           <pll3_base,        4_KiB, 3>   pll3{};
    PLL           <pll4_base,        4_KiB, 4>   pll4{};
    PcieEsr       <pcie_esr_base,    4_KiB>      pcie_esr{};
    PLL           <pcie_pllp0_base,  4_KiB, 5>   pcie_pllp0{};
    PcieApbSubsys <pcie_apb_subsys_base, 2_MiB>  pcie_apb_subsys{};
    ShireLpddr    <shire_lpddr_base, 512_MiB>    shire_lppdr;
#endif

protected:
    static inline bool above(const MemoryRegion* lhs, size_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(size_type pos, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), pos, above);
        if ((lo == regions.cend()) || ((*lo)->first() > pos))
            return nullptr;
        if (pos+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::SvcProcRegion::search()");
        return *lo;
    }

    // These arrays must be sorted by region offset
#ifdef SYS_EMU
    std::array<MemoryRegion*,27> regions = {{
        &sp_rom,
        &sp_sram,
        &sp_plic,
        &sp_i2c0,
        &sp_spi0,
        &spio_uart0,
        &sp_timer,
        &sp_efuse,
        &sp_cru,
        &sp_rvtim,
        &pvtc0,
        &pvtc1,
        &pvtc2,
        &pvtc3,
        &pvtc4,
        &sp_i2c1,
        &sp_spi1,
        &spio_uart1,
        &pll0,
        &pll1,
        &pll2,
        &pll3,
        &pll4,
        &pcie_esr,
        &pcie_pllp0,
        &pcie_apb_subsys,
        &shire_lppdr
    }};
#else
    std::array<MemoryRegion*,5> regions = {{
        &sp_rom,
        &sp_sram,
        &sp_plic,
        &spio_uart0,
        &spio_uart1,
    }};
#endif
};


} // namespace bemu

#endif // BEMU_SVCPROC_REGION_H
