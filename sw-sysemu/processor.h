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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>

#include "state.h"
#include "traps.h"

namespace bemu {


// A logical processor (Hart)

struct Processor {

    // Register files
    std::array<uint64_t,NXREGS>   xregs;
    std::array<freg_t,NFREGS>     fregs;
    std::array<mreg_t,NMREGS>     mregs;
    std::array<freg_t,NFREGS>     tenc; // TODO: this is per core not per hart

    // Program counter
    uint64_t    pc;
    uint64_t    npc;

    // RISCV control and status registers
    uint32_t    fcsr;
    uint64_t    stvec;
    uint16_t    scounteren;             // 9b
    uint64_t    sscratch;
    uint64_t    sepc;
    uint64_t    scause;
    uint64_t    stval;
    uint64_t    satp;           // TODO: this is per core not per hart
    uint64_t    mstatus;
    uint32_t    medeleg;
    uint32_t    mideleg;
    uint32_t    mie;
    uint64_t    mtvec;
    uint16_t    mcounteren;             // 9b
    std::array<uint8_t,6>  mhpmevent;   // 5b
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
    uint64_t    matp;             // TODO: this is per core not per hart
    uint64_t    minstmask;        // 33b
    uint32_t    minstmatch;
    uint8_t     menable_shadows;  // 2b -- TODO: this is per core not per hart
    uint8_t     excl_mode;        // 1b -- TODO: this is per core not per hart
    uint64_t    mbusaddr;         // 40b
    uint8_t     mcache_control;   // 2b -- TODO: this is per core not per hart
    uint64_t    tensor_conv_size; // can we remove?
    uint64_t    tensor_conv_ctrl; // can we remove?
    uint32_t    tensor_coop;
    uint16_t    tensor_mask;
    uint16_t    tensor_error;
    uint16_t    ucache_control;   // TODO: this is per core not per hart
    uint8_t     gsc_progress;     // log2(MLEN) bits
    uint64_t    validation0;
    uint8_t     validation1;
    uint64_t    validation2;
    uint64_t    validation3;
    std::array<uint32_t,4> portctrl;
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

    // Tensor accelerator state

    enum class Tensor {
        None,
        Reduce,
        Quant,
        FMA
    };

    // Tensor reduction operation state machine
    // TODO: this is per core not per hart
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
    } reduce;

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
    } wait;

    // Tensor quantization operation state machine
    // TODO: this is per core not per hart
    uint64_t txquant;

    // NB: Due to pipelining a TensorQuant can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    uint64_t shadow_txquant;

    // Tensor FMA operation state machine
    // TODO: this is per core not per hart
    uint64_t txfma;

    // NB: Due to pipelining a TensorFMA can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    uint64_t shadow_txfma;

    // TensorLoad state machines
    std::array<uint64_t,2> txload;
    std::array<uint64_t,2> txstride;

    // NB: Due to pipelining a TensorLoad can start a few cycles before the
    // last one finishes, but this case manifests only in ZSIM.
    std::array<uint64_t,2> shadow_txload;
    std::array<uint64_t,2> shadow_txstride;

    // Active tensor operation state machines
    // TODO: this is per core not per hart
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


inline void advance_pc(Processor& cpu)
{
    cpu.pc = cpu.npc;
}


inline void activate_breakpoints(Processor& cpu)
{
    uint64_t mcontrol = cpu.tdata1;
    int priv = int(cpu.prv);
    cpu.break_on_load  = !(~mcontrol & ((8 << priv) | 1));
    cpu.break_on_store = !(~mcontrol & ((8 << priv) | 2));
    cpu.break_on_fetch = !(~mcontrol & ((8 << priv) | 4));
}


inline void set_prv(Processor& cpu, prv_t value)
{
    cpu.prv = value;
    activate_breakpoints(cpu);
}


inline void set_tdata1(Processor& cpu, uint64_t value)
{
    cpu.tdata1 = value;
    activate_breakpoints(cpu);
}


inline void check_pending_interrupts(const Processor& cpu)
{
    // Are there any non-masked pending interrupts? If excl_mode != 0 this
    // thread is either in exclusive mode or blocked, but either way it cannot
    // receive interrupts
    uint_fast32_t xip = (cpu.mip | cpu.ext_seip) & cpu.mie;

    if (!xip || cpu.excl_mode)
        return;

    // If there are any pending interrupts for the current privilege level
    // 'x', they are only taken if mstatus.xIE=1. If there are any pending
    // interrupts for a higher privilege level 'y>x' they must be taken
    // independently of the value in mstatus.yIE. Pending interrupts for a
    // lower privilege level 'w<x' are not taken.
    uint_fast32_t mip = xip & ~cpu.mideleg;
    uint_fast32_t sip = xip & cpu.mideleg;
    uint_fast32_t mie = cpu.mstatus & 8;
    uint_fast32_t sie = cpu.mstatus & 2;
    switch (cpu.prv) {
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


} // namespace bemu

#endif // BEMU_PROCESSOR_H
