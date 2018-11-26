#ifndef __INSN_H__
#define __INSN_H__

#include <cstdint>
#include <cstddef>
#include <new>
#include "emu_defines.h"

// Instruction encodings that match minstmatch/minstmask will execute this
extern void check_minst_match(uint32_t bits);

class insn_t
{
public:
    insn_t() : bits(0), flags(0), exec_fn(nullptr) {}

    void fetch_and_decode(uint64_t vaddr);

    void execute() const {
        check_minst_match(bits);
        (exec_fn) (*this);
    }

    size_t size() const {
        return ((bits & 3) == 3) ? 4 : 2;
    }

    bool is_1ulp() const            { return (flags & flag_1ULP); }
    bool is_amo() const             { return (flags & flag_AMO); }
    bool is_csr_read() const        { return (flags & flag_CSR_READ); }
    bool is_fcc() const             { return (flags & flag_FCC); }
    bool is_flb() const             { return (flags & flag_FLB); }
    bool is_load() const            { return (flags & flag_LOAD); }
    bool is_reduce() const          { return (flags & flag_REDUCE); }
    bool is_stall() const           { return (flags & flag_STALL); }
    bool is_tensor_fma() const      { return (flags & flag_TENSOR_FMA); }
    bool is_tensor_load() const     { return (flags & flag_TENSOR_LOAD); }
    bool is_tensor_quant() const    { return (flags & flag_TENSOR_QUANT); }
    bool is_wfi() const             { return (flags & flag_WFI); }

    /* extract RV64IMAF+ET fields */

    xreg  rd() const { return xreg((bits >>  7) & 31); }
    xreg rs1() const { return xreg((bits >> 15) & 31); }
    xreg rs2() const { return xreg((bits >> 20) & 31); }
    //int aqrl() const { return (bits >> 24) & 3; }

    freg  fd() const { return freg((bits >>  7) & 31); }
    freg fs1() const { return freg((bits >> 15) & 31); }
    freg fs2() const { return freg((bits >> 20) & 31); }
    freg fs3() const { return freg((bits >> 27) & 31); }
    rounding_mode rm() const { return rounding_mode((bits >> 12) & 7); }

    mreg  md() const { return mreg((bits >>  7) & 7); };
    mreg ms1() const { return mreg((bits >> 15) & 7); };
    mreg ms2() const { return mreg((bits >> 20) & 7); };

    int64_t i_imm() const {
        return sx<12>((bits >> 20) & 0xFFF);
    }

    int64_t s_imm() const {
        return sx<12>( ((bits >> 20) & 0xFE0) | ((bits >> 7) & 0x1F) );
    }

    int64_t b_imm() const {
        return sx<13>( ((bits >> 7) & 0x001E) | ((bits >> 20) & 0x07E0) |
                       ((bits << 4) & 0x0800) | ((bits >> 19) & 0x1000) );
    }

    int64_t u_imm() const {
        return sx<32>(bits & 0xFFFFF000);
    }

    int64_t j_imm() const {
        return sx<21>( ((bits >> 20) & 0x0007FE) | ((bits >>  9) & 0x000800) |
                       ((bits >>  0) & 0x0FF000) | ((bits >> 11) & 0x100000) );
    }

    uint64_t uimm5() const {
        return int64_t((bits >> 15) & 0x1F);
    }

    unsigned shamt6() const {
        return unsigned((bits >> 20) & 0x3F);
    }

    unsigned shamt5() const {
        return unsigned((bits >> 20) & 0x1F);
    }

    uint16_t csrimm() const {
        return uint16_t((bits >> 20) & 0xFFF);
    }

    unsigned uimm3() const {
        return unsigned((bits >> 20) & 0x7);
    }

    unsigned uimm8() const {
        return unsigned( ((bits >> 12) & 0x07) | ((bits >> 17) & 0xF8) );
    }

    uint32_t f32imm() const {
        uint32_t val = (bits & 0xFFFFF000);
        uint32_t low4 = (val >> 12) & 0xF;
        return (low4 < 0x8) ? (val | (low4 << 8) | (low4 << 4) | low4      )
                            : (val | (low4 << 8) | (low4 << 4) | (low4 + 1));
    }

    int32_t i32imm() const {
        return sx32<20>( (bits & 0xFFFFF000) >> 12 );
    }

    int32_t v_imm() const {
        return sx32<10>( ((bits >> 20) & 0x01f) | ((bits >> 22) & 0x3e0) );
    }

    unsigned umsk4() const {
        return unsigned( ((bits >> 18) & 0x3) | ((bits >> 21) & 0xC) );
    }

    /* extract RV64C fields */

    xreg rvc_rs1()  const { return xreg((bits >> 7) & 31); }
    xreg rvc_rs2()  const { return xreg((bits >> 2) & 31); }
    xreg rvc_rs1p() const { return xreg(8 + ((bits >> 7) & 7)); }
    xreg rvc_rs2p() const { return xreg(8 + ((bits >> 2) & 7)); }

    int64_t rvc_imm6() const {
        return sx<6>( ((bits >> 2) & 0x1F) | ((bits >> 7) & 0x20) );
    }

    int64_t rvc_nzimm_addi16sp() const {
        return sx<10>( ((bits >> 2) & 0x010) | ((bits << 3) & 0x020) |
                       ((bits << 1) & 0x040) | ((bits << 4) & 0x180) |
                       ((bits >> 3) & 0x200) );
    }

    int64_t rvc_nzuimm_addi4spn() const {
        return ( ((bits >> 4) & 0x004) | ((bits >> 2) & 0x008) |
                 ((bits >> 7) & 0x030) | ((bits >> 1) & 0x3C0) );
    }

    int64_t rvc_nzimm_lui() const {
        return sx<18>( ((bits << 10) & 0x1F000) | ((bits << 5) & 0x20000) );
    }

    int64_t rvc_imm_lwsp() const {
        return ( ((bits >> 2) & 0x1C) | ((bits >> 7) & 0x20) |
                      ((bits << 4) & 0xC0) );
    }

    int64_t rvc_imm_ldsp() const {
        return ( ((bits >> 2) & 0x018) | ((bits >> 7) & 0x020) |
                 ((bits << 4) & 0x1C0) );
    }

    int64_t rvc_imm_swsp() const {
        return ( ((bits >> 7) & 0x3C) | ((bits >> 1) & 0xC0) );
    }

    int64_t rvc_imm_sdsp() const {
        return ( ((bits >> 7) & 0x038) | ((bits >> 1) & 0x1C0) );
    }

    int64_t rvc_imm_lsw() const {
       return ( ((bits >> 4) & 0x04) | ((bits >> 7) & 0x38) |
                ((bits << 1) & 0x40) );
    }

    int64_t rvc_imm_lsd() const {
        return ( ((bits >> 7) & 0x38) | ((bits << 1) & 0xC0) );
    }

    int64_t rvc_j_imm() const {
        return sx<12>( ((bits >> 2) & 0x00E) | ((bits >> 7) & 0x010) |
                       ((bits << 3) & 0x020) | ((bits << 1) & 0x080) |
                       ((bits << 2) & 0x400) | ((bits >> 1) & 0xB40) );
    }

    int64_t rvc_b_imm() const {
        return sx<9>( ((bits >> 2) & 0x006) | ((bits >> 7) & 0x018) |
                      ((bits << 3) & 0x020) | ((bits << 1) & 0x0C0) |
                      ((bits >> 4) & 0x100) );
    }

    unsigned rvc_shamt() const {
        return unsigned( ((bits >> 2) & 0x1F) | ((bits >> 7) & 0x10) );
    }

public:
    const uint32_t  bits;
    const uint32_t  flags;

    constexpr static uint32_t flag_1ULP         = 0x0001;
    constexpr static uint32_t flag_AMO          = 0x0002;
    constexpr static uint32_t flag_CSR_READ     = 0x0004;
    constexpr static uint32_t flag_FCC          = 0x0008;
    constexpr static uint32_t flag_FLB          = 0x0010;
    constexpr static uint32_t flag_LOAD         = 0x0020;
    constexpr static uint32_t flag_REDUCE       = 0x0040;
    constexpr static uint32_t flag_TENSOR_FMA   = 0x0080;
    constexpr static uint32_t flag_TENSOR_LOAD  = 0x0100;
    constexpr static uint32_t flag_TENSOR_QUANT = 0x0200;
    constexpr static uint32_t flag_WFI          = 0x0400;
    constexpr static uint32_t flag_STALL        = 0x0800;

    /* instruction execution functions */
    typedef void (*insn_exec_func_t)(insn_t);

private:
    const insn_exec_func_t  exec_fn;

    insn_t(uint32_t b, uint32_t f, insn_exec_func_t e)
        : bits(b), flags(f), exec_fn(e)
    {}

    template<size_t N> static int64_t sx(uint32_t x) {
        return (x & (UINT32_MAX << (N-1)))
            ? ((UINT64_MAX << N) | uint64_t(x))
            : uint64_t(x);
    }

    template<size_t N> static int32_t sx32(uint32_t x) {
        return (x & (UINT32_MAX << (N-1)))
            ? ((UINT32_MAX << N) | uint32_t(x))
            : uint32_t(x);
    }
};

    
// NB: "slow", try to avoid
extern csr get_csr_enum(uint16_t);


#endif // __INSN_H__
