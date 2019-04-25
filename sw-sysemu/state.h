/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_STATE_H
#define BEMU_STATE_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>

#include "fpu/fpu_types.h"

//namespace bemu {


// -----------------------------------------------------------------------------
// A packed type can be accessed as unsigned or signed, bytes, halfwords,
// words, double-words, and as half, single, and double precision
// floating-point values.

template<size_t Nbits>
union Packed {
    std::array<uint8_t,Nbits/8>    u8;
    std::array<uint16_t,Nbits/16>  u16;
    std::array<uint32_t,Nbits/32>  u32;
    std::array<uint64_t,Nbits/64>  u64;
    std::array<int8_t,Nbits/8>     i8;
    std::array<int16_t,Nbits/16>   i16;
    std::array<int32_t,Nbits/32>   i32;
    std::array<int64_t,Nbits/64>   i64;
    std::array<float10_t,Nbits/16> f10;
    std::array<float11_t,Nbits/16> f11;
    std::array<float16_t,Nbits/16> f16;
    std::array<float32_t,Nbits/32> f32;
};


// -----------------------------------------------------------------------------
// Register types

enum : size_t {
    XLEN = 64,
    FLEN = 32,
    VLEN = 256,
    MLEN = (VLEN/32),

    NXREGS = 32,
    NFREGS = 32,
    NMREGS = 8
};


typedef Packed<VLEN>        freg_t;
typedef std::bitset<MLEN>   mreg_t;


static_assert(VLEN == 256, "Only 256-bit vectors supported");


// -----------------------------------------------------------------------------
// Hart state

struct state_t {

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
    uint64_t    misa;           // could be hardcoded
    uint32_t    medeleg;
    uint32_t    mideleg;
    uint32_t    mie;
    uint64_t    mtvec;
    uint32_t    mcounteren;
    uint64_t    mscratch;
    uint64_t    mepc;
    uint64_t    mcause;
    uint64_t    mtval;
    uint32_t    mip;
    uint64_t    tdata1;
    uint64_t    tdata2;
    // TODO: dcsr, dpc, dscratch
    uint32_t    mvendorid;      // could be hardcoded
    uint64_t    marchid;        // could be hardcoded
    uint64_t    mimpid;         // could be hardcoded
    uint16_t    mhartid;

    // Esperanto control and status registers
    uint64_t minstmask;           // 33b
    uint32_t minstmatch;
    uint8_t  msleep_txfma_27;     // 1b
    uint8_t  menable_shadows;     // 2b
    // TODO: uint8_t excl_mode;   // 1b
    uint8_t  mtxfma_sleep_traps;  // 5b
    uint8_t  mcache_control;      // 2b
    uint16_t tensor_mask;
    uint16_t tensor_error;
    uint16_t ucache_control;
    uint8_t  gsc_progress;        // log2(MLEN) bits
    uint64_t validation0;
    uint64_t validation2;
    uint64_t validation3;
    uint64_t portctrl[4];

    // Other hart internal state
    uint64_t tensor_conv_size;    // can we remove?
    uint64_t tensor_conv_ctrl;    // can we remove?
};


//} // namespace bemu

#endif // BEMU_STATE_H
