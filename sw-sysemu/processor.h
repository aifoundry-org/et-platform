/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_PROCESSOR_H
#define BEMU_PROCESSOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>

#include "state.h"

//namespace bemu {


// A logical processor (Hart)

struct Processor {

    // Register files
    uint64_t    xregs[NXREGS];
    freg_t      fregs[NFREGS];
    mreg_t      mregs[NMREGS];
    freg_t      tenc[NFREGS];   // TODO: this is per core not per hart

    // RISCV control and status registers
    uint32_t    fcsr;
    uint64_t    stvec;
    uint32_t    scounteren;
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
    uint64_t    mbusaddr;
    uint8_t     mcache_control;   // 2b -- TODO: this is per core not per hart
    uint64_t    tensor_conv_size; // can we remove?
    uint64_t    tensor_conv_ctrl; // can we remove?
    // TODO: tensor_coop
    uint16_t    tensor_mask;
    uint16_t    tensor_error;
    uint16_t    ucache_control;   // TODO: this is per core not per hart
    uint8_t     gsc_progress;     // log2(MLEN) bits
    uint64_t    validation0;
    uint8_t     validation1;
    uint64_t    validation2;
    uint64_t    validation3;
    std::array<uint64_t,4> portctrl;

    // Other hart internal (microarchitectural or hidden) state
    prv_t       prv;

    // Pre-computed state to improve simulation speed
    bool break_on_load;
    bool break_on_store;
    bool break_on_fetch;
    bool enabled;

    enum class Tensor {
        None,
        Reduce,
        Quant,
        FMA
    };

    // Tensor accelerator state

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


//} // namespace bemu

#endif // BEMU_PROCESSOR_H
