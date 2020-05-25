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
#include "dump_data.h"
#include "lazy_array.h"
#include "memory_error.h"
#include "memory_region.h"

namespace bemu {


extern typename MemoryRegion::reset_value_type memory_reset_value;


template <unsigned long long Base, unsigned long long N, unsigned long long M,
          bool Writeable=true>
struct SparseRegion : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;
    typedef lazy_array<value_type,M>              bucket_type;
    typedef std::array<bucket_type,N/M>           storage_type;

    static_assert(!(Base % 64),
                  "bemu::SparseRegion must be aligned to 64");
    static_assert((M & (M - 1)) == 0,
                  "bemu::SparseRegion bucket size must be a power of 2");
    static_assert((M > 0) && !(M % 64),
                  "bemu::SparseRegion bucket size must be a multiple of 64");
    static_assert((N > 0) && !(N % M),
                  "bemu::SparseRegion size must be a multiple of bucket size");

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        size_type bucket = pos / M;
        size_type offset = pos % M;
        size_type count = std::min(n, M - offset);
        n -= read_bucket(bucket, offset, count, result);
        while (n > 0) {
            result += count;
            count = std::min(n, M);
            n -= read_bucket(++bucket, 0, count, result);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (!Writeable)
            throw memory_error(first() + pos);
        init(agent, pos, n, source);
    }

    void init(const Agent&, size_type pos, size_type n, const_pointer source) override {
        size_type bucket = pos / M;
        size_type offset = pos % M;
        size_type count = std::min(n, M - offset);
        n -= write_bucket(bucket, offset, count, source);
        while (n > 0) {
            source += count;
            count = std::min(n, M);
            n -= write_bucket(++bucket, 0, count, source);
        }
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream& os, size_type pos, size_type n) const override {
        size_type lo = pos / M;
        size_type hi = (pos + n - 1) / M;
        size_type offset = pos % M;
        while (lo != hi) {
            bemu::dump_data(os, storage[lo], offset,
                            M - offset, memory_reset_value[0]);
            ++lo;
            offset = 0;
        }
        bemu::dump_data(os, storage[lo], offset,
                        1 + ((pos + n - 1) % M) - offset, memory_reset_value[0]);
    }

    // For exposition only
    storage_type  storage;

protected:
    size_type read_bucket(size_type bucket, size_type pos,
                          size_type count, pointer result) const
    {
        if (storage[bucket].empty()) {
          default_value(result, count, memory_reset_value, pos);
        } else {
            std::copy_n(storage[bucket].cbegin() + pos, count, result);
        }
        return count;
    }

    size_type write_bucket(size_type bucket, size_type pos,
                           size_type count, const_pointer source)
    {
        if (storage[bucket].empty()) {
            storage[bucket].allocate();
            storage[bucket].fill_pattern(memory_reset_value, MEM_RESET_PATTERN_SIZE);
        }
        std::copy_n(source, count, storage[bucket].begin() + pos);
        return count;
    }
};


} // namespace bemu

#endif // BEMU_SPARSE_REGION_H
