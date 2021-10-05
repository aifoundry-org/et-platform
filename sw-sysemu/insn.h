/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_INSN_H
#define BEMU_INSN_H

#include <cstdint>
#include <cstddef>
#include <new>

namespace bemu {


class insn_t
{
public:
    enum : uint16_t {
        flag_1ULP         = 0x0001,
        flag_CMO          = 0x0002, // Coherent Memory Operation (AMOs, etc)
        flag_CSR_READ     = 0x0004,
        flag_CSR_WRITE    = 0x0008,
        flag_LOAD         = 0x0020,
        flag_WFI          = 0x0040,
        flag_REDUCE       = 0x0080,
        flag_TENSOR_LOAD  = 0x0100,
        flag_TENSOR_QUANT = 0x0200,
        flag_TENSOR_STORE = 0x0400,
        flag_TENSOR_WAIT  = 0x0800,
        flag_TENSOR_FMA   = 0x1000,
        flag_STALL        = 0x2000,
        flag_FCC          = 0x4000,
        flag_FLB          = 0x8000
    };

    uint32_t          bits;
    uint16_t          flags;

private:
    template<size_t N> static constexpr int64_t sx(uint32_t x) {
        return (x & (UINT32_MAX << (N-1)))
            ? ((UINT64_MAX << N) | uint64_t(x)) : uint64_t(x);
    }

    template<size_t N> static constexpr int32_t sx32(uint32_t x) {
        return (x & (UINT32_MAX << (N-1)))
            ? ((UINT32_MAX << N) | uint32_t(x)) : uint32_t(x);
    }

public:
    constexpr size_t size() const       { return ((bits & 3) == 3) ? 4 : 2; }

    constexpr bool is_1ulp() const      { return (flags & flag_1ULP); }
    constexpr bool is_cmo() const       { return (flags & flag_CMO); }
    constexpr bool is_csr_read() const  { return (flags & flag_CSR_READ); }
    constexpr bool is_csr_write() const { return (flags & flag_CSR_WRITE); }
    constexpr bool is_fcc_write() const { return (flags & flag_FCC) && (flags & flag_CSR_WRITE); }
    constexpr bool is_flb() const       { return (flags & flag_FLB); }
    constexpr bool is_load() const      { return (flags & flag_LOAD); }
    constexpr bool is_reduce() const    { return (flags & flag_REDUCE); }
    constexpr bool is_wfi() const       { return (flags & flag_WFI); }
    constexpr bool is_stall_write() const        { return (flags & flag_STALL) && (flags & flag_CSR_WRITE); }
    constexpr bool is_tensor_fma_write() const   { return (flags & flag_TENSOR_FMA) && (flags & flag_CSR_WRITE); }
    constexpr bool is_tensor_load_write() const  { return (flags & flag_TENSOR_LOAD) && (flags & flag_CSR_WRITE); }
    constexpr bool is_tensor_quant_write() const { return (flags & flag_TENSOR_QUANT) && (flags & flag_CSR_WRITE); }
    constexpr bool is_tensor_store_write() const { return (flags & flag_TENSOR_STORE) && (flags & flag_CSR_WRITE); }
    constexpr bool is_tensor_wait_write() const  { return (flags & flag_TENSOR_WAIT) && (flags & flag_CSR_WRITE); }

    /* extract RV64IMAF+ET fields */

    constexpr unsigned  rd() const { return (bits >>  7) & 31; }
    constexpr unsigned rs1() const { return (bits >> 15) & 31; }
    constexpr unsigned rs2() const { return (bits >> 20) & 31; }
    //constexpr unsigned aqrl() const { return (bits >> 24) & 3; }

    constexpr unsigned  fd() const { return (bits >>  7) & 31; }
    constexpr unsigned fs1() const { return (bits >> 15) & 31; }
    constexpr unsigned fs2() const { return (bits >> 20) & 31; }
    constexpr unsigned fs3() const { return (bits >> 27) & 31; }
    constexpr unsigned  rm() const { return (bits >> 12) &  7; }

    constexpr unsigned  md() const { return (bits >>  7) & 7; };
    constexpr unsigned ms1() const { return (bits >> 15) & 7; };
    constexpr unsigned ms2() const { return (bits >> 20) & 7; };

    constexpr int64_t i_imm() const {
        return sx<12>((bits >> 20) & 0xFFF);
    }

    constexpr int64_t s_imm() const {
        return sx<12>( ((bits >> 20) & 0xFE0) | ((bits >> 7) & 0x1F) );
    }

    constexpr int64_t b_imm() const {
        return sx<13>( ((bits >> 7) & 0x001E) | ((bits >> 20) & 0x07E0) |
                       ((bits << 4) & 0x0800) | ((bits >> 19) & 0x1000) );
    }

    constexpr int64_t u_imm() const {
        return sx<32>(bits & 0xFFFFF000);
    }

    constexpr int64_t j_imm() const {
        return sx<21>( ((bits >> 20) & 0x0007FE) | ((bits >>  9) & 0x000800) |
                       ((bits >>  0) & 0x0FF000) | ((bits >> 11) & 0x100000) );
    }

    constexpr uint64_t uimm5() const {
        return int64_t((bits >> 15) & 0x1F);
    }

    constexpr unsigned shamt6() const {
        return unsigned((bits >> 20) & 0x3F);
    }

    constexpr unsigned shamt5() const {
        return unsigned((bits >> 20) & 0x1F);
    }

    constexpr uint16_t csrimm() const {
        return uint16_t((bits >> 20) & 0xFFF);
    }

    constexpr unsigned uimm3() const {
        return unsigned((bits >> 20) & 0x7);
    }

    constexpr unsigned uimm8() const {
        return unsigned( ((bits >> 12) & 0x07) | ((bits >> 17) & 0xF8) );
    }

    uint32_t f32imm() const {
        uint32_t val = (bits & 0xFFFFF000);
        uint32_t low4 = (val >> 12) & 0xF;
        return (low4 < 0x8) ? (val | (low4 << 8) | (low4 << 4) | low4      )
                            : (val | (low4 << 8) | (low4 << 4) | (low4 + 1));
    }

    constexpr int32_t i32imm() const {
        return sx32<20>( (bits & 0xFFFFF000) >> 12 );
    }

    constexpr int32_t v_imm() const {
        return sx32<10>( ((bits >> 20) & 0x01f) | ((bits >> 22) & 0x3e0) );
    }

    constexpr unsigned umsk4() const {
        return unsigned( ((bits >> 18) & 0x3) | ((bits >> 21) & 0xC) );
    }

    /* extract RV64C fields */

    constexpr unsigned rvc_rs1()  const { return (bits >> 7) & 31; }
    constexpr unsigned rvc_rs2()  const { return (bits >> 2) & 31; }
    constexpr unsigned rvc_rs1p() const { return 8 + ((bits >> 7) & 7); }
    constexpr unsigned rvc_rs2p() const { return 8 + ((bits >> 2) & 7); }

    constexpr int64_t rvc_imm6() const {
        return sx<6>( ((bits >> 2) & 0x1F) | ((bits >> 7) & 0x20) );
    }

    constexpr int64_t rvc_nzimm_addi16sp() const {
        return sx<10>( ((bits >> 2) & 0x010) | ((bits << 3) & 0x020) |
                       ((bits << 1) & 0x040) | ((bits << 4) & 0x180) |
                       ((bits >> 3) & 0x200) );
    }

    constexpr int64_t rvc_nzuimm_addi4spn() const {
        return ( ((bits >> 4) & 0x004) | ((bits >> 2) & 0x008) |
                 ((bits >> 7) & 0x030) | ((bits >> 1) & 0x3C0) );
    }

    constexpr int64_t rvc_nzimm_lui() const {
        return sx<18>( ((bits << 10) & 0x1F000) | ((bits << 5) & 0x20000) );
    }

    constexpr int64_t rvc_imm_lwsp() const {
        return ( ((bits >> 2) & 0x1C) | ((bits >> 7) & 0x20) |
                      ((bits << 4) & 0xC0) );
    }

    constexpr int64_t rvc_imm_ldsp() const {
        return ( ((bits >> 2) & 0x018) | ((bits >> 7) & 0x020) |
                 ((bits << 4) & 0x1C0) );
    }

    int64_t rvc_imm_swsp() const {
        return ( ((bits >> 7) & 0x3C) | ((bits >> 1) & 0xC0) );
    }

    constexpr int64_t rvc_imm_sdsp() const {
        return ( ((bits >> 7) & 0x038) | ((bits >> 1) & 0x1C0) );
    }

    constexpr int64_t rvc_imm_lsw() const {
       return ( ((bits >> 4) & 0x04) | ((bits >> 7) & 0x38) |
                ((bits << 1) & 0x40) );
    }

    constexpr int64_t rvc_imm_lsd() const {
        return ( ((bits >> 7) & 0x38) | ((bits << 1) & 0xC0) );
    }

    constexpr int64_t rvc_j_imm() const {
        return sx<12>( ((bits >> 2) & 0x00E) | ((bits >> 7) & 0x010) |
                       ((bits << 3) & 0x020) | ((bits << 1) & 0x080) |
                       ((bits << 2) & 0x400) | ((bits >> 1) & 0xB40) );
    }

    constexpr int64_t rvc_b_imm() const {
        return sx<9>( ((bits >> 2) & 0x006) | ((bits >> 7) & 0x018) |
                      ((bits << 3) & 0x020) | ((bits << 1) & 0x0C0) |
                      ((bits >> 4) & 0x100) );
    }

    constexpr unsigned rvc_shamt() const {
        return unsigned( ((bits >> 2) & 0x1F) | ((bits >> 7) & 0x20) );
    }
};


} // namespace bemu

#endif // BEMU_INSN_H
