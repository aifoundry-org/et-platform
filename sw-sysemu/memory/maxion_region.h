/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MAXION_REGION_H
#define BEMU_MAXION_REGION_H

#include <algorithm>
#include <stdexcept>
#include "emu_defines.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"

namespace bemu {


extern typename MemoryRegion::reset_value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N>
struct MaxionRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 256_MiB, "bemu::MaxionRegion has illegal size");

    void read(size_type pos, size_type n, pointer result) override {
        extern unsigned current_thread;
        if (current_thread != EMU_IO_SHIRE_SP_THREAD)
            throw memory_error(first() + pos);
        default_value(result, n, memory_reset_value, pos);
    }

    void write(size_type pos, size_type, const_pointer) override {
        extern unsigned current_thread;
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
