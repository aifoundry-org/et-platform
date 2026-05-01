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

#include <hwinc/top.h>
namespace bemu {


template <uint64_t Base>
void SysregsEr<Base>::reset(ResetCause cause)
{
    // TODO: use methods to write some of the registers (once implemented),
    // because that can have side effects
    version            = SYSTEM_VERSION_RESET_VALUE;
    system_config      = SYSTEM_SYSTEMCONFIG_RESET_VALUE;
    sys_interrupt      = 0;
    reset_cause        = static_cast<uint32_t>(cause);  // Set reset cause
    power_domain_req   = SYSTEM_POWERDOMAINREQ_RESET_VALUE;
    power_domain_ack   = SYSTEM_POWERDOMAINACK_RESET_VALUE;
    spin_lock          = SYSTEM_SPINLOCK_RESET_VALUE;
    chip_mode          = SYSTEM_CHIPMODE_RESET_VALUE;
    soft_reset         = SYSTEM_SOFTRESET_RESET_VALUE;
    mailbox0           = SYSTEM_MAILBOX0_RESET_VALUE;
    mailbox1           = SYSTEM_MAILBOX1_RESET_VALUE;
    power_good         = SYSTEM_POWERGOOD_RESET_VALUE;
    ring_osc           = SYSTEM_RING_OSC_RESET_VALUE;
    cpu_divider        = SYSTEM_CPU_DIVIDER_RESET_VALUE;
    system_divider     = SYSTEM_SYSTEM_DIVIDER_RESET_VALUE;
    periph_divider     = SYSTEM_PERIPH_DIVIDER_RESET_VALUE;

    // Initialize watchdog with default count and disabled state
    watchdog.set_count_from(SYSTEM_WATCHDOG_COUNT_RESET_VALUE);
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
            return SYSTEM_WATCHDOG_RESET_VALUE;

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
            return SYSTEM_POWERSTATUS_RESET_VALUE;

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
            ring_osc = value & SYSTEM_RING_OSC_WRITE_MASK;
            break;

        case CPU_DIVIDER:
            cpu_divider = value & SYSTEM_CPU_DIVIDER_WRITE_MASK;
            break;

        case SYSTEM_DIVIDER:
            system_divider = value & SYSTEM_SYSTEM_DIVIDER_WRITE_MASK;
            break;

        case PERIPH_DIVIDER:
            periph_divider = value & SYSTEM_PERIPH_DIVIDER_WRITE_MASK;
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


template struct SysregsEr<ERBIUM_TOP_SYSTEM_REGISTERS_BASE>;

} // namespace bemu
