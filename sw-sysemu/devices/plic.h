/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_PLIC_H
#define BEMU_PLIC_H

#include <array>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "memory/memory_error.h"
#include "memory/memory_region.h"


namespace bemu {

struct PLIC_Source {
    virtual ~PLIC_Source() {}
    virtual void interrupt_completed(void) const = 0;
};

using PLIC_Target_Notify = void (*)(void);

struct PLIC_Interrupt_Target {
    uint32_t           name_id;
    uint32_t           address_id;
    PLIC_Target_Notify notify;
};

// S = #sources, T = #targets
template <unsigned long long Base, size_t N, size_t S, size_t T>
struct PLIC : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

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
    void read(size_type pos, size_type n, pointer result) override {
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
            reg_threshold_maxid_read(pos - PLIC_REG_THRESHOLD_MAXID, result32);
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
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
            reg_threshold_maxid_write(pos - PLIC_REG_THRESHOLD_MAXID, source32);
        }
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PLIC::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // PLIC methods

    void interrupt_pending_set(size_type source_id) {
        assert(source_id < S);
        ip[source_id] = true;
        update_logic();
    }

    void interrupt_pending_clear(size_type source_id) {
        assert(source_id < S);
        ip[source_id] = false;
        update_logic();
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
    void update_logic() {
        // For each interrupt target
        for (const auto &t : get_target_list()) {
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
                    if (priority[s] > new_max_id) {
                        new_max_id = priority[s];
                        trigger = true;
                    }
                }
            }

            // Update target's MaxID
            max_id[t.name_id] = new_max_id;
            if (trigger /* && !eip[t.name_id] */) {
                // Send interrupt to target
                t.notify();
            }
        }
    }

    // PLIC Read register subregions
    void reg_priority_source_read(size_type pos, uint32_t *result32) const {
        *result32 = priority[pos / 4];
    }

    void reg_pending_read(size_type pos, uint32_t *result32) const {
        size_type idx = 32 * (pos / 4);
        *result32 = bitset_read_u32(ip, idx);
    }

    void reg_enable_read(size_type pos, uint32_t *result32) const {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x80;
        size_type sources = 32 * ((pos % 0x80) / 4);

        if (target_address_to_name(target_addr, &name_id))
            *result32 = bitset_read_u32(ie[name_id], sources);
        else
            *result32 = 0;
    }

    void reg_threshold_maxid_read(size_type pos, uint32_t *result32) {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x1000;

        if (target_address_to_name(target_addr, &name_id)) {
            if ((pos % 1000) == 0) { // Threshold registers
                *result32 = threshold[name_id];
            } else if ((pos % 1000) == 4) { // MaxID registers
                // To claim an interrupt, the target reads its MaxID register
                if (max_id[name_id] > 0) {
                    in_flight[max_id[name_id]] = true;
                    *result32 = max_id[name_id];
                    update_logic();
                }
            }
        } else {
            *result32 = 0;
        }
    }

    // PLIC Write register subregions
    void reg_priority_source_write(size_type pos, const uint32_t *source32) {
        priority[pos / 4] = *source32 & PLIC_PRIORITY_MASK;
    }

    void reg_enable_write(size_type pos, const uint32_t *source32) {
        uint32_t name_id = 0;
        uint32_t target_addr = pos / 0x80;
        uint32_t enable_offset = pos % 0x80;

        if (target_address_to_name(target_addr, &name_id))
            bitset_write_u32(ie[name_id], 32 * (enable_offset / 4), *source32);
    }

    void reg_threshold_maxid_write(size_type pos, const uint32_t *source32) {
        uint32_t name_id = 0;
        size_type target_addr = pos / 0x1000;

        if (target_address_to_name(target_addr, &name_id)) {
            if ((pos % 1000) == 0) { // Threshold registers
                threshold[name_id] = *source32 & PLIC_THRESHOLD_MASK;
            } else if ((pos % 1000) == 4) { // MaxID registers
                // Complete an interrupt: target writes to MaxID the ID of the interrupt
                if (max_id[name_id] == *source32) {
                    in_flight[max_id[name_id]] = false;
                    update_logic();
                }
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
    std::array<uint32_t, S>       priority;  // Interrupt Priority (per source)
    std::array<std::bitset<S>, T> ie;        // Interrupt Enable (per target x source)
    //std::bitset<T>               eip;      // External Interrupt Pending (per target). Not needed for now
    std::array<uint32_t, T>       threshold; // Interrupt Threshold (per target)
    std::array<uint32_t, T>       max_id;    // Interrupt MaxID (per target)
};

template <unsigned long long Base, size_t N>
struct PU_PLIC : public PLIC<Base, N, 41, 12>
{
    static void Target_M_mode_Notify(void) {
#ifdef SYS_EMU
        for (int i = 0; i < EMU_NUM_MINION_SHIRES; i++)
            sys_emu::raise_external_interrupt(i);
#endif
    }

    static void Target_S_mode_Notify(void) {
#ifdef SYS_EMU
        for (int i = 0; i < EMU_NUM_MINION_SHIRES; i++)
            sys_emu::raise_external_supervisor_interrupt(i);
#endif
    }

    const std::vector<PLIC_Interrupt_Target> &get_target_list() const {
        static const std::vector<PLIC_Interrupt_Target> targets = {
            {10, 0x21, Target_M_mode_Notify},
            {11, 0x20, Target_S_mode_Notify},
        };
        return targets;
    }
};

} // namespace bemu

#endif // BEMU_PLIC_H
