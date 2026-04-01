/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cinttypes>
#include <cassert>
#include "devices/sysregs_er.h"
#include "system.h"
#include "memory/memory_error.h"
#include "emu_gio.h"

#define SYSREGS_ER_REGION_BASE 0x0002000000ULL

namespace bemu {


template <uint64_t Base>
void SysregsEr<Base>::reset(ResetCause cause)
{
    // TODO: use methods to write some of the registers (once implemented),
    // because that can have side effects
    version            = 0xEB680000;  // chipid=0xEB68, variation=0, respin=0
    system_config      = SYSTEM_CONFIG_WDOG_DISABLE | SYSTEM_CONFIG_SPI_ENABLE;
    sys_interrupt      = 0;
    reset_cause        = static_cast<uint32_t>(cause);  // Set reset cause
    power_domain_req   = 0;
    mram_powered       = false;
    power_domain_ack   = 0;
    spin_lock          = 0;
    chip_mode          = 0;
    soft_reset         = SOFT_RESET_MRAM_RST_B;
    mailbox0           = 0;
    mailbox1           = 0;
    power_good         = 0xFFFFF;
    ring_osc           = 0x7F; // en=1, divby2_sel=1, trm=0x1F
    cpu_divider        = 0x1F; // count=0xF, div_enable=1
    system_divider     = 0x1F; // count=0xF, div_enable=1
    periph_divider     = 0x1F; // count=0xF, div_enable=1

    // Initialize watchdog with default count and disabled state
    watchdog.set_count_from(0xFFFF);
    watchdog.kick();
    watchdog.set_enabled((system_config & SYSTEM_CONFIG_WDOG_DISABLE) == 0);
    // Set timeout handler (system pointer will be extracted from agent at runtime)
    watchdog.set_timeout_handler(watchdog_timeout_handler);
}


template <uint64_t Base>
uint32_t SysregsEr<Base>::read_register(const Agent& agent, uint64_t offset)
{
    uint64_t addr = Base + offset;

    switch (offset) {
        case VERSION:
            return version;

        case WATCHDOG_COUNT:
            return watchdog.get_count_from();

        case SYSTEM_CONFIG: {
            // Update the WDOG_DISABLE bit based on actual watchdog state
            uint32_t config = system_config;
            if (watchdog.is_enabled()) {
                config &= ~SYSTEM_CONFIG_WDOG_DISABLE;  // Clear disable bit (enabled)
            } else {
                config |= SYSTEM_CONFIG_WDOG_DISABLE;   // Set disable bit (disabled)
            }
            return config;
        }

        case WATCHDOG:
            // Watchdog register always reads as 0
            return 0;

        case RESET_CAUSE: {
            // Read-clear operation: return current value, then clear all bits
            // All bits [5:0] are marked as rclr (read-clear)
            uint32_t current_value = reset_cause;
            reset_cause = 0;
            return current_value;
        }

        case POWER_DOMAIN_REQ:
            return power_domain_req;

        case POWER_DOMAIN_ACK:
            return power_domain_ack;

        case POWER_STATUS:
            // Simplified: no power sequencing modeled, MRAM is always ready.
            return 0;

        case SPIN_LOCK: {
            // Read-set atomic operation: return current value, then set lock bit
            uint32_t current_value = spin_lock;
            spin_lock |= SPIN_LOCK_LOCK;
            return current_value;
        }

        case CHIP_MODE:
            return chip_mode;

        case SOFT_RESET:
            return soft_reset;

        case MAILBOX0:
            return mailbox0;

        case MAILBOX1:
            return mailbox1;

        case POWER_GOOD:
            return power_good;

        case RING_OSC:
            return ring_osc;

        case CPU_DIVIDER:
            return cpu_divider;

        case SYSTEM_DIVIDER:
            return system_divider;

        case PERIPH_DIVIDER:
            return periph_divider;

        default:
            WARN_AGENT(erbium_regs, agent, "Read unknown Erbium register 0x%" PRIx64, addr);
            throw memory_error(addr);
    }
}


template <uint64_t Base>
void SysregsEr<Base>::write_register(const Agent& agent, uint64_t offset, uint32_t value)
{
    uint64_t addr = Base + offset;

    switch (offset) {
        case WATCHDOG_COUNT:
            watchdog.set_count_from(value);
            break;

        case SYSTEM_CONFIG: {
            system_config = value;
            const bool wdog_enabled = (value & SYSTEM_CONFIG_WDOG_DISABLE) == 0;
            watchdog.set_enabled(wdog_enabled);
            break;
        }

        case WATCHDOG:
            if (value & WATCHDOG_KICK) {
                watchdog.kick();
            }
            break;

        case SOFT_RESET:
            soft_reset = value;
            break;

        case POWER_DOMAIN_REQ:
            power_domain_req = value;
            mram_powered = !(value & POWER_DOMAIN_REQ_MRAM_DSLEEP_EN);
            break;

        case POWER_STATUS:
            // Simplified: writes accepted but ignored (no power sequencing).
            break;

        case MAILBOX0:
            mailbox0 = value;
            break;

        case MAILBOX1:
            mailbox1 = value;
            break;

        case SPIN_LOCK:
            // Only write bit 0 (lock bit), ignore all other bits
            spin_lock = (spin_lock & ~SPIN_LOCK_LOCK) | (value & SPIN_LOCK_LOCK);
            break;

        case RING_OSC:
            ring_osc = value & 0x7FF; // bits [10:0]
            break;

        case CPU_DIVIDER:
            cpu_divider = value & 0x1F; // bits [4:0]
            break;

        case SYSTEM_DIVIDER:
            system_divider = value & 0x1F; // bits [4:0]
            break;

        case PERIPH_DIVIDER:
            periph_divider = value & 0x1F; // bits [4:0]
            break;

        default:
            WARN_AGENT(erbium_regs, agent, "Write unknown Erbium register 0x%" PRIx64 " = 0x%" PRIx32, addr, value);
            throw memory_error(addr);
    }
}


template <uint64_t Base>
void SysregsEr<Base>::read(const Agent& agent, size_type pos, size_type count, pointer result)
{
    assert(count == 4 || count == 8);

    uint32_t reg_value = read_register(agent, pos);

    if (count == 4) {
        *reinterpret_cast<uint32_t*>(result) = reg_value;
    } else {
        *reinterpret_cast<uint64_t*>(result) = reg_value;
    }
}


template <uint64_t Base>
void SysregsEr<Base>::write(const Agent& agent, size_type pos, size_type count, const_pointer source)
{
    // Erbium Sysregs are all 32bit registers with 64bit alignment.
    // We allow both 32 and 64 bit access, but high bits (32-63) are
    // never used (ignored on write and zeroed on read).
    assert(count == 4 || count == 8);

    uint32_t value;
    if (count == 4) {
        value = *reinterpret_cast<const uint32_t*>(source);
    } else {
        // For 64-bit writes, take the lower 32 bits
        value = static_cast<uint32_t>(*reinterpret_cast<const uint64_t*>(source));
    }

    write_register(agent, pos, value);
}


template <uint64_t Base>
void SysregsEr<Base>::wdt_clock_tick(const Agent& agent, uint64_t cycle)
{
    watchdog.clock_tick(agent, cycle);
}


template struct SysregsEr<SYSREGS_ER_REGION_BASE>;

} // namespace bemu
