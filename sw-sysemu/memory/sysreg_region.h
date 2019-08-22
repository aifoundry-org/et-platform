/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_SYSREG_REGION_H
#define BEMU_SYSREG_REGION_H

#include <cassert>
#include <cstdint>
#include "esrs.h"
#include "memory_region.h"

namespace bemu {


template <unsigned long long Base, size_t N>
struct SysregRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(Base == ESR_REGION_BASE,
                  "bemu::SysregRegion has illegal base address");
    static_assert(N == ESR_REGION_SIZE,
                  "bemu::SysregRegion has illegal size");

    void read(size_type pos, size_type count, pointer result) override {
        assert(count == 8);
        *reinterpret_cast<uint64_t*>(result) = esr_read(first() + pos);
    }

    void write(size_type pos, size_type count, const_pointer source) override {
        assert(count == 8);
        esr_write(first() + pos, *reinterpret_cast<const uint64_t*>(source));
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::SysregRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};


} // namesapce bemu

#endif // BEMU_SYSREG_REGION_H
