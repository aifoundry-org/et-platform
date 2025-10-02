/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PCIE_REGION_H
#define BEMU_PCIE_REGION_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include "emu_gio.h"
#include "literals.h"
#include "system.h"
#include "devices/pcie_dma.h"
#include "devices/pcie_dbi_slv.h"
#include "devices/pcie_nopcie_esr.h"
#include "devices/pcie_usr_esr.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "memory/null_region.h"

namespace bemu {


template<unsigned long long Base, unsigned long long N>
struct PcieRegion : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    enum : unsigned long long {
        // base offsets for the various regions of the address space
        r_pcie0_slv_pos     = 0x0000000000,
        r_pcie1_slv_pos     = 0x2000000000,
        r_pcie0_dbi_slv_pos = 0x3E80000000,
        r_pcie1_dbi_slv_pos = 0x3F00000000,
        r_pcie_usresr_pos   = 0x3F80000000,
        r_pcie_nopciesr_pos = 0x3F80001000,
    };

    PcieRegion() {
        for (int i = 0; i < ETSOC_CC_NUM_DMA_WR_CHAN; i++) {
            pcie0_dma_wrch[i].chan_id = i;
            pcie1_dma_wrch[i].chan_id = i;
        }

        for (int i = 0; i < ETSOC_CC_NUM_DMA_RD_CHAN; i++) {
            pcie0_dma_rdch[i].chan_id = i;
            pcie1_dma_rdch[i].chan_id = i;
        }
    }

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            default_value(result, n, agent.chip->memory_reset_value, pos);
            return;
        }
        try {
            elem->read(agent, pos - elem->first(), n, result);
        } catch (const memory_error&) {
            throw memory_error(first() + pos);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        const auto &msix_match_low = pcie0_dbi_slv.msix_match_low;
        const auto &msix_match_high = pcie0_dbi_slv.msix_match_high;

        // First check if  MSI-X Address Match feature is enabled
        if (msix_match_low & 1) {
            uint64_t match_addr = ((uint64_t)msix_match_high << 32) | (msix_match_low & ~3u);
            // Check if accessed address matches MSI-X address
            if (pos == (match_addr - Base)) {
                if (n < 4) {
                    throw memory_error(first() + pos);
                }
                agent.chip->raise_host_interrupt(1ul << *source32);
                return;
            }
        }

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
            throw std::runtime_error("bemu::PcieRegion::init()");
        elem->init(agent, pos - elem->first(), n, source);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // Members
    NullRegion          <r_pcie0_slv_pos,   128_GiB>  pcie0_slv{};
    NullRegion          <r_pcie1_slv_pos,   122_GiB>  pcie1_slv{};
    PcieDbiSlvRegion    <r_pcie0_dbi_slv_pos, 2_GiB, 0> pcie0_dbi_slv{pcie0_dma_wrch, pcie0_dma_rdch};
    PcieDbiSlvRegion    <r_pcie1_dbi_slv_pos, 2_GiB, 1> pcie1_dbi_slv{pcie1_dma_wrch, pcie1_dma_rdch};
    PcieUsrEsrRegion    <r_pcie_usresr_pos,   4_KiB>  pcie_usresr{pcie0_dma_wrch, pcie1_dma_wrch,
                                                                  pcie0_dma_rdch, pcie1_dma_rdch};
    PcieNoPcieEsrRegion <r_pcie_nopciesr_pos, 4_KiB>  pcie_nopciesr{};

protected:
    static inline bool above(const MemoryRegion* lhs, size_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(size_type pos, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), pos, above);
        if ((lo == regions.cend()) || ((*lo)->first() > pos))
            return nullptr;
        if (pos+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::PcieRegion::search()");
        return *lo;
    }

    // These arrays must be sorted by region offset
    std::array<MemoryRegion*,6> regions = {{
        &pcie0_slv,
        &pcie1_slv,
        &pcie0_dbi_slv,
        &pcie1_dbi_slv,
        &pcie_usresr,
        &pcie_nopciesr,
    }};

    std::array<PcieDma<true>,  ETSOC_CC_NUM_DMA_WR_CHAN> pcie0_dma_wrch{{},};
    std::array<PcieDma<true>,  ETSOC_CC_NUM_DMA_WR_CHAN> pcie1_dma_wrch{{},};
    std::array<PcieDma<false>, ETSOC_CC_NUM_DMA_RD_CHAN> pcie0_dma_rdch{{},};
    std::array<PcieDma<false>, ETSOC_CC_NUM_DMA_RD_CHAN> pcie1_dma_rdch{{},};
};


} // namespace bemu

#endif // BEMU_PCIE_REGION_H
