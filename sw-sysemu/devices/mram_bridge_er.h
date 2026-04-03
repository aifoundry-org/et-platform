/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_MRAM_BRIDGE_ER_H
#define BEMU_MRAM_BRIDGE_ER_H

#include <cstdint>
#include "agent.h"
#include "system.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "emu_gio.h"

namespace bemu {

template <unsigned long long Base, size_t N>
struct MramBridgeEr : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    // Register offsets (64-bit aligned)
    static constexpr size_type ARBITER_MODE      = 0x00;
    static constexpr size_type BRIDGE_STATUS     = 0x08;
    static constexpr size_type SLVERR_STATUS     = 0x10;
    static constexpr size_type CONTROL           = 0x18;
    static constexpr size_type ECC_1BIT_COUNT    = 0x20;
    static constexpr size_type ECC_2BIT_COUNT    = 0x28;
    static constexpr size_type ECC_3BIT_COUNT    = 0x30;

    // bridge_status_reg bits
    static constexpr uint32_t MRAM_READY_MASK    = 0xF00;  // bits [11:8]

    // slverr_status_reg bits
    static constexpr uint32_t SLVERR_MRAM_NOT_READY = 1 << 2;
    static constexpr uint32_t SLVERR_MRAM_UNPOWERED = 1 << 3;

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        (void)n;
        uint64_t val = 0;

        switch (pos) {
        case ARBITER_MODE:
            val = arbiter_mode;
            break;
        case BRIDGE_STATUS:
            // mram_ready reflects current dsleep state
            val = agent.chip->memory.is_mram_dsleep() ? 0 : MRAM_READY_MASK;
            break;
        case SLVERR_STATUS:
            // Clear-on-read
            val = slverr_status;
            slverr_status = 0;
            break;
        case CONTROL:
            val = control;
            break;
        case ECC_1BIT_COUNT:
        case ECC_2BIT_COUNT:
        case ECC_3BIT_COUNT:
            val = 0;
            break;
        default:
            throw memory_error(Base + pos);
        }

        *reinterpret_cast<uint64_t*>(result) = val;
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        (void)agent; (void)n;
        uint64_t val = *reinterpret_cast<const uint64_t*>(source);

        switch (pos) {
        case ARBITER_MODE:
            arbiter_mode = val & 0x3;
            break;
        case BRIDGE_STATUS:
            // Read-only, writes ignored
            break;
        case SLVERR_STATUS:
            // Read-only (clear-on-read), writes ignored
            break;
        case CONTROL:
            control = val;
            break;
        case ECC_1BIT_COUNT:
        case ECC_2BIT_COUNT:
        case ECC_3BIT_COUNT:
            // Counters are read-only from software, writes ignored
            break;
        default:
            throw memory_error(Base + pos);
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::MramBridgeEr::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // Called from PMA to set slverr sticky bits
    void set_slverr(uint32_t bits) { slverr_status |= bits; }

private:
    uint32_t arbiter_mode   = 0x2;  // Reset: round-robin
    uint32_t slverr_status  = 0;
    uint32_t control        = 0;
};

} // namespace bemu

#endif // BEMU_MRAM_BRIDGE_ER_H
