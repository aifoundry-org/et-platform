/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_DENSE_REGION_H
#define BEMU_DENSE_REGION_H

#include <algorithm>
#include <array>
#include "dump_data.h"
#include "lazy_array.h"
#include "memory_error.h"
#include "memory_region.h"

namespace bemu {


extern typename MemoryRegion::value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N, bool Writeable=true>
struct DenseRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;
    typedef lazy_array<value_type,N>              storage_type;

    static_assert(!(Base % 64),
                  "bemu::DenseRegion must be aligned to 64");
    static_assert((N > 0) && !(N % 64),
                  "bemu::DenseRegion size must be a multiple of 64");

    void read(size_type pos, size_type n, pointer result) override {
        if (storage.empty()) {
            std::fill_n(result, n, memory_reset_value);
        } else {
            std::copy_n(storage.cbegin() + pos, n, result);
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        if (!Writeable)
            throw memory_error(first() + pos);
        init(pos, n, source);
    }

    void init(size_type pos, size_type n, const_pointer source) override {
        if (storage.empty()) {
            storage.allocate();
            storage.fill(memory_reset_value);
        }
        std::copy_n(source, n, storage.begin() + pos);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream& os, size_type pos, size_type n) const override {
        bemu::dump_data(os, storage, pos, n, memory_reset_value);
    }

    // For exposition only
    storage_type  storage;
};


} // namespace bemu

#endif // BEMU_DENSE_REGION_H
