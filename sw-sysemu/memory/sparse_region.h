/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SPARSE_REGION_H
#define BEMU_SPARSE_REGION_H

#include <algorithm>
#include <array>
#include "support/lazy_array.h"
#include "memory/dump_data.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {


template <unsigned long long Base, unsigned long long N, unsigned long long M,
          bool Writeable=true>
struct SparseRegion : public MemoryRegion
{
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;
    using bucket_type   = lazy_array<value_type,M>;
    using storage_type  = std::array<bucket_type,N/M>;

    static_assert(!(Base % 64),
                  "bemu::SparseRegion must be aligned to 64");
    static_assert((M & (M - 1)) == 0,
                  "bemu::SparseRegion bucket size must be a power of 2");
    static_assert((M > 0) && !(M % 64),
                  "bemu::SparseRegion bucket size must be a multiple of 64");
    static_assert((N > 0) && !(N % M),
                  "bemu::SparseRegion size must be a multiple of bucket size");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        size_type bucket = pos / M;
        size_type offset = pos % M;
        size_type count = std::min(n, M - offset);
        n -= read_bucket(agent.chip, bucket, offset, count, result);
        while (n > 0) {
            result += count;
            count = std::min(n, M);
            n -= read_bucket(agent.chip, ++bucket, 0, count, result);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (!Writeable)
            throw memory_error(first() + pos);
        init(agent, pos, n, source);
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        size_type bucket = pos / M;
        size_type offset = pos % M;
        size_type count = std::min(n, M - offset);
        n -= write_bucket(agent.chip, bucket, offset, count, source);
        while (n > 0) {
            source += count;
            count = std::min(n, M);
            n -= write_bucket(agent.chip, ++bucket, 0, count, source);
        }
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent& agent, std::ostream& os, size_type pos, size_type n) const override {
        size_type lo = pos / M;
        size_type hi = (pos + n - 1) / M;
        size_type offset = pos % M;
        while (lo != hi) {
            bemu::dump_data(os, storage[lo], offset,
                            M - offset, agent.chip->memory_reset_value[0]);
            ++lo;
            offset = 0;
        }
        bemu::dump_data(os, storage[lo], offset,
                        1 + ((pos + n - 1) % M) - offset, agent.chip->memory_reset_value[0]);
    }

    // For exposition only
    storage_type  storage;

protected:
    size_type read_bucket(System* system, size_type bucket, size_type pos,
                          size_type count, pointer result) const
    {
        if (storage[bucket].empty()) {
          default_value(result, count, system->memory_reset_value, pos);
        } else {
            std::copy_n(storage[bucket].cbegin() + pos, count, result);
        }
        return count;
    }

    size_type write_bucket(System* system, size_type bucket, size_type pos,
                           size_type count, const_pointer source)
    {
        if (storage[bucket].empty()) {
            storage[bucket].allocate();
            storage[bucket].fill_pattern(system->memory_reset_value, MEM_RESET_PATTERN_SIZE);
        }
        std::copy_n(source, count, storage[bucket].begin() + pos);
        return count;
    }
};


} // namespace bemu

#endif // BEMU_SPARSE_REGION_H
