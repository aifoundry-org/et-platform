/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SCRATCH_REGION_H
#define BEMU_SCRATCH_REGION_H

#include <algorithm>
#include <array>
#include "lazy_array.h"
#include "system.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {


template <unsigned long long Base, unsigned long long N, unsigned long long M,
          bool Writeable=true>
struct ScratchRegion : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;
    typedef lazy_array<value_type,N>              bucket_type;
    typedef std::array<bucket_type,M>             storage_type;

    static_assert(!(Base % 8_KiB),
                  "bemu::ScratchRegion must be aligned to 8KiB");
    static_assert(N <= 8_MiB,
                  "bemu::ScratchRegion bucket size must be at most 8MiB");
    static_assert((N > 0) && !(N % 8_KiB),
                  "bemu::ScratchRegion bucket size must be a multiple of 8KiB");
    static_assert((M > 0) && (M < 128),
                  "bemu::ScratchRegion must have at most 127 buckets");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        if (!read_impl(agent.chip, pos, n, result)) {
            throw memory_error(first() + pos);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (!Writeable || !write_impl(agent.chip, pos, n, source))
            throw memory_error(first() + pos);
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        if (!write_impl(agent.chip, pos, n, source))
            throw memory_error(first() + pos);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + 2_GiB; }

    void dump_data(const Agent& agent, std::ostream& os, size_type pos, size_type n) const override {
        value_type elem;
        while (n-- > 0) {
            if (!read_impl(agent.chip, pos++, 1, &elem)) {
                throw std::runtime_error("bemu::ScratchRegion::dump_data()");
            }
            os.write(reinterpret_cast<const char*>(&elem), sizeof(value_type));
        }
    }

    // For exposition only
    storage_type  storage;

protected:
    bool read_impl(const System* chip, size_type pos, size_type n, pointer result) const {
        size_type bucket = slice(pos);
        size_type offset = pos % 8_MiB;
        if (out_of_range(chip, bucket, offset, n)) {
            return false;
        }
        if (storage[bucket].empty()) {
            default_value(result, n, chip->memory_reset_value, pos);
        } else {
            std::copy_n(storage[bucket].cbegin() + offset, n, result);
        }
        return true;
    }

    bool write_impl(const System* chip, size_type pos, size_type n, const_pointer source) {
        size_type bucket = slice(pos);
        size_type offset = pos % 8_MiB;
        if (out_of_range(chip, bucket, offset, n)) {
            return false;
        }
        if (storage[bucket].empty()) {
            storage[bucket].allocate();
            storage[bucket].fill_pattern(chip->memory_reset_value, MEM_RESET_PATTERN_SIZE);
        }
        std::copy_n(source, n, storage[bucket].begin() + offset);
        return true;
    }

    size_type slice(size_type pos) const {
        size_type num = (pos >> 23) & 255;
        return (num == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : num;
    }

    bool out_of_range(const System* chip, size_type bucket, size_type offset, size_type n) const {
        // sc_scp_cache_ctl.set_size holds the scratchpad size in sets per
        // subbank, and we have 4 subbanks per each of the 4 banks, and each
        // set has 4 ways of 64B lines, so each 'set' holds 4KiB of data.
        // Only check bank 0 because the spec says that all banks must have
        // the same cfg, otherwise behavior is undefined.
        if (bucket >= M)
            return true;
        uint64_t cfg = chip->shire_cache_esrs[bucket].bank[0].sc_scp_cache_ctl;
        size_type bucket_size = std::min(size_type((cfg >> 20) & 0x1fff000), N);
        return (bucket_size < offset) || (n > bucket_size - offset);
    }
};


} // namespace bemu

#endif // BEMU_SCRATCH_REGION_H
