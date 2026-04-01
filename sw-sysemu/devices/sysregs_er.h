/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_SYSREGS_ER_H
#define BEMU_SYSREGS_ER_H

#include <cstdint>
#include <stdexcept>
#include "memory/memory_region.h"
#include "agent.h"
#include "system.h"
#include "devices/watchdog.h"
#include "emu_defines.h"

namespace bemu {

// TODO: move to reset
// Reset cause reasons
enum class ResetCause {
    NONE            = 0x0,
    POR             = (1 << 0),  // Power-On Reset
    WATCHDOG        = (1 << 1),  // Watchdog timeout
    SYSRESET        = (1 << 2),  // System reset request
    BROWNOUT        = (1 << 3),  // Brownout detector
};


template <uint64_t Base>
struct SysregsEr : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;
    
    // Constructor - initializes to power-on reset state
    SysregsEr() {
        reset(ResetCause::POR);
    }

    void read(const Agent& agent, size_type pos, size_type count, pointer result) override;

    void write(const Agent& agent, size_type pos, size_type count, const_pointer source) override;

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::ErbiumRegRegion::init()");
    }
    
    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + LAST_OFFSET + 7; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    void wdt_clock_tick(const Agent& agent, uint64_t cycle);

    bool is_uart_enabled() const { return system_config & SYSTEM_CONFIG_UART_ENABLE; }
    bool is_mram_dsleep() const { return !mram_powered; }

private:

    // Register Offsets
    static constexpr uint64_t VERSION           = 0x00;
    static constexpr uint64_t SYSTEM_CONFIG     = 0x08;
    static constexpr uint64_t WATCHDOG_COUNT    = 0x10;
    static constexpr uint64_t WATCHDOG          = 0x18;
    static constexpr uint64_t SYS_INTERRUPT     = 0x20;
    static constexpr uint64_t SOFT_RESET        = 0x28;
    static constexpr uint64_t RESET_CAUSE       = 0x30;
    static constexpr uint64_t POWER_DOMAIN_REQ  = 0x38;
    static constexpr uint64_t POWER_DOMAIN_ACK  = 0x40;
    static constexpr uint64_t POWER_GOOD        = 0x48;
    static constexpr uint64_t POWER_STATUS      = 0x50;
    static constexpr uint64_t SPIN_LOCK         = 0x58;
    static constexpr uint64_t CHIP_MODE         = 0x60;
    static constexpr uint64_t MAILBOX0          = 0x68;
    static constexpr uint64_t MAILBOX1          = 0x70;
    static constexpr uint64_t RING_OSC          = 0x98;
    static constexpr uint64_t CPU_DIVIDER       = 0xA0;
    static constexpr uint64_t SYSTEM_DIVIDER    = 0xA8;
    static constexpr uint64_t PERIPH_DIVIDER    = 0xB0;
    // Must match the highest offset
    static constexpr uint64_t LAST_OFFSET       = 0xB0;

    // Register Bit Masks
    static constexpr uint32_t SYSTEM_CONFIG_SYS_INTR_EN         = 1 << 0;
    static constexpr uint32_t SYSTEM_CONFIG_MRAM_STARTUP_BYPASS = 1 << 1;
    static constexpr uint32_t SYSTEM_CONFIG_WDOG_DISABLE        = 1 << 2;
    static constexpr uint32_t SYSTEM_CONFIG_SPI_ENABLE          = 1 << 4;
    static constexpr uint32_t SYSTEM_CONFIG_UART_ENABLE         = 1 << 6;

    static constexpr uint32_t WATCHDOG_KICK                     = 1 << 7;

    static constexpr uint32_t SPIN_LOCK_LOCK                    = 1 << 0;

    static constexpr uint32_t POWER_DOMAIN_REQ_MRAM_DSLEEP_EN   = 1 << 16;

    static constexpr uint32_t SOFT_RESET_MRAM_RST_B             = 1 << 2;

    // Register Values
    uint32_t version;
    uint32_t system_config;
    uint32_t sys_interrupt;
    uint32_t reset_cause;
    uint32_t power_domain_req;
    uint32_t power_domain_ack;
    uint32_t spin_lock;
    uint32_t chip_mode;
    uint32_t soft_reset;
    uint32_t mailbox0;
    uint32_t mailbox1;
    uint32_t power_good;
    uint32_t ring_osc;
    uint32_t cpu_divider;
    uint32_t system_divider;
    uint32_t periph_divider;

    // MRAM power state: starts off, turns on when mram_dsleep_en is first written 0
    bool mram_powered = false;

    // Watchdog device with 4-cycle divider (250MHz from 1GHz system clock)
    Watchdog<4> watchdog;

    void reset(ResetCause cause = ResetCause::NONE);

    // Static watchdog timeout handler, triggers cold reset
    static void watchdog_timeout_handler(const Agent& agent) {
        agent.chip->cold_reset();
    }

    uint32_t read_register(const Agent& agent, uint64_t offset);
    void write_register(const Agent& agent, uint64_t offset, uint32_t value);

}; 
  
} // namespace bemu

#endif // BEMU_SYSREGS_ER_H
