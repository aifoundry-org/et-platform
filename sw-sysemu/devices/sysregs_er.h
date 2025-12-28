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
    addr_type last() const override { return Base + LAST_OFFSET; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    void wdt_clock_tick(const Agent& agent, uint64_t cycle);

private:

    // Register Offsets
    static constexpr uint64_t VERSION           = 0x00;
    static constexpr uint64_t WATCHDOG_COUNT    = 0x08;
    static constexpr uint64_t SYSTEM_CONFIG     = 0x10;
    static constexpr uint64_t WATCHDOG          = 0x18;
    static constexpr uint64_t SYS_INTERRUPT     = 0x20;
    static constexpr uint64_t RESET_CAUSE       = 0x28;
    static constexpr uint64_t POWER_DOMAIN_REQ  = 0x30;
    static constexpr uint64_t POWER_DOMAIN_ACK  = 0x38;
    static constexpr uint64_t SPIN_LOCK         = 0x40;
    static constexpr uint64_t CHIP_MODE         = 0x48;
    static constexpr uint64_t SOFT_RESET        = 0x50;
    static constexpr uint64_t MAILBOX0          = 0x58;
    static constexpr uint64_t MAILBOX1          = 0x60;
    static constexpr uint64_t POWER_GOOD        = 0x68;
    // Must match the highest offset
    static constexpr uint64_t LAST_OFFSET       = 0x68;

    // Register Bit Masks
    static constexpr uint32_t SYSTEM_CONFIG_WDOG_DISABLE        = 1 << 8;
    static constexpr uint32_t SYSTEM_CONFIG_MRAM_STARTUP_BYPASS = 1 << 6;
    static constexpr uint32_t SYSTEM_CONFIG_SYS_INTR_EN         = 1 << 3;

    static constexpr uint32_t WATCHDOG_KICK                     = 1 << 7;

    static constexpr uint32_t SPIN_LOCK_LOCK                    = 1 << 0;

    static constexpr uint32_t POWER_DOMAIN_REQ_MRAM_DSLEEP_EN   = 1 << 16;

    static constexpr uint32_t SOFT_RESET_MRAM_RST_B             = 1 << 2;

    // Register Values
    uint32_t version;           // 0x00: [31:16]r=chipid(0xEB68), [15:8]r=variation, [7:0]r=respin
    // uint32_t watchdog_count; // 0x08: Now managed by watchdog device
    uint32_t system_config;     // 0x10: See SYSTEM_CONFIG_* constants above
    // uint32_t watchdog;       // 0x18: [7]rw=kick (no storage needed)
    uint32_t sys_interrupt;     // 0x20: [0]rw=interrupt
    uint32_t reset_cause;       // 0x28:
                                //       [5]=hresetn(rclr), [4]=softreset(rclr)
                                //       [3]=brownout(rclr), [2]=sysreset_req(rclr),
                                //       [1]=watchdog_timedout(rclr), [0]=por(1, rclr)
    uint32_t power_domain_req;  // 0x30: [17]rw=cpu_sleep_en, [16]rw=mram_sleep_en(1),
                                //       [6]r=hyperbus, [5]w=system_poweroff,
                                //       [4]rw=mram_pd, [3]r=chiplet_pd,
                                //       [2]rw=cpu_ram_powerdown,
                                //       [1]r=sram_pd, [0]rw=cpu_pd
    uint32_t power_domain_ack;  // 0x38: [6]r=hyperbus_pd_ack, [5]r=system_pd_ack,
                                //       [4]r=mram_pd_ack, [3]r=chiplet_pd_ack,
                                //       [1]r=sram_pd_ack, [0]r=cpu_pd_ack
    uint32_t spin_lock;         // 0x40: [0]=lock(rw,rset)
    uint32_t chip_mode;         // 0x48: [6:5]=load_external, [4:3]=bootload,
                                //       [2]=ifc_width, [1:0]=chip_mode
    uint32_t soft_reset;        // 0x50: [2]rw=mram_rst_b(1),
                                //       [1]rw=cpu_hreset, [0]rw=soft_reset
    uint32_t mailbox0;          // 0x58: [31:0]=mbox0
    uint32_t mailbox1;          // 0x60: [31:0]=mbox1
    uint32_t power_good;        // 0x68: [20:0]=counter(0xFFFFF)

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
