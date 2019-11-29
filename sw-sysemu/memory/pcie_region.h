/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_PCIE_REGION_H
#define BEMU_PCIE_REGION_H

#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include <cstring>
#include "emu_gio.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

namespace bemu {

extern typename MemoryRegion::reset_value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N>
struct PcieDbiSlvRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        PF0_TYPE0_HDR_STATUS_COMMAND_REG = 0x04,
        PF0_MSI_CAP                      = 0x50,
        PF0_MSIX_CAP                     = 0xb0,
    };

    void read(size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PcieDbiSlvRegion::read(pos=0x%" PRIx64 ")", (long unsigned int)pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case PF0_MSI_CAP:
            *result32 = (1u << 16) | (0u << 20); /* PCI_MSI_ENABLE = 1, PCI_MSI_MULTIPLE_MSG_EN = 0 */
            break;
        default:
            *result32 = 0;
            std::memcpy(result32, memory_reset_value, std::min(sizeof(memory_reset_value),4UL));            
            break;
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_NOTHREAD(DEBUG, "PcieDbiSlvRegion::write(pos=0x%" PRIx64 ")", (long unsigned int)pos);

        if (n != 4)
            throw memory_error(first() + pos);
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PcieDbiSlvRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};

template<unsigned long long Base, unsigned long long N>
struct PcieNoPcieEsrRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        MSI_TX_VEC = 0x18,
    };

    void read(size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PcieNoPcieEsrRegion::read(pos=0x%" PRIx64 ")", (long unsigned int)pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        default:
          *result32 = 0;
          memcpy(result32, memory_reset_value, std::min(sizeof(memory_reset_value), 4UL));
          break;
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_NOTHREAD(DEBUG, "PcieNoPcieEsrRegion::write(pos=0x%" PRIx64 ")", (long unsigned int)pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case MSI_TX_VEC:
#ifdef SYS_EMU
            if (*source32 & 1) {
                if (sys_emu::get_api_communicate())
                    sys_emu::get_api_communicate()->raise_host_interrupt();
                else
                    LOG(WARN, "%s", "API Communicate is NULL!");
            }
#endif
            break;
        }
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PcieNoPcieEsrRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};

template<unsigned long long Base, unsigned long long N>
struct PcieRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        // base offsets for the various regions of the address space
        r_pcie0_slv_pos     = 0x0000000000,
        r_pcie1_slv_pos     = 0x2000000000,
        r_pcie0_dbi_slv_pos = 0x3E80000000,
        r_pcie1_dbi_slv_pos = 0x3F00000000,
        r_pcie_usresr_pos   = 0x3F80000000,
        r_pcie_nopciesr_pos = 0x3F80001000,
    };

    void read(size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            default_value(result, n, memory_reset_value, pos);
            return;
        }
        elem->read(pos - elem->first(), n, result);
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(pos, n);
        if (elem) {
            try {
                elem->write(pos - elem->first(), n, source);
            } catch (const memory_error&) {
                throw memory_error(first() + pos);
            }
        }
    }

    void init(size_type pos, size_type n, const_pointer source) {
        const auto elem = search(pos, n);
        if (!elem)
            throw std::runtime_error("bemu::PcieRegion::init()");
        elem->init(pos - elem->first(), n, source);
    }

    addr_type first() const { return Base; }
    addr_type last() const { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // Members
    NullRegion          <r_pcie0_slv_pos,   128_GiB>  pcie0_slv{};
    NullRegion          <r_pcie1_slv_pos,   122_GiB>  pcie1_slv{};
    PcieDbiSlvRegion    <r_pcie0_dbi_slv_pos, 2_GiB>  pcie0_dbi_slv{};
    PcieDbiSlvRegion    <r_pcie1_dbi_slv_pos, 2_GiB>  pcie1_dbi_slv{};
    NullRegion          <r_pcie_usresr_pos,   4_KiB>  pcie_usresr{};
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
};


} // namespace bemu

#endif // BEMU_PCIE_REGION_H
