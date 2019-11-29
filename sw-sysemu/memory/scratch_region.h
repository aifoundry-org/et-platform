/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_SCRATCH_REGION_H
#define BEMU_SCRATCH_REGION_H

#include <algorithm>
#include <array>
#include "esrs.h"
#include "lazy_array.h"
#include "memory_error.h"
#include "memory_region.h"

extern unsigned current_thread;

namespace bemu {


extern typename MemoryRegion::reset_value_type memory_reset_value;


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

    void read(size_type pos, size_type n, pointer result) override {
        read_const(pos, n, result);
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        if (!Writeable)
            throw memory_error(first() + pos);
        init(pos, n, source);
    }

    void init(size_type pos, size_type n, const_pointer source) override {
        size_type index = normalize(pos);
        size_type bucket = slice(index);
        size_type offset = index % 8_MiB;
        if (out_of_range(bucket, offset, n)) {
            throw memory_error(first() + pos);
        }
        if (storage[bucket].empty()) {
            storage[bucket].allocate();
            storage[bucket].fill_pattern(memory_reset_value, MEM_RESET_PATTERN_SIZE);
        }
        std::copy_n(source, n, storage[bucket].begin() + offset);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + 2_GiB; }

    void dump_data(std::ostream& os, size_type pos, size_type n) const override {
        value_type elem;
        while (n-- > 0) {
            this->read_const(pos++, 1, &elem);
            os.write(reinterpret_cast<const char*>(&elem), sizeof(value_type));
        }
    }

    // For exposition only
    storage_type  storage;

protected:
    void read_const(size_type pos, size_type n, pointer result) const {
        size_type index = normalize(pos);
        size_type bucket = slice(index);
        size_type offset = index % 8_MiB;
        if (out_of_range(bucket, offset, n)) {
            throw memory_error(first() + pos);
        }
        if (storage[bucket].empty()) {
            default_value(result, n, memory_reset_value, pos);
        } else {
            std::copy_n(storage[bucket].cbegin() + offset, n, result);
        }
    }

    size_type normalize(size_type pos) const {
        if (pos >= 1_GiB) {
            pos = ( (pos         & ~0x4fffffc0ull) |
                    ((pos <<  1) &  0x40000000ull) |
                    ((pos << 17) &  0x0f800000ull) |
                    ((pos >>  5) &  0x007fffc0ull) );
        }
        return pos | ((pos << 1) & 0x40000000ull);
    }

    size_type slice(size_type pos) const {
        size_type num = (pos >> 23) & 255;
        if (num == 255) return ::current_thread / EMU_THREADS_PER_SHIRE;
        if (num == IO_SHIRE_ID) return EMU_IO_SHIRE_SP;
        return num;
    }

    bool out_of_range(size_type bucket, size_type offset, size_type n) const {
        // sc_scp_cache_ctl.set_size holds the scratchpad size in sets per
        // subbank, and we have 4 subbanks per each of the 4 banks, and each
        // set has 4 ways of 64B lines, so each 'set' holds 4KiB of data.
        // Only check bank 0 because the spec says that all banks must have
        // the same cfg, otherwise behavior is undefined.
        if (bucket >= M)
            return true;
        uint64_t cfg = shire_cache_esrs[bucket].bank[0].sc_scp_cache_ctl;
        size_type bucket_size = std::min(size_type((cfg >> 20) & 0x1fff000), N);
        return (bucket_size < offset) || (n > bucket_size - offset);
    }
};


} // namespace bemu

#endif // BEMU_SCRATCH_REGION_H
