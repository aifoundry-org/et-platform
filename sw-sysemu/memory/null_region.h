/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_NULL_REGION_H
#define BEMU_NULL_REGION_H

#include <algorithm>
#include <stdexcept>
#include "memory/memory_region.h"

namespace bemu {


template<unsigned long long Base, unsigned long long N>
struct NullRegion : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    static_assert(!(Base % 64),
                  "bemu::NullRegion must be aligned to 64");
    static_assert((N > 0) && !(N % 64),
                  "bemu::NullRegion size must be a multiple of 64");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        default_value(result, n, agent.chip->memory_reset_value, pos);
    }

    void write(const Agent&, size_type, size_type, const_pointer) override { }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::NullRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_NULL_REGION_H
