/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SYSTEM_H
#define BEMU_SYSTEM_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <bitset>

#include "support/intrusive/list.h"
#include "memory/main_memory.h"
#include "emu_defines.h"
#include "esrs.h"
#include "processor.h"
#include "testLog.h"

class sys_emu;

namespace bemu {


//
// A bitmask with 1-bit per Minion of a shire
//
using Coop_minion_mask  = std::bitset<EMU_MINIONS_PER_SHIRE>;


//
// An entry in the cooperative tensor load table. `pending` is only valid
// for the leader. When `all` is empty the entry is unused.
//
struct Coop_tload_state {
    Coop_minion_mask  all;      // Bitmask of all harts cooperating
    Coop_minion_mask  pending;  // Bitmask of pending cooperating harts
};


//
// Cooperative tensor load tracking logic; indexed by [tenb][coopid]
//
using Coop_tload_table  = std::array<std::array<Coop_tload_state, 32>, 2>;


//==------------------------------------------------------------------------==//
//
// An ET-SoC-1 system
//
//==------------------------------------------------------------------------==//

class System {
public:
    // ----- Types -----
    using neigh_pmu_counters_t  = std::array<std::array<uint64_t, EMU_THREADS_PER_MINION>, 6>;
    using neigh_pmu_events_t    = std::array<std::array<uint8_t, EMU_THREADS_PER_NEIGH>, 6>;

    using msg_func_t = std::function<void(unsigned)>;
    using hart_mask_t = std::bitset<EMU_NUM_THREADS>;

    enum class Stepping {
        unknown,
        A0,
    };

    struct msg_port_write_t {
        uint32_t source_thread;
        uint32_t target_thread;
        uint32_t target_port;
        bool     is_remote;
        bool     is_tbox;
        bool     is_rbox;
        uint8_t  oob;
        uint32_t data[(1 << PORT_LOG2_MAX_SIZE)/4];
    };

    // ----- Public system methods -----

    // Configure the emulation environment
    void init(Stepping);

    // Preload memory
    void load_elf(const char* filename);
    void load_raw(const char* filename, unsigned long long addr);

    // Reset state
    void reset_shire_state(unsigned shireid);
    void reset_hart(unsigned thread);

    uint64_t get_csr(unsigned thread, uint16_t cnum);
    void set_csr(unsigned thread, uint16_t cnum, uint64_t data);

    // Simulation control
    bool get_emu_done() const;
    void set_emu_done(bool value);

    bool has_active_harts() const;
    bool has_sleeping_harts() const;
    bool has_available_harts() const;

    // Interrupts
    void pu_plic_interrupt_pending_set(uint32_t source_id);
    void pu_plic_interrupt_pending_clear(uint32_t source_id);
    void sp_plic_interrupt_pending_set(uint32_t source_id);
    void sp_plic_interrupt_pending_clear(uint32_t source_id);
    void raise_machine_timer_interrupt(unsigned shire);
    void clear_machine_timer_interrupt(unsigned shire);
    void raise_machine_external_interrupt(unsigned shire);
    void clear_machine_external_interrupt(unsigned shire);
    void raise_supervisor_external_interrupt(unsigned shire);
    void clear_supervisor_external_interrupt(unsigned shire);
    void raise_machine_software_interrupt(unsigned shire, uint64_t thread_mask);
    void clear_machine_software_interrupt(unsigned shire, uint64_t thread_mask);
    void send_ipi_redirect(unsigned shire, uint64_t thread_mask);

    // Device/Host interface
    bool raise_host_interrupt(uint32_t bitmap);
    void copy_memory_from_host_to_device(uint64_t from_addr, uint64_t to_addr, uint32_t size);
    void copy_memory_from_device_to_host(uint64_t from_addr, uint64_t to_addr, uint32_t size);
    void notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value);

    // UARTs
    void pu_uart0_set_rx_fd(int fd);
    void pu_uart1_set_rx_fd(int fd);
    int pu_uart0_get_rx_fd() const;
    int pu_uart1_get_rx_fd() const;
    void spio_uart0_set_rx_fd(int fd);
    void spio_uart1_set_rx_fd(int fd);
    int spio_uart0_get_rx_fd() const;
    int spio_uart1_get_rx_fd() const;
    void pu_uart0_set_tx_fd(int fd);
    void pu_uart1_set_tx_fd(int fd);
    int pu_uart0_get_tx_fd() const;
    int pu_uart1_get_tx_fd() const;
    void spio_uart0_set_tx_fd(int fd);
    void spio_uart1_set_tx_fd(int fd);
    int spio_uart0_get_tx_fd() const;
    int spio_uart1_get_tx_fd() const;

    // Peripherals/devices
    void tick_peripherals(uint64_t cycle);

    // Timers
    bool pu_rvtimer_is_active() const;
    bool spio_rvtimer_is_active() const;

    // System registers
    uint64_t esr_read(const Agent& agent, uint64_t addr);
    void esr_write(const Agent& agent, uint64_t addr, uint64_t value);

    void write_shire_coop_mode(unsigned shire, uint64_t value);
    void write_thread0_disable(unsigned shire, uint32_t value);
    void write_thread1_disable(unsigned shire, uint32_t value);
    void write_minion_feature(unsigned shire, uint8_t value);

    void write_icache_prefetch(Privilege privilege, unsigned shire, uint64_t val);
    uint64_t read_icache_prefetch(Privilege privilege, unsigned shire) const;
    void finish_icache_prefetch(unsigned shire);

    // Message ports
    void set_msg_funcs(msg_func_t fn);
    void set_delayed_msg_port_write(bool);
    void write_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread, uint32_t* data);
    void commit_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread);

    void set_emu(sys_emu* emu) noexcept;
    sys_emu* emu() const noexcept;

    uint64_t emu_cycle() const noexcept;

    // ----- Public system state -----

    // Configuration
    Stepping stepping = Stepping::unknown;

    // Harts and cores
    std::array<Hart, EMU_NUM_THREADS>  cpu {};
    std::array<Core, EMU_NUM_MINIONS>  core {};

    // `active` holds all harts in the running state that arehave actions to
    // peform. This includes harts that can execute RISC-V instruction (i.e.,
    // non-waiting, and non-blocked), or harts that are executing coprocessor
    // instructions (i.e., the associated coprocessor is not idle, even though
    // the hart may be blocked or waiting).
    //
    // `sleeping` holds all harts in the running state that have no actions to
    // perform. This includes harts that cannot execute RISC-V instruction
    // (i.e., waiting or blocked) and either do not have an associated
    // coprocessor or the coprocessor is idle.
    //
    // `awaking` holds all harts previously in the `waiting` list that must be
    // moved to the `running` list.
    intrusive::List<Hart, &Hart::links> active;
    intrusive::List<Hart, &Hart::links> awaking;
    intrusive::List<Hart, &Hart::links> sleeping;

    // Main memory
    MainMemory                                memory {};
    typename MemoryRegion::reset_value_type   memory_reset_value {};

    // Performance monitoring counters
    std::array<neigh_pmu_counters_t, EMU_NUM_NEIGHS>  neigh_pmu_counters {};
    std::array<neigh_pmu_events_t, EMU_NUM_NEIGHS>    neigh_pmu_events {};

    // Cooperative tensor load tracking
    std::array<Coop_tload_table, EMU_NUM_NEIGHS>    coop_tloads {};

    // System registers
    std::array<neigh_esrs_t, EMU_NUM_NEIGHS>        neigh_esrs {};
    std::array<shire_cache_esrs_t, EMU_NUM_SHIRES>  shire_cache_esrs {};
    std::array<shire_other_esrs_t, EMU_NUM_SHIRES>  shire_other_esrs {};
    std::array<broadcast_esrs_t, EMU_NUM_SHIRES>    broadcast_esrs {};

    // Logging
    testLog     log {"EMU", LOG_INFO}; // Consider making this a "plugin"
    hart_mask_t log_thread;

    // System agent
    Noagent   noagent {this, "SYSTEM"};

private:
    // ----- Private system methods -----

    // System registers
    void write_fcc_credinc(unsigned index, uint64_t shire, uint64_t minion_mask);
    void recalculate_thread0_enable(unsigned shire);
    void recalculate_thread1_enable(unsigned shire);

    // Message ports
    void write_msg_port_data_to_scp(Hart& cpu, unsigned id, uint32_t *data, uint8_t oob);

    // ----- Private system state -----

    // Simulation control
    bool m_emu_done {false};

    // Message ports
    bool msg_port_delayed_write {false};
    std::array<std::vector<msg_port_write_t>, EMU_NUM_SHIRES> msg_port_pending_writes {};
    msg_func_t msg_to_thread = nullptr;

    sys_emu* m_emu = nullptr;
};


inline bool System::get_emu_done() const
{
    return m_emu_done;
}


inline void System::set_emu_done(bool value)
{
    m_emu_done = value;
}


inline bool System::has_active_harts() const
{
    return !active.empty() || !awaking.empty();
}


inline bool System::has_sleeping_harts() const
{
    return !sleeping.empty();
}


inline bool System::has_available_harts() const
{
    return has_active_harts() || has_sleeping_harts();
}


inline void System::pu_plic_interrupt_pending_set(uint32_t source_id)
{
    memory.pu_plic_interrupt_pending_set(noagent, source_id);
}


inline void System::pu_plic_interrupt_pending_clear(uint32_t source_id)
{
    memory.pu_plic_interrupt_pending_clear(noagent, source_id);
}


inline void System::sp_plic_interrupt_pending_set(uint32_t source_id)
{
    memory.sp_plic_interrupt_pending_set(noagent, source_id);
}


inline void System::sp_plic_interrupt_pending_clear(uint32_t source_id)
{
    memory.sp_plic_interrupt_pending_clear(noagent, source_id);
}

inline void System::pu_uart0_set_rx_fd(int fd)
{
    memory.pu_uart0_set_rx_fd(fd);
}


inline void System::pu_uart1_set_rx_fd(int fd)
{
    memory.pu_uart1_set_rx_fd(fd);
}


inline int System::pu_uart0_get_rx_fd() const
{
    return memory.pu_uart0_get_rx_fd();
}


inline int System::pu_uart1_get_rx_fd() const
{
    return memory.pu_uart1_get_rx_fd();
}


inline void System::spio_uart0_set_rx_fd(int fd)
{
    memory.spio_uart0_set_rx_fd(fd);
}


inline void System::spio_uart1_set_rx_fd(int fd)
{
    memory.spio_uart1_set_rx_fd(fd);
}


inline int System::spio_uart0_get_rx_fd() const
{
    return memory.spio_uart0_get_rx_fd();
}


inline int System::spio_uart1_get_rx_fd() const
{
    return memory.spio_uart1_get_rx_fd();
}


inline void System::pu_uart0_set_tx_fd(int fd)
{
    memory.pu_uart0_set_tx_fd(fd);
}


inline void System::pu_uart1_set_tx_fd(int fd)
{
    memory.pu_uart1_set_tx_fd(fd);
}


inline int System::pu_uart0_get_tx_fd() const
{
    return memory.pu_uart0_get_tx_fd();
}


inline int System::pu_uart1_get_tx_fd() const
{
    return memory.pu_uart1_get_tx_fd();
}


inline void System::spio_uart0_set_tx_fd(int fd)
{
    memory.spio_uart0_set_tx_fd(fd);
}


inline void System::spio_uart1_set_tx_fd(int fd)
{
    memory.spio_uart1_set_tx_fd(fd);
}


inline int System::spio_uart0_get_tx_fd() const
{
    return memory.spio_uart0_get_tx_fd();
}


inline int System::spio_uart1_get_tx_fd() const
{
    return memory.spio_uart1_get_tx_fd();
}


inline bool System::pu_rvtimer_is_active() const
{
    return memory.pu_rvtimer_is_active();
}


inline bool System::spio_rvtimer_is_active() const
{
    return memory.spio_rvtimer_is_active();
}


inline void System::tick_peripherals(uint64_t cycle)
{
    // cycle at 1GHz, timer clock at 10MHz
    if ((cycle % 100) == 0) {
        memory.pu_rvtimer_clock_tick(noagent);
        memory.spio_rvtimer_clock_tick(noagent);
        memory.pu_apb_timers_clock_tick(*this);
        memory.spio_apb_timers_clock_tick(*this);
    }
}


inline void System::set_delayed_msg_port_write(bool what)
{
    msg_port_delayed_write = what;
}


inline void System::set_emu(sys_emu* emu) noexcept
{
    m_emu = emu;
}


inline sys_emu* System::emu() const noexcept
{
    assert(m_emu && "sys_emu not linked with system");
    return m_emu;
}


} // namespace bemu

#endif // BEMU_SYSTEM_H
