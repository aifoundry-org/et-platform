/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_MAIN_MEMORY_H
#define BEMU_MAIN_MEMORY_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include "agent.h"
#include "literals.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

#include <hwinc/top.h>
namespace bemu {


// Erbium Memory Map
//
// +---------------------------------+----------+-------------------+
// |      Address range (hex)        |          |                   |
// |      From      |      To        |   Size   | Maps to           |
// +----------------+----------------+----------+-------------------+
// | 0x00_0200_0000 | 0x00_0200_0FFF |  4KiB    | SystemRegisters   |
// | 0x00_0200_4000 | 0x00_0200_4FFF |  4KiB    | UART              |
// | 0x00_0200_A000 | 0x00_0200_BFFF |  8KiB    | Boot ROM          |
// | 0x00_0200_C000 | 0x00_0200_CFFF |  4KiB    | Scratch SRAM      |
// | 0x00_4000_0000 | 0x00_40FF_FFFF | 16MiB    | MRAM              |
// | 0x00_7FFF_D000 | 0x00_7FFF_FFFF | 12KiB    | OTP (read-only)   |
// | 0x00_8000_0000 | 0x00_80FF_FFFF | 16MiB    | ESR Registers     |
// | 0x00_A000_0000 | 0x00_A3FF_FFFF | 64MiB    | PLIC              |
// +----------------+----------------+----------+-------------------+
//

struct MainMemory {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

private:
    enum : unsigned {
        erbreg_idx,
        mram_bridge_idx,
        uart_idx,
        bootrom_idx,
        sram_idx,
        dram_idx,
        otp_idx,
        sysreg_idx,
        plic_idx,

        REGION_COUNT
    };

    constexpr static uint64_t region_bases[REGION_COUNT] = {
        /* erbreg  */ ERBIUM_TOP_SYSTEM_REGISTERS_BASE,
        /* mram_bridge */ ERBIUM_TOP_MRAM_REGISTERS_BASE,
        /* uart    */ ERBIUM_TOP_UART_REGISTERS_BASE,
        /* bootrom */ ERBIUM_TOP_BOOTROM_BASE,
        /* sram    */ ERBIUM_TOP_SRAM_BASE,
        /* dram    */ ERBIUM_TOP_MRAM_BASE,
        /* otp     */ 0x007FFFD000ull,
        /* sysreg  */ ERBIUM_TOP_CPU_REGISTERS_BASE,
        /* plic    */ ERBIUM_TOP_PLIC_BASE,
    };

    constexpr static size_t region_sizes[REGION_COUNT] = {
        /* erbreg  */ ERBIUM_TOP_MRAM_REGISTERS_BASE - ERBIUM_TOP_SYSTEM_REGISTERS_BASE,
        /* mram_bridge */ ERBIUM_TOP_I2C_REGISTERS_BASE - ERBIUM_TOP_MRAM_REGISTERS_BASE,
        /* uart    */ 4_KiB,
        /* bootrom */ ERBIUM_TOP_BOOTROM_SIZE,
        /* sram    */ ERBIUM_TOP_SRAM_SIZE,
        /* dram    */ ERBIUM_TOP_MRAM_SIZE,
        /* otp     */ 12_KiB,
        /* sysreg  */ 16_MiB,
        /* plic    */ 64_MiB,
    };

public:
    // ----- Public methods -----

    void reset();

    void read(const Agent& agent, addr_type addr, size_type n, void* result) {
        const auto elem = search(addr, n);
        elem->read(agent, addr - elem->first(), n, reinterpret_cast<pointer>(result));
    }

    void write(const Agent& agent, addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->write(agent, addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    void init(const Agent& agent, addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->init(agent, addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    addr_type first() const { return regions.front()->first(); }
    addr_type last() const { return regions.back()->last(); }

    void dump_data(const Agent& agent, std::ostream& os, addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        auto hi = std::lower_bound(regions.cbegin(), regions.cend(), addr+n-1, above);
        if (hi == regions.cend())
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        size_type pos = addr - (*lo)->first();
        while (lo != hi) {
            (*lo)->dump_data(agent, os, pos, (*lo)->last() - (*lo)->first() - pos + 1);
            ++lo;
            pos = 0;
        }
        (*lo)->dump_data(agent, os, pos, addr + n - (*lo)->first() - pos);
    }

    void wdt_clock_tick(const Agent& agent, uint64_t cycle);

    // UART helpers
    void uart_set_tx_fd(int fd);
    void uart_set_rx_fd(int fd);
    int uart_get_tx_fd() const;
    int uart_get_rx_fd() const;
    bool is_uart_enabled() const;
    void uart_clock_tick(const Agent& agent, uint64_t cycle);

    // PLIC helpers
    void plic_interrupt_pending_set(const Agent&, uint32_t source);
    void plic_interrupt_pending_clear(const Agent&, uint32_t source);

    // RVTimer helpers
    bool rvtimer_is_active() const;
    uint64_t rvtimer_read_mtime() const;
    uint64_t rvtimer_read_mtimecmp() const;
    uint64_t rvtimer_read_time_config() const;
    void rvtimer_clock_tick(const Agent&, uint64_t cycle);
    void rvtimer_write_mtime(const Agent&, uint64_t value);
    void rvtimer_write_mtimecmp(const Agent&, uint64_t value);
    void rvtimer_write_time_config(const Agent&, uint64_t value);
    void rvtimer_reset();

protected:
    static inline bool above(const std::unique_ptr<MemoryRegion>& lhs, addr_type rhs) {
        return lhs->last() < rhs;
    }

    inline auto& rvtimer() const;

    MemoryRegion* search(addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw memory_error(addr);
        if (addr+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::MainMemory::search()");
        return lo->get();
    }

    // This array must be sorted by region base address
    std::array<std::unique_ptr<MemoryRegion>, REGION_COUNT> regions{};
};


} // namespace bemu

#endif // BEMU_MAIN_MEMORY_H
