/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DENSE_REGION_H
#define BEMU_DENSE_REGION_H

#include <algorithm>
#include <array>
#include "lazy_array.h"
#include "memory/dump_data.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {


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

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        if (storage.empty()) {
            default_value(result, n, agent.chip->memory_reset_value, pos);
        } else {
            std::copy_n(storage.cbegin() + pos, n, result);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (!Writeable)
            throw memory_error(first() + pos);
        init(agent, pos, n, source);
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (storage.empty()) {
            storage.allocate();
            storage.fill_pattern(agent.chip->memory_reset_value, MEM_RESET_PATTERN_SIZE);
        }
        std::copy_n(source, n, storage.begin() + pos);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent& agent, std::ostream& os, size_type pos, size_type n) const override {
        bemu::dump_data(os, storage, pos, n, agent.chip->memory_reset_value[0]);
    }

    // For exposition only
    storage_type  storage;
};


} // namespace bemu

#endif // BEMU_DENSE_REGION_H
