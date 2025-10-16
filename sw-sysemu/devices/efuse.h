/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_EFUSE_H
#define BEMU_EFUSE_H

#include <array>
#include <cstdint>
#include <cassert>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "literals.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct Efuse : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    static_assert(N == 8_KiB, "bemu::Efuse has illegal size");

    static constexpr int EFUSE_SIZE_BYTES = 8192 / 8;
    static constexpr int BANK_SIZE_BYTES = 16;
    static constexpr int LOCK_BITS_BANK = 63;

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "eFuse::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos < EFUSE_SIZE_BYTES) {
            *result32 = ~storage[pos / sizeof(uint32_t)];
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_AGENT(DEBUG, agent, "eFuse::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos < EFUSE_SIZE_BYTES) {
            uint32_t bank = pos / BANK_SIZE_BYTES;
            uint32_t lock_bits = storage[LOCK_BITS_BANK + bank / 32];
            if (!(lock_bits & (1 << (bank % 32)))) {
                storage[pos / sizeof(uint32_t)] = ~*source32;
            }
        }
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint8_t *src = reinterpret_cast<const uint8_t *>(source);
        uint8_t *dst = reinterpret_cast<uint8_t *>(storage.data()) + pos;

        LOG_AGENT(DEBUG, agent, "eFuse::init(pos=0x%llx, n=0x%llx)", pos, n);
        assert(pos + n <= EFUSE_SIZE_BYTES);

        while (n-- > 0) {
            *dst++ = ~*src++;
        }
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

private:
    std::array<uint32_t, EFUSE_SIZE_BYTES / sizeof(uint32_t)> storage{};
};

} // namespace bemu

#endif // BEMU_EFUSE_H
