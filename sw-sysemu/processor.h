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

#include "agent.h"
#include "cache.h"
#include "emu_defines.h"
#include "insn.h"
#include "mmu.h"
#include "state.h"
#include "traps.h"

namespace bemu {


//
// A processing core
//
struct Core {
    // Tensor operations
    enum class Tensor {
        None,
        Reduce,
        Quant,
        FMA
    };

    // Tensor reduction operation state machine
    struct Reduce {
        uint16_t thread;    // partner hart
        uint8_t  regid;     // next register to send/receive
        uint8_t  count;     // number of remaining registers to operate on
        uint8_t  optype;    // arithmetic operation and reduction type
        enum class State : uint8_t {
            Idle = 0,
            Send = 1,
            Recv = 2,
            Skip = 3
        } state;
    };

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

    // Tensor reduction operation state machine
    Reduce reduce;

    // Tensor quantization operation state machine
    uint64_t txquant;

    // NB: Due to pipelining a TensorQuant can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    uint64_t shadow_txquant;

    // Tensor FMA operation state machine
    uint64_t txfma;

    // NB: Due to pipelining a TensorFMA can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    uint64_t shadow_txfma;

    // Active tensor operation state machines
    std::array<Tensor,3> tensor_op;

    // Keep track of TensorLoadSetupB pairing
    bool tensorload_setupb_topair;
    int  tensorload_setupb_numlines;

    bool enqueue_tensor_op(Tensor kind) {
        for (unsigned i = 0; i < 3; ++i) {
            if (tensor_op[i] == Tensor::None) {
                tensor_op[i] = kind;
                return true;
            }
            // Cannot have the same FSM twice in the list
            assert(tensor_op[i] != kind);
        }
        return false;
    }

    Tensor dequeue_tensor_op() {
        Tensor op = tensor_op[0];
        tensor_op[0] = tensor_op[1];
        tensor_op[1] = tensor_op[2];
        tensor_op[2] = Tensor::None;
        return op;
    }

    constexpr Tensor active_tensor_op() const {
        return tensor_op[0];
    }
};


//
// A hardware thread
//
struct Hart : public Agent {
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

    // Tensor wait operation state machine
    struct Wait {
        uint8_t  id;    // ID of the wait
        uint64_t value; // Value used to do the tensor wait
        enum class State : uint8_t {
            Idle = 0,
            Wait = 1,
            WaitReady = 2,
            TxFMA = 3
        } state;
    };

    long shireid() const;
    std::string name() const override;

    void advance_pc();
    void activate_breakpoints();
    void set_prv(prv_t value);
    void set_tdata1(uint64_t value);
    void check_pending_interrupts() const;
    void fetch();
    void execute();
    void take_trap(const trap_t&);
    void notify_pmu_minion_event(uint8_t event);

    // Message ports
    bool get_msg_port_stall(unsigned port) const;

    // Core that this hart belongs to
    Core*  core;

    // Program counter
    uint64_t    pc;
    uint64_t    npc;

    // Instruction being executed
    insn_t      inst;

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

    uint8_t frm() const { return (fcsr >> 5) & 7; }

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
    prv_t       prv;
    bool        debug_mode;
    bool        fcc_wait;
    uint8_t     fcc_cnt; // FIXME: Why do we need this?

    // Pre-computed state to improve simulation speed
    bool break_on_load;
    bool break_on_store;
    bool break_on_fetch;
    bool enabled;
    bool mtvec_is_set;  // for debugging of benchmarks
    bool stvec_is_set;  // for debugging of benchmarks

    // Tensor wait operation state machine
    Wait wait;

    // TensorLoad state machines
    std::array<uint64_t,2> txload;
    std::array<uint64_t,2> txstride;

    // NB: Due to pipelining a TensorLoad can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    std::array<uint64_t,2> shadow_txload;
    std::array<uint64_t,2> shadow_txstride;

    // validation1 CSR emulation needs this
    std::ostringstream uart_stream;
};


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
    int priv = int(prv);
    break_on_load  = !(~mcontrol & ((8 << priv) | 1));
    break_on_store = !(~mcontrol & ((8 << priv) | 2));
    break_on_fetch = !(~mcontrol & ((8 << priv) | 4));
}


inline void Hart::set_prv(prv_t value)
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

    if (!xip || core->excl_mode)
        return;

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
        case PRV_M:
            if (!mip || !mie)
                return;
            xip = mip;
            break;
        case PRV_S:
            if (!mip && !sie)
                return;
            xip = mip | (sie ? sip : 0);
            break;
        default:
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


inline bool Hart::get_msg_port_stall(unsigned id) const
{
    return portctrl[id].stall;
}


inline uint64_t hart_index(const Hart& cpu)
{
    return (cpu.mhartid == IO_SHIRE_SP_HARTID)
        ? EMU_IO_SHIRE_SP_THREAD
        : cpu.mhartid;
}


inline uint64_t core_index(const Hart& cpu)
{
    return hart_index(cpu) / EMU_THREADS_PER_MINION;
}


inline uint64_t neigh_index(const Hart& cpu)
{
    return hart_index(cpu) / EMU_THREADS_PER_NEIGH;
}


inline long shire_index(const Hart& cpu)
{
    return shire_index(cpu.shireid());
}


} // namespace bemu

#endif // BEMU_PROCESSOR_H
