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

#include <array>
#include <cstdint>
#include <functional>
#include "dense_region.h"
#include "devices/uart.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"
#ifdef SYS_EMU
#include "devices/pcie_esr.h"
#include "devices/pcie_apb_subsys.h"
#include "devices/pll.h"
#include "devices/shire_lpddr.h"
#endif
#include "sparse_region.h"

namespace bemu {


extern typename MemoryRegion::reset_value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N>
struct SvcProcRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 1_GiB, "bemu::SvcProcRegion has illegal size");

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        sp_rom_base          = 0x00000000,
        sp_sram_base         = 0x00400000,
        spio_uart0_base      = 0x12022000,
        spio_uart1_base      = 0x14052000,
        pll2_base            = 0x14055000,
        pll4_base            = 0x14057000,
        pcie_esr_base        = 0x18200000,
        pcie_apb_subsys_base = 0x18400000,
        shire_lpddr_base     = 0x20000000,
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            default_value(result, n, memory_reset_value, pos);
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

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // Members
    DenseRegion   <sp_rom_base, 128_KiB, false>  sp_rom{};
    SparseRegion  <sp_sram_base, 1_MiB, 64_KiB>  sp_sram{};
    Uart          <spio_uart0_base,  4_KiB>      spio_uart0{};
    Uart          <spio_uart1_base,  4_KiB>      spio_uart1{};
#ifdef SYS_EMU
    PLL           <pll2_base,        4_KiB, 2>   pll2{};
    PLL           <pll4_base,        4_KiB, 4>   pll4{};
    PcieEsr       <pcie_esr_base,    4_KiB>      pcie_esr{};
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
    std::array<MemoryRegion*,9> regions = {{
        &sp_rom,
        &sp_sram,
        &spio_uart0,
        &spio_uart1,
        &pll2,
        &pll4,
        &pcie_esr,
        &pcie_apb_subsys,
        &shire_lppdr
    }};
#else
    std::array<MemoryRegion*,4> regions = {{
        &sp_rom,
        &sp_sram,
        &spio_uart0,
        &spio_uart1,
    }};
#endif
};


} // namespace bemu

#endif // BEMU_SVCPROC_REGION_H
