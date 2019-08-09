/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAXION_REGION_H
#define BEMU_MAXION_REGION_H

#include <algorithm>
#include <stdexcept>
#include "emu_defines.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"

extern unsigned current_thread;

namespace bemu {


extern typename MemoryRegion::value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N>
struct MaxionRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 256_MiB, "bemu::MaxionRegion has illegal size");

    void read(size_type pos, size_type n, pointer result) override {
        if (current_thread != EMU_IO_SHIRE_SP_THREAD)
            throw memory_error(first() + pos);
        std::fill_n(result, n, memory_reset_value);
    }

    void write(size_type pos, size_type, const_pointer) override {
        if (current_thread != EMU_IO_SHIRE_SP_THREAD)
            throw memory_error(first() + pos);
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::MaxionRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_MAXION_REGION_H
