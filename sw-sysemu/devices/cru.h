/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_CRU_H
#define BEMU_CRU_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct Cru : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    enum : unsigned {
        RESET_MANAGER_RM_STATUS2_ADDRESS = 0x254u
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        LOG_AGENT(DEBUG, agent, "Cru::read(pos=0x%llx)", pos);
        if (n != 4) {
            throw memory_error(first() + pos);
        }
        *reinterpret_cast<uint32_t*>(result) = 0;
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer) override {
        LOG_AGENT(DEBUG, agent, "Cru::write(pos=0x%llx)", pos);
        if (n != 4) {
            throw memory_error(first() + pos);
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::Cru::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};

} // namespace bemu

#endif // BEMU_CRU_H
