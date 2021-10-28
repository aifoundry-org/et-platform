/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PROCESSOR_H
#define BEMU_PROCESSOR_H

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sstream>

#include "support/intrusive/list.h"
#include "agent.h"
#include "cache.h"
#include "emu_defines.h"
#include "insn.h"
#include "mmu.h"
#include "state.h"
#include "tensor.h"
#include "traps.h"

namespace bemu {


//
// Privilege levels
//
enum class Privilege {
    U = 0,
    S = 1,
    M = 3
};


//
// Trivial object thrown as exception for canceling and (maybe at a later
// time) restarting the execution of an instruction
//
struct instruction_restart { };


//
// Forward declaration
//
struct Hart;


//==------------------------------------------------------------------------==//
//
// Tensor coprocessor
//
//==------------------------------------------------------------------------==//

struct TLoad {
    enum class State : uint8_t {
        idle,           // nothing to do
        ready,          // ready to execute
        loading,        // tenb loaded but waiting for txfma to pair
        waiting_coop,   // waiting other co-operating harts
    };

    // FIXME: We also save all necessary bits for accessing the page table and
    // PMA (mprv, mpp, prv, sum, mxr) but we don't save the satp register.

    uint64_t        value;  // CSR write value
    uint64_t        stride; // x31 value (stride + wait-id)
    uint32_t        tcoop;  // local tensor_coop value
    std::bitset<16> tmask;  // local tensor_mask value
    bool            paired; // only valid for loads to tenb
    State           state;  // FSM state

    uint64_t uuid = 0;
};


struct TMul {
    enum class State : uint8_t {
        idle,           // nothing to do
        ready,          // ready to execute
        waiting_tenb,   // waiting for coop tensor load to tenb
    };

    uint64_t        value;  // CSR write value
    std::bitset<16> tmask;  // Local tensor_mask value
    uint8_t         frm;    // Rounding mode
    State           state;  // FSM state

    uint64_t uuid = 0;
};


struct TQuant {
    enum class State : uint8_t {
        idle,           // nothing to do
        ready,          // ready to execute
    };

    uint64_t  value;  // CSR write value
    uint8_t   frm;    // Rounding mode
    State     state;  // FSM state

    uint64_t uuid = 0;
};


struct TReduce {
    enum class State : uint8_t {
        idle,               // nothing to do
        ready_to_send,      // sender ready
        ready_to_receive,   // receiver ready
        waiting_to_send,    // sender waiting for receiver
        waiting_to_receive, // receiver waiting for sender
    };

    Hart*     hart;   // the target or source hart
    uint8_t   freg;   // current register to send/receive
    uint8_t   count;  // number of remaining registers to send/receive
    uint8_t   funct;  // receiver operation to perform
    uint8_t   frm;    // Rounding mode
    State     state;  // FSM state

    uint64_t uuid = 0;
};


struct TStore {
    enum class State : uint8_t {
        idle,      // nothing to do
        ready,     // ready to execute
    };

    uint64_t value;  // CSR write value
    uint64_t stride; // x31 stride value
    State    state;  // FSM state

    uint64_t uuid = 0;
};


// The tfma, tquant, reduce, and tstore instructions share the same
// issue port. There can be one pending instruction per FSM, but ready
// instructions must execute in order.  So, we use an issue queue for holding
// all the in-flight tensor arithmetic instructions.
//
struct TQueue {
    using size_type = size_t;

    enum class Instruction : uint8_t {
        none,
        tfma,
        tquant,
        reduce,
        tstore,
    };

    bool empty() const noexcept
    { return m_elems.front() == Instruction::none; }

    bool full() const noexcept
    { return m_elems.back() != Instruction::none; }

    Instruction front() const noexcept
    { return m_elems.front(); }

    constexpr size_type max_size() const noexcept
    { return m_elems.size(); }

    size_type size() const noexcept
    {
        for (auto i = max_size(); i > 0; --i) {
            if (m_elems[i - 1] != Instruction::none) return i;
        }
        return 0;
    }

    void push(Instruction value) noexcept
    {
        assert(!full());
        m_elems[size()] = value;
    }

    void pop() noexcept
    {
        assert(!empty());
        m_elems[0] = Instruction::none;
        std::rotate(m_elems.begin(), m_elems.begin() + 1, m_elems.end());
    }

    void clear() noexcept
    {
        std::fill(m_elems.begin(), m_elems.end(), Instruction::none);
    }

private:
    std::array<Instruction, 4>  m_elems;
};


//==------------------------------------------------------------------------==//
//
// A processing core
//
//==------------------------------------------------------------------------==//

struct Core {
    // Only one TenC in the core
    std::array<freg_t,NFREGS>   tenc;

    // L1 scratchpad
    std::array<cache_line_t,L1_SCP_ENTRIES+TFMA_MAX_AROWS>  scp;

    // L1 D-cache lock bits and addresses of locked lines
    std::array<std::array<bool,L1D_NUM_WAYS>,L1D_NUM_SETS>      scp_lock;
    std::array<std::array<uint64_t,L1D_NUM_WAYS>,L1D_NUM_SETS>  scp_addr;

    // CSRs shared between threads of a core
    uint64_t    satp;
    uint64_t    matp;
    uint8_t     menable_shadows;  // 2b
    uint8_t     excl_mode;        // 1b
    uint8_t     mcache_control;   // 2b
    uint16_t    ucache_control;

    // Unique ID to identify tensor ops in the log
    uint64_t    tensor_uuid = 0;

    // Tensor arithmetic state machines
    TMul        tmul;
    TQuant      tquant;
    TReduce     reduce;
    TStore      tstore;

    // Tensor execution ports
    std::array<TLoad, 2>  tload_a;
    TLoad                 tload_b;
    TQueue                tqueue;
};


//==------------------------------------------------------------------------==//
//
// A hardware thread
//
//==------------------------------------------------------------------------==//

struct Hart : public Agent {
    // ----- Types -----

    // Message port configuration
    struct Port {
        bool            enabled;
        bool            enable_oob;
        bool            umode;
        uint8_t         logsize;
        uint8_t         max_msgs;
        uint8_t         scp_set;
        uint8_t         scp_way;
        bool            stall;
        uint8_t         rd_ptr;
        uint8_t         wr_ptr;
        uint8_t         size;
        std::bitset<16> oob_data;
    };

    // Current thread state
    enum class State {
        nonexistent,        // Non-simulating
        unavailable,        // Currently/temporarily unavailable
        active,             // Has work to do
        sleeping,           // Waiting
    };

    // Stall reasons
    enum class Waiting : uint32_t {
        none        = 0,
        // tensor_wait reasons
        tload_0     = 1 << 0,
        tload_1     = 1 << 1,
        tload_L2_0  = 1 << 2,
        tload_L2_1  = 1 << 3,
        prefetch_0  = 1 << 4,
        prefetch_1  = 1 << 5,
        cacheop     = 1 << 6,
        tfma        = 1 << 7,
        tstore      = 1 << 8,
        reduce      = 1 << 9,
        tquant      = 1 << 10,
        // 11-15 are reserved for tensor_wait
        interrupt   = 1 << 16,
        message     = 1 << 17,
        credit0     = 1 << 18,
        credit1     = 1 << 19,
        // resource conflicts
        tload_tenb  = 1 << 20,
    };

    // ----- Public methods -----

    long shireid() const;
    std::string name() const override;

    void advance_pc();
    void activate_breakpoints();
    void set_prv(Privilege value);
    void set_tdata1(uint64_t value);
    void check_pending_interrupts() const;
    void fetch();
    void async_execute();
    void execute();
    void take_trap(const Trap&);
    void raise_interrupt(int cause, uint64_t data = 0);
    void clear_interrupt(int cause);
    void notify_pmu_minion_event(uint8_t event);

    uint8_t frm() const;

    bool is_blocked() const;

    bool is_nonexistent() const;
    bool is_unavailable() const;
    bool is_halted() const;
    bool is_running() const;
    bool is_active() const;
    bool is_sleeping() const;
    bool is_waiting() const;
    bool is_waiting(Waiting what) const;
    bool has_active_coprocessor() const;

    void become_nonexistent();
    void become_unavailable();
    void start_running();
    void enter_debug_mode();
    void start_waiting(Waiting what);
    void stop_waiting(Waiting what);
    void maybe_sleep();
    void maybe_wakeup();

    // ----- Public state -----

    // Hart state (disabled, running, etc.)
    State       state = State::unavailable;
    Waiting     waits = Waiting::none;
    Waiting     twait = Waiting::none;

    // Next and previous hart in list of waiting/running harts
    intrusive::List_hook  links;

    // Core that this hart belongs to
    Core*       core = nullptr;

    // Program counter
    uint64_t    pc;
    uint64_t    npc;

    // Instruction being executed
    Instruction inst;

    // Fetch buffer
    uint64_t              fetch_pc;
    std::array<char, 32>  fetch_cache;

    // Register files
    std::array<uint64_t,NXREGS>   xregs;
    std::array<freg_t,NFREGS>     fregs;
    std::array<mreg_t,NMREGS>     mregs;

    // RISCV control and status registers
    uint32_t    fcsr;
    uint64_t    stvec;
    uint16_t    scounteren;             // 9b
    uint64_t    sscratch;
    uint64_t    sepc;
    uint64_t    scause;
    uint64_t    stval;
    uint64_t    mstatus;
    uint32_t    medeleg;
    uint32_t    mideleg;
    uint32_t    mie;
    uint64_t    mtvec;
    uint16_t    mcounteren;             // 9b
    uint64_t    mscratch;
    uint64_t    mepc;
    uint64_t    mcause;
    uint64_t    mtval;
    uint32_t    mip;
    uint64_t    tdata1;
    uint64_t    tdata2;
    // TODO: dcsr, dpc, dscratch
    uint16_t    mhartid;

    // Esperanto control and status registers
    uint64_t    minstmask;        // 33b
    uint32_t    minstmatch;
    uint64_t    mbusaddr;         // 40b
    uint64_t    tensor_conv_size; // can we remove?
    uint64_t    tensor_conv_ctrl; // can we remove?
    uint32_t    tensor_coop;
    std::bitset<16> tensor_mask;
    uint16_t    tensor_error;
    uint8_t     gsc_progress;     // log2(MLEN) bits
    uint64_t    validation0;
    uint8_t     validation1;
    uint64_t    validation2;
    uint64_t    validation3;
    std::array<Port,4>     portctrl;
    std::array<uint16_t,2> fcc;

    // Supervisor external interrupt pin (as 32-bit for performance)
    uint32_t    ext_seip;

    // Other hart internal (microarchitectural or hidden) state
    Privilege   prv;
    bool        debug_mode;

    // Pre-computed state to improve simulation speed
    bool break_on_load;
    bool break_on_store;
    bool break_on_fetch;

    // validation1 CSR emulation needs this
    std::ostringstream uart_stream;
};


inline Hart::Waiting operator~(Hart::Waiting rhs)
{
    using T = typename std::underlying_type<Hart::Waiting>::type;
    return static_cast<Hart::Waiting>(~static_cast<T>(rhs));
}


inline Hart::Waiting operator&(Hart::Waiting lhs, Hart::Waiting rhs)
{
    using T = typename std::underlying_type<Hart::Waiting>::type;
    return static_cast<Hart::Waiting>(static_cast<T>(lhs) & static_cast<T>(rhs));
}


inline Hart::Waiting operator|(Hart::Waiting lhs, Hart::Waiting rhs)
{
    using T = typename std::underlying_type<Hart::Waiting>::type;
    return static_cast<Hart::Waiting>(static_cast<T>(lhs) | static_cast<T>(rhs));
}


inline Hart::Waiting operator^(Hart::Waiting lhs, Hart::Waiting rhs)
{
    using T = typename std::underlying_type<Hart::Waiting>::type;
    return static_cast<Hart::Waiting>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
}


inline Hart::Waiting& operator&=(Hart::Waiting& lhs, Hart::Waiting rhs)
{
    return lhs = lhs & rhs;
}


inline Hart::Waiting& operator|=(Hart::Waiting& lhs, Hart::Waiting rhs)
{
    return lhs = lhs | rhs;
}


inline Hart::Waiting& operator^=(Hart::Waiting& lhs, Hart::Waiting rhs)
{
    return lhs = lhs | rhs;
}


inline long Hart::shireid() const
{
    return mhartid / EMU_THREADS_PER_SHIRE;
}


inline std::string Hart::name() const
{
    return std::string("H")
         + std::to_string(mhartid)
         + std::string(" S")
         + std::to_string(mhartid / EMU_THREADS_PER_SHIRE)
         + std::string(":N")
         + std::to_string((mhartid / EMU_THREADS_PER_NEIGH) % EMU_NEIGH_PER_SHIRE)
         + std::string(":C")
         + std::to_string((mhartid / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_NEIGH)
         + std::string(":T")
         + std::to_string(mhartid % EMU_THREADS_PER_MINION);
}


inline void Hart::advance_pc()
{
    pc = npc;
}


inline void Hart::activate_breakpoints()
{
    uint64_t mcontrol = tdata1;
    int priv = static_cast<int>(prv);
    break_on_load  = !(~mcontrol & ((8 << priv) | 1));
    break_on_store = !(~mcontrol & ((8 << priv) | 2));
    break_on_fetch = !(~mcontrol & ((8 << priv) | 4));
}


inline void Hart::set_prv(Privilege value)
{
    prv = value;
    activate_breakpoints();
}


inline void Hart::set_tdata1(uint64_t value)
{
    tdata1 = value;
    activate_breakpoints();
}


inline void Hart::check_pending_interrupts() const
{
    // Are there any non-masked pending interrupts? If excl_mode != 0 this
    // thread is either in exclusive mode or blocked, but either way it cannot
    // receive interrupts
    uint_fast32_t xip = (mip | ext_seip) & mie;

    if (!xip || core->excl_mode) {
        return;
    }

    // If there are any pending interrupts for the current privilege level
    // 'x', they are only taken if mstatus.xIE=1. If there are any pending
    // interrupts for a higher privilege level 'y>x' they must be taken
    // independently of the value in mstatus.yIE. Pending interrupts for a
    // lower privilege level 'w<x' are not taken.
    uint_fast32_t mip = xip & ~mideleg;
    uint_fast32_t sip = xip & mideleg;
    uint_fast32_t mie = mstatus & 8;
    uint_fast32_t sie = mstatus & 2;
    switch (prv) {
    case Privilege::M:
        if (!mip || !mie) {
            return;
        }
        xip = mip;
        break;
    case Privilege::S:
        if (!mip && !sie) {
            return;
        }
        xip = mip | (sie ? sip : 0);
        break;
    case Privilege::U:
        /* nothing */
        break;
    }

    if (xip & (1 << MACHINE_EXTERNAL_INTERRUPT)) {
        throw trap_machine_external_interrupt();
    }
    if (xip & (1 << MACHINE_SOFTWARE_INTERRUPT)) {
        throw trap_machine_software_interrupt();
    }
    if (xip & (1 << MACHINE_TIMER_INTERRUPT)) {
        throw trap_machine_timer_interrupt();
    }
    if (xip & (1 << SUPERVISOR_EXTERNAL_INTERRUPT)) {
        throw trap_supervisor_external_interrupt();
    }
    if (xip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)) {
        throw trap_supervisor_software_interrupt();
    }
    if (xip & (1 << SUPERVISOR_TIMER_INTERRUPT)) {
        throw trap_supervisor_timer_interrupt();
    }
    if (xip & (1 << BAD_IPI_REDIRECT_INTERRUPT)) {
        throw trap_bad_ipi_redirect_interrupt();
    }
    if (xip & (1 << ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT)) {
        throw trap_icache_ecc_counter_overflow_interrupt();
    }
    if (xip & (1 << BUS_ERROR_INTERRUPT)) {
        throw trap_bus_error_interrupt();
    }
}


inline void Hart::fetch()
{
    inst.bits = mmu_fetch(*this, pc);
    inst.flags = 0;
}


inline void Hart::async_execute()
{
    if (mhartid % EMU_THREADS_PER_MINION != 0) {
        return;
    }
    if (core->tload_a[0].state == TLoad::State::ready) {
        tensor_load_execute(*this, 0, false);
    }
    if (core->tload_a[1].state == TLoad::State::ready) {
        tensor_load_execute(*this, 1, false);
    }
    if (core->tload_b.state == TLoad::State::ready) {
        tensor_load_execute(*this, 0, true);
    }
    switch (core->tqueue.front()) {
    case TQueue::Instruction::none:
        break;
    case TQueue::Instruction::tfma:
        if (core->tmul.state == TMul::State::ready) {
            tensor_fma_execute(*this);
        }
        break;
    case TQueue::Instruction::tquant:
        if (core->tquant.state == TQuant::State::ready) {
            tensor_quant_execute(*this);
        }
        break;
    case TQueue::Instruction::reduce:
        if (core->reduce.state == TReduce::State::ready_to_receive) {
            auto& send = core->reduce.hart->core->reduce;
            if (send.state == TReduce::State::ready_to_send) {
                if (send.hart == this) {
                    tensor_reduce_execute(*this);
                }
            }
        }
        break;
    case TQueue::Instruction::tstore:
        if (core->tstore.state == TStore::State::ready) {
            tensor_store_execute(*this);
        }
        break;
    }
}


inline uint8_t Hart::frm() const
{
    return (fcsr >> 5) & 7;
}


inline bool Hart::is_blocked() const
{
    return core->excl_mode == 1 + ((~mhartid & 1) << 1);
}


inline bool Hart::is_nonexistent() const
{
    return state == State::nonexistent;
}


inline bool Hart::is_unavailable() const
{
    return state == State::unavailable;
}


inline bool Hart::is_halted() const
{
    return (state >= State::active) && debug_mode;
}


inline bool Hart::is_running() const
{
    return (state >= State::active) && !debug_mode;
}


inline bool Hart::is_active() const
{
    return state == State::active;
}


inline bool Hart::is_sleeping() const
{
    return state == State::sleeping;
}


inline bool Hart::is_waiting() const
{
    return waits != Waiting::none;
}


inline bool Hart::is_waiting(Waiting what) const
{
    return (waits & what) != Waiting::none;
}


inline bool Hart::has_active_coprocessor() const
{
    return ((mhartid % EMU_THREADS_PER_MINION) == 0)
        && ((core->tload_a[0].state != TLoad::State::idle)
            || (core->tload_a[1].state != TLoad::State::idle)
            || (core->tload_b.state != TLoad::State::idle)
            || (core->tmul.state != TMul::State::idle)
            || (core->tquant.state != TQuant::State::idle)
            || (core->reduce.state != TReduce::State::idle)
            || (core->tstore.state != TStore::State::idle));
}


inline unsigned hart_index(const Hart& cpu)
{
    return (cpu.mhartid == IO_SHIRE_SP_HARTID)
        ? EMU_IO_SHIRE_SP_THREAD
        : cpu.mhartid;
}


inline unsigned core_index(const Hart& cpu)
{
    return hart_index(cpu) / EMU_THREADS_PER_MINION;
}


inline unsigned neigh_index(const Hart& cpu)
{
    return hart_index(cpu) / EMU_THREADS_PER_NEIGH;
}


inline unsigned shire_index(const Hart& cpu)
{
    return hart_index(cpu) / EMU_THREADS_PER_SHIRE;
}


} // namespace bemu

#endif // BEMU_PROCESSOR_H
