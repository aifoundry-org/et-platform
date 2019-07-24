/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_NULL_REGION_H
#define BEMU_NULL_REGION_H

#include <algorithm>
#include <stdexcept>
#include "memory_region.h"

namespace bemu {


extern typename MemoryRegion::value_type memory_reset_value;


template<unsigned long long Base, size_t N>
struct NullRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(!(Base % 64),
                  "bemu::NullRegion must be aligned to 64");
    static_assert((N > 0) && !(N % 64),
                  "bemu::NullRegion size must be a multiple of 64");

    void read(size_type, size_type n, pointer result) const override {
        std::fill_n(result, n, memory_reset_value);
    }

    void write(size_type, size_type, const_pointer) override { }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::NullRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_NULL_REGION_H
