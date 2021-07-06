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
#include <queue>

#include "emu_defines.h"
#include "esrs.h"
#include "processor.h"
#include "memory/main_memory.h"
#include "testLog.h"

#ifdef SYS_EMU
class sys_emu;
#endif

namespace bemu {


//
// An ET-SoC system
//
class System {
public:
    // ----- Types -----
    typedef std::array<std::array<uint64_t, EMU_THREADS_PER_MINION>, 6> neigh_pmu_counters_t;
    typedef std::array<std::array<uint8_t, EMU_THREADS_PER_NEIGH>, 6> neigh_pmu_events_t;

    using msg_func_t = std::function<void(unsigned)>;
    using hart_mask_t = std::bitset<EMU_NUM_THREADS>;

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
    void init(system_version_t);

    // Preload memory
    void load_elf(const char* filename);
    void load_raw(const char* filename, unsigned long long addr);

    // Reset state
    void reset_esrs_for_shire(unsigned shireid);
    void reset_hart(unsigned thread);

    // Hart state manipulation
    bool thread_is_blocked(unsigned thread) const;
    uint64_t get_csr(unsigned thread, uint16_t cnum);
    void set_csr(unsigned thread, uint16_t cnum, uint64_t data);

    // Simulation control
    bool emu_done() const;
    void set_emu_done(bool value);
    void emu_set_done();

    // Interrupts
    void pu_plic_interrupt_pending_set(uint32_t source_id);
    void pu_plic_interrupt_pending_clear(uint32_t source_id);
    void sp_plic_interrupt_pending_set(uint32_t source_id);
    void sp_plic_interrupt_pending_clear(uint32_t source_id);
    void raise_timer_interrupt(uint64_t shire_mask);
    void clear_timer_interrupt(uint64_t shire_mask);
    void raise_external_interrupt(unsigned shire);
    void clear_external_interrupt(unsigned shire);
    void raise_external_supervisor_interrupt(unsigned shire);
    void clear_external_supervisor_interrupt(unsigned shire);
    void raise_software_interrupt(unsigned shire, uint64_t thread_mask);
    void clear_software_interrupt(unsigned shire, uint64_t thread_mask);
    void send_ipi_redirect_to_threads(unsigned shire, uint64_t thread_mask);

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

    void write_icache_prefetch(int privilege, unsigned shire, uint64_t val);
    uint64_t read_icache_prefetch(int privilege, unsigned shire) const;
    void finish_icache_prefetch(unsigned shire);

    // Message ports
    void set_msg_funcs(msg_func_t fn);
    void set_delayed_msg_port_write(bool);
    void write_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread, uint32_t* data);
    void commit_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread);

    // Checker interface
    std::queue<uint32_t>& get_minions_to_awake();

#ifdef SYS_EMU
    void set_emu(sys_emu* emu) { m_emu = emu; }
    sys_emu* emu() const noexcept {
        assert(m_emu && "sys_emu not linked with system");
        return m_emu;
    }
#endif

    uint64_t emu_cycle() const noexcept;

    // ----- Public system state -----

    // Configuration
    system_version_t sysver {system_version_t::UNKNOWN};

    // Harts and cores
    std::array<Hart, EMU_NUM_THREADS>  cpu {};
    std::array<Core, EMU_NUM_MINIONS>  core {};

    // Main memory
    MainMemory memory {};
    typename MemoryRegion::reset_value_type memory_reset_value {};

    // Performance monitoring counters
    std::array<neigh_pmu_counters_t, EMU_NUM_NEIGHS> neigh_pmu_counters {};
    std::array<neigh_pmu_events_t, EMU_NUM_NEIGHS> neigh_pmu_events {};

    // System registers
    std::array<neigh_esrs_t, EMU_NUM_NEIGHS>       neigh_esrs {};
    std::array<shire_cache_esrs_t, EMU_NUM_SHIRES> shire_cache_esrs {};
    std::array<shire_other_esrs_t, EMU_NUM_SHIRES> shire_other_esrs {};
    std::array<broadcast_esrs_t, EMU_NUM_SHIRES>   broadcast_esrs {};

    // Logging
    testLog     log{"EMU", LOG_INFO}; // Consider making this a "plugin"
    hart_mask_t log_thread;

    Noagent noagent{this, "SYSTEM"};

private:
    // ----- Private system methods -----

    // System registers
    void write_fcc_credinc(int index, uint64_t shire, uint64_t minion_mask);
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

    // Only for the checker: list of minions to awake when an FCC is written
    std::queue<uint32_t> m_minions_to_awake;

#ifdef SYS_EMU
    sys_emu* m_emu = nullptr;
#endif
};


inline bool System::emu_done() const
{
    return m_emu_done;
}


inline void System::set_emu_done(bool value)
{
    m_emu_done = value;
}


inline void System::emu_set_done()
{
    m_emu_done = true;
}


inline bool System::thread_is_blocked(unsigned thread) const
{
    unsigned other_excl = 1 + ((~thread & 1) << 1);
    return cpu[thread].core->excl_mode == other_excl;
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

inline std::queue<uint32_t>& System::get_minions_to_awake()
{
    return m_minions_to_awake;
}


} // namespace bemu

#endif // BEMU_SYSTEM_H
