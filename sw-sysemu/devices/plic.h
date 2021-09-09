/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PLIC_H
#define BEMU_PLIC_H

#include <array>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <system_error>
#include <vector>
#include <unistd.h>

#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "system.h"


namespace bemu {

using PLIC_Target_Notify = void (*)(System* system, bool raise);

struct PLIC_Interrupt_Target {
    uint32_t           name_id;
    uint32_t           address_id;
    PLIC_Target_Notify notify;
};

// S = #sources, T = #targets
template <unsigned long long Base, size_t N, size_t S, size_t T>
struct PLIC : public MemoryRegion
{
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    #define PLIC_PRIORITY_MASK   7
    #define PLIC_THRESHOLD_MASK  7

    // PLIC Register offsets
    enum : size_type {
        PLIC_REG_PRIORITY_SOURCE = 0x000000,
        PLIC_REG_PENDING         = 0x001000,
        PLIC_REG_ENABLE          = 0x002000,
        PLIC_REG_THRESHOLD_MAXID = 0x200000
    };

    // MemoryRegion methods
    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos >= PLIC_REG_PRIORITY_SOURCE && pos < PLIC_REG_PENDING) {
            reg_priority_source_read(pos, result32);
        } else if (pos >= PLIC_REG_PENDING && pos < PLIC_REG_ENABLE) {
            reg_pending_read(pos - PLIC_REG_PENDING, result32);
        } else if (pos >= PLIC_REG_ENABLE && pos < PLIC_REG_THRESHOLD_MAXID) {
            reg_enable_read(pos - PLIC_REG_ENABLE, result32);
        } else {
            reg_threshold_maxid_read(agent.chip, pos - PLIC_REG_THRESHOLD_MAXID, result32);
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos >= PLIC_REG_PRIORITY_SOURCE && pos < PLIC_REG_PENDING) {
            reg_priority_source_write(pos, source32);
        } else if (pos >= PLIC_REG_PENDING && pos < PLIC_REG_ENABLE) {
            // Read-only region!
            throw memory_error(first() + pos);
        } else if (pos >= PLIC_REG_ENABLE && pos < PLIC_REG_THRESHOLD_MAXID) {
            reg_enable_write(pos - PLIC_REG_ENABLE, source32);
        } else {
            reg_threshold_maxid_write(agent.chip, pos - PLIC_REG_THRESHOLD_MAXID, source32);
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PLIC::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // PLIC methods

    void interrupt_pending_set(const Agent& agent, uint32_t source_id) {
        if (source_id < S) {
            ip[source_id] = true;
            update_logic(agent.chip);
        }
    }

    void interrupt_pending_clear(const Agent& agent, uint32_t source_id) {
        if (source_id < S) {
            ip[source_id] = false;
            update_logic(agent.chip);
        }
    }

protected:
    // Returns the list of Interrupt Targets
    virtual const std::vector<PLIC_Interrupt_Target> &get_target_list() const = 0;

private:
    // Returns the interrupt target's Name ID given its Address ID
    bool target_address_to_name(uint32_t address_id, uint32_t *name_id) const {
        for (const auto &t : get_target_list()) {
            if (t.address_id == address_id) {
                *name_id = t.name_id;
                return true;
            }
        }
        return true;
    }

    // Run PLIC logic when there is a potential change
    void update_logic(System* system) {
        // For each interrupt target
        for (const auto &t : get_target_list()) {
            uint32_t max_prio = 0;
            uint32_t new_max_id = 0;
            bool trigger = false;
            // For each interrupt source
            for (size_t s = 0; s < S; s++) {
                // In-flight interrupts are ignored
                if (in_flight[s])
                    continue;

                // Interrupt pending and enabled for that target and
                // the source priority is greater than the target threshold
                if (ip[s] && ie[t.name_id][s] && (priority[s] > threshold[t.name_id])) {
                    // Update which source has max priority
                    if (priority[s] > max_prio) {
                        max_prio = priority[s];
                        new_max_id = s;
                        trigger = true;
                    }
                }
            }

            // Update target's MaxID
            max_id[t.name_id] = new_max_id;

            // Raise/clear interrupt
            if (trigger && !eip[t.name_id]) {
                eip[t.name_id] = true;
                // Send interrupt to target
                t.notify(system, true);
            } else if (!trigger && eip[t.name_id]) {
                eip[t.name_id] = false;
                // Clear interrupt to target
                t.notify(system, false);
            }
        }
    }

    // PLIC Read register subregions
    void reg_priority_source_read(size_type pos, uint32_t *result32) const {
        uint32_t index = pos / 4;
        if (index >= S)
            return;

        *result32 = priority[index];
    }

    void reg_pending_read(size_type pos, uint32_t *result32) const {
        uint32_t index = pos / 4;
        if (index >= S)
            return;

        *result32 = bitset_read_u32(ip, 32 * index);
    }

    void reg_enable_read(size_type pos, uint32_t *result32) const {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x80;
        size_type sources = 32 * ((pos % 0x80) / 4);

        if (target_address_to_name(target_addr, &name_id))
            *result32 = bitset_read_u32(ie[name_id], sources);
    }

    void reg_threshold_maxid_read(System* system, size_type pos, uint32_t *result32) {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x1000;

        if (!target_address_to_name(target_addr, &name_id))
            return;

        if ((pos % 0x1000) == 0) { // Threshold registers
            *result32 = threshold[name_id];
        } else if ((pos % 0x1000) == 4) { // MaxID registers
            // Read current MaxID
            *result32 = max_id[name_id];
            // To claim an interrupt, the target reads its MaxID register
            if (max_id[name_id] > 0) {
                in_flight[max_id[name_id]] = true;
                // Save the ID of the Target that claimed the source interrupt
                in_flight_by[max_id[name_id]] = name_id;
                update_logic(system);
            }
        }
    }

    // PLIC Write register subregions
    void reg_priority_source_write(size_type pos, const uint32_t *source32) {
        uint32_t index = pos / 4;
        if (index >= S)
            return;

        priority[index] = *source32 & PLIC_PRIORITY_MASK;
    }

    void reg_enable_write(size_type pos, const uint32_t *source32) {
        uint32_t name_id = 0;
        uint32_t target_addr = pos / 0x80;
        uint32_t enable_offset = pos % 0x80;

        if (target_address_to_name(target_addr, &name_id))
            bitset_write_u32(ie[name_id], 32 * (enable_offset / 4), *source32);
    }

    void reg_threshold_maxid_write(System* system, size_type pos, const uint32_t *source32) {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x1000;

        if (!target_address_to_name(target_addr, &name_id))
            return;

        if ((pos % 0x1000) == 0) { // Threshold registers
            threshold[name_id] = *source32 & PLIC_THRESHOLD_MASK;
        } else if ((pos % 0x1000) == 4) { // MaxID registers
            // Complete an interrupt: target writes to MaxID the ID of the interrupt
            if (in_flight[*source32] && (in_flight_by[*source32] == name_id)) {
                in_flight[*source32] = false;
                update_logic(system);
            }
        }
    }

    // Helpers
    template<size_t Bitset_size>
    static uint32_t bitset_read_u32(const std::bitset<Bitset_size> &set, size_t pos) {
        uint32_t val = 0;
        for (int i = 0; i < 32; i++)
            val |= uint32_t(set[pos + i]) << i;
        return val;
    }

    template<size_t Bitset_size>
    static void bitset_write_u32(std::bitset<Bitset_size> &set, size_t pos, uint32_t val) {
        for (int i = 0; i < 32; i++)
            set[pos + i] = (val >> i) & 1;
    }

    std::bitset<S>                ip;        // Interrupt Pending (per source)
    std::bitset<S>                in_flight; // Interrupt inFlight (per source)
    std::array<uint32_t, S>       in_flight_by; // ID of the target that claimed the source interrupt (per source)
    std::array<uint32_t, S>       priority;  // Interrupt Priority (per source)
    std::array<std::bitset<S>, T> ie;        // Interrupt Enable (per target x source)
    std::bitset<T>                eip;      // External Interrupt Pending (per target)
    std::array<uint32_t, T>       threshold; // Interrupt Threshold (per target)
    std::array<uint32_t, T>       max_id;    // Interrupt MaxID (per target)
};

template <unsigned long long Base, size_t N>
struct PU_PLIC : public PLIC<Base, N, 41, 12>
{
    static void Target_Minion_Machine_external_interrupt(System* system, bool raise) {
        for (int i = 0; i < EMU_NUM_MINION_SHIRES; i++) {
            if (raise) {
                system->raise_machine_external_interrupt(i);
            } else {
                system->clear_machine_external_interrupt(i);
            }
        }
    }

    static void Target_Minion_Supervisor_external_interrupt(System* system, bool raise) {
        for (int i = 0; i < EMU_NUM_MINION_SHIRES; i++) {
            if (raise) {
                system->raise_supervisor_external_interrupt(i);
            } else {
                system->clear_supervisor_external_interrupt(i);
            }
        }
    }

    const std::vector<PLIC_Interrupt_Target> &get_target_list() const {
        static const std::vector<PLIC_Interrupt_Target> targets = {
            {10, 0x21, Target_Minion_Machine_external_interrupt},
            {11, 0x20, Target_Minion_Supervisor_external_interrupt},
        };
        return targets;
    }
};

template <unsigned long long Base, size_t N>
struct SP_PLIC : public PLIC<Base, N, 148, 2>
{
    static void Target_SP_Machine_external_interrupt(System* system, bool raise) {
        if (raise) {
            system->raise_machine_external_interrupt(IO_SHIRE_ID);
        } else {
            system->clear_machine_external_interrupt(IO_SHIRE_ID);
        }
    }

    static void Target_SP_Supervisor_external_interrupt(System* system, bool raise) {
        if (raise) {
            system->raise_supervisor_external_interrupt(IO_SHIRE_ID);
        } else {
            system->clear_supervisor_external_interrupt(IO_SHIRE_ID);
        }
    }

    const std::vector<PLIC_Interrupt_Target> &get_target_list() const {
        static const std::vector<PLIC_Interrupt_Target> targets = {
            {0, 0, Target_SP_Machine_external_interrupt},
            {1, 1, Target_SP_Supervisor_external_interrupt},
        };
        return targets;
    }
};

} // namespace bemu

#endif // BEMU_PLIC_H
