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

#include <hwinc/system.h>
namespace bemu {

// TODO: move to reset
// Reset cause reasons
enum class ResetCause {
    NONE            = 0x0,
    POR             = SYSTEM_RESETCAUSE_POR_FIELD_MASK,
    WATCHDOG        = SYSTEM_RESETCAUSE_WATCHDOG_TIMEDOUT_FIELD_MASK,
    SYSRESET        = SYSTEM_RESETCAUSE_SYSRESET_REQ_FIELD_MASK,
    BROWNOUT        = SYSTEM_RESETCAUSE_BROWNOUT_FIELD_MASK,
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

private:

    static constexpr uint64_t VERSION          = SYSTEM_VERSION_OFFSET;
    static constexpr uint64_t SYSTEM_CONFIG    = SYSTEM_SYSTEMCONFIG_OFFSET;
    static constexpr uint64_t WATCHDOG_COUNT   = SYSTEM_WATCHDOG_COUNT_OFFSET;
    static constexpr uint64_t WATCHDOG         = SYSTEM_WATCHDOG_OFFSET;
    static constexpr uint64_t SYS_INTERRUPT    = SYSTEM_SYSINTERRUPT_OFFSET;
    static constexpr uint64_t SOFT_RESET       = SYSTEM_SOFTRESET_OFFSET;
    static constexpr uint64_t RESET_CAUSE      = SYSTEM_RESETCAUSE_OFFSET;
    static constexpr uint64_t POWER_DOMAIN_REQ = SYSTEM_POWERDOMAINREQ_OFFSET;
    static constexpr uint64_t POWER_DOMAIN_ACK = SYSTEM_POWERDOMAINACK_OFFSET;
    static constexpr uint64_t POWER_GOOD       = SYSTEM_POWERGOOD_OFFSET;
    static constexpr uint64_t POWER_STATUS     = SYSTEM_POWERSTATUS_OFFSET;
    static constexpr uint64_t SPIN_LOCK        = SYSTEM_SPINLOCK_OFFSET;
    static constexpr uint64_t CHIP_MODE        = SYSTEM_CHIPMODE_OFFSET;
    static constexpr uint64_t MAILBOX0         = SYSTEM_MAILBOX0_OFFSET;
    static constexpr uint64_t MAILBOX1         = SYSTEM_MAILBOX1_OFFSET;
    static constexpr uint64_t RING_OSC         = SYSTEM_RING_OSC_OFFSET;
    static constexpr uint64_t CPU_DIVIDER      = SYSTEM_CPU_DIVIDER_OFFSET;
    static constexpr uint64_t SYSTEM_DIVIDER   = SYSTEM_SYSTEM_DIVIDER_OFFSET;
    static constexpr uint64_t PERIPH_DIVIDER   = SYSTEM_PERIPH_DIVIDER_OFFSET;
    static constexpr uint64_t LAST_OFFSET      = SYSTEM_PERIPH_DIVIDER_OFFSET;

    static constexpr uint32_t SYSTEM_CONFIG_SYS_INTR_EN         = SYSTEM_SYSTEMCONFIG_SYS_INTERRUPT_ENABLE_FIELD_MASK;
    static constexpr uint32_t SYSTEM_CONFIG_MRAM_STARTUP_BYPASS = SYSTEM_SYSTEMCONFIG_MRAM_STARTUP_BYPASS_FIELD_MASK;
    static constexpr uint32_t SYSTEM_CONFIG_WDOG_DISABLE        = SYSTEM_SYSTEMCONFIG_WDOG_DISABLE_FIELD_MASK;
    static constexpr uint32_t SYSTEM_CONFIG_SPI_ENABLE          = SYSTEM_SYSTEMCONFIG_SPI_ENABLE_FIELD_MASK;
    static constexpr uint32_t SYSTEM_CONFIG_UART_ENABLE         = SYSTEM_SYSTEMCONFIG_UART_ENABLE_FIELD_MASK;

    static constexpr uint32_t WATCHDOG_KICK         = SYSTEM_WATCHDOG_KICK_FIELD_MASK;
    static constexpr uint32_t SPIN_LOCK_LOCK        = SYSTEM_SPINLOCK_LOCK_FIELD_MASK;
    static constexpr uint32_t SOFT_RESET_MRAM_RST_B = SYSTEM_SOFTRESET_MRAM_RST_B_FIELD_MASK;

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
