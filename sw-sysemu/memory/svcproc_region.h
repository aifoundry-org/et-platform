/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_SVCPROC_REGION_H
#define BEMU_SVCPROC_REGION_H

#include <array>
#include <cstdint>
#include <functional>
#include "dense_region.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"
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
        sp_rom_base    = 0x00000000,
        sp_sram_base   = 0x00400000,
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
            throw std::runtime_error("bemu::SvcProcRegion::init()");
        elem->init(pos - elem->first(), n, source);
    }

    addr_type first() const { return Base; }
    addr_type last() const { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // Members
    DenseRegion  <sp_rom_base, 128_KiB, false>  sp_rom{};
    SparseRegion <sp_sram_base, 1_MiB, 64_KiB>  sp_sram{};

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
    std::array<MemoryRegion*,2> regions = {{
        &sp_rom,
        &sp_sram,
    }};
};


} // namespace bemu

#endif // BEMU_SVCPROC_REGION_H
