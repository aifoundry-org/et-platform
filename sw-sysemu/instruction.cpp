#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include <boost/regex.hpp>

#include "instruction.h"
#include "emu.h"
#include "emu_gio.h"

using emu::gprintf;

// Typedef for the hash of functions
typedef std::unordered_map<std::string, func_ptr> emu_ptr_hash_t;

// Map of mnemonic string to function pointer
static const emu_ptr_hash_t pointer_cache({
// ----- RV64I -----------------------------------------------------------------
    {"beq",            func_ptr(beq)},
    {"bne",            func_ptr(bne)},
    {"blt",            func_ptr(blt)},
    {"bge",            func_ptr(bge)},
    {"bltu",           func_ptr(bltu)},
    {"bgeu",           func_ptr(bgeu)},
    {"jalr",           func_ptr(jalr)},
    {"jal",            func_ptr(jal)},
    {"lui",            func_ptr(lui)},
    {"auipc",          func_ptr(auipc)},
    {"addi",           func_ptr(addi)},
    {"slli",           func_ptr(slli)},
    {"slti",           func_ptr(slti)},
    {"sltiu",          func_ptr(sltiu)},
    {"xori",           func_ptr(xori)},
    {"srli",           func_ptr(srli)},
    {"srai",           func_ptr(srai)},
    {"ori",            func_ptr(ori)},
    {"andi",           func_ptr(andi)},
    {"add",            func_ptr(add)},
    {"sub",            func_ptr(sub)},
    {"sll",            func_ptr(sll)},
    {"slt",            func_ptr(slt)},
    {"sltu",           func_ptr(sltu)},
    {"xor",            func_ptr(xor_)},
    {"srl",            func_ptr(srl)},
    {"sra",            func_ptr(sra)},
    {"or",             func_ptr(or_)},
    {"and",            func_ptr(and_)},
    {"addiw",          func_ptr(addiw)},
    {"slliw",          func_ptr(slliw)},
    {"srliw",          func_ptr(srliw)},
    {"sraiw",          func_ptr(sraiw)},
    {"addw",           func_ptr(addw)},
    {"subw",           func_ptr(subw)},
    {"sllw",           func_ptr(sllw)},
    {"srlw",           func_ptr(srlw)},
    {"sraw",           func_ptr(sraw)},
    {"lb",             func_ptr(lb)},
    {"lh",             func_ptr(lh)},
    {"lw",             func_ptr(lw)},
    {"ld",             func_ptr(ld)},
    {"lbu",            func_ptr(lbu)},
    {"lhu",            func_ptr(lhu)},
    {"lwu",            func_ptr(lwu)},
    {"sd",             func_ptr(sd)},
    {"sw",             func_ptr(sw)},
    {"sh",             func_ptr(sh)},
    {"sb",             func_ptr(sb)},
    {"fence",          func_ptr(fence)},
    {"fence_i",        func_ptr(fence_i)},
// ----- RV32M -----------------------------------------------------------------
    {"mul",            func_ptr(mul)},
    {"mulh",           func_ptr(mulh)},
    {"mulhsu",         func_ptr(mulhsu)},
    {"mulhu",          func_ptr(mulhu)},
    {"div",            func_ptr(div_)},
    {"divu",           func_ptr(divu)},
    {"rem",            func_ptr(rem)},
    {"remu",           func_ptr(remu)},
// ----- RV64M -----------------------------------------------------------------
    {"mulw",           func_ptr(mulw)},
    {"divw",           func_ptr(divw)},
    {"divuw",          func_ptr(divuw)},
    {"remw",           func_ptr(remw)},
    {"remuw",          func_ptr(remuw)},
// ----- RV32A -----------------------------------------------------------------
    {"amoadd_w",       func_ptr(amoadd_w)},
    {"amoxor_w",       func_ptr(amoxor_w)},
    {"amoor_w",        func_ptr(amoor_w)},
    {"amoand_w",       func_ptr(amoand_w)},
    {"amomin_w",       func_ptr(amomin_w)},
    {"amomax_w",       func_ptr(amomax_w)},
    {"amominu_w",      func_ptr(amominu_w)},
    {"amomaxu_w",      func_ptr(amomaxu_w)},
    {"amoswap_w",      func_ptr(amoswap_w)},
//  {"lr_w",            func_ptr(lr_w)},
//  {"sc_w",            func_ptr(sc_w)},
// ----- RV64A -----------------------------------------------------------------
    {"amoadd_d",       func_ptr(amoadd_d)},
    {"amoxor_d",       func_ptr(amoxor_d)},
    {"amoor_d",        func_ptr(amoor_d)},
    {"amoand_d",       func_ptr(amoand_d)},
    {"amomin_d",       func_ptr(amomin_d)},
    {"amomax_d",       func_ptr(amomax_d)},
    {"amominu_d",      func_ptr(amominu_d)},
    {"amomaxu_d",      func_ptr(amomaxu_d)},
    {"amoswap_d",      func_ptr(amoswap_d)},
//  {"lr_d",            func_ptr(lr_d)},
//  {"sc_d",            func_ptr(sc_d)},
// ----- SYSTEM ----------------------------------------------------------------
    {"ecall",          func_ptr(ecall)},
    {"ebreak",         func_ptr(ebreak)},
//  {"uret",            func_ptr(uret)},
    {"sret",           func_ptr(sret)},
    {"mret",           func_ptr(mret)},
//  {"dret",            func_ptr(dret)},
    {"sfence_vma",     func_ptr(sfence_vma)},
    {"wfi",            func_ptr(wfi)},
    {"csrrw",          func_ptr(csrrw)},
    {"csrrs",          func_ptr(csrrs)},
    {"csrrc",          func_ptr(csrrc)},
    {"csrrwi",         func_ptr(csrrwi)},
    {"csrrsi",         func_ptr(csrrsi)},
    {"csrrci",         func_ptr(csrrci)},
// ----- RV64C -----------------------------------------------------------------
    {"c_jalr",         func_ptr(c_jalr)},
    {"c_jal",          func_ptr(c_jal)},
// ----- RV64F -----------------------------------------------------------------
    {"fadd_s",         func_ptr(fadd_s)},
    {"fsub_s",         func_ptr(fsub_s)},
    {"fmul_s",         func_ptr(fmul_s)},
    {"fdiv_s",         func_ptr(fdiv_s)},
    {"fsgnj_s",        func_ptr(fsgnj_s)},
    {"fsgnjn_s",       func_ptr(fsgnjn_s)},
    {"fsgnjx_s",       func_ptr(fsgnjx_s)},
    {"fmin_s",         func_ptr(fmin_s)},
    {"fmax_s",         func_ptr(fmax_s)},
    {"fsqrt_s",        func_ptr(fsqrt_s)},
    {"feq_s",          func_ptr(feq_s)},
    {"fle_s",          func_ptr(fle_s)},
    {"flt_s",          func_ptr(flt_s)},
    {"fcvt_w_s",       func_ptr(fcvt_w_s)},
    {"fcvt_wu_s",      func_ptr(fcvt_wu_s)},
    {"fcvt_l_s",       func_ptr(fcvt_l_s)},
    {"fcvt_lu_s",      func_ptr(fcvt_lu_s)},
    {"fmv_x_w",        func_ptr(fmv_x_w)},
    {"fmv_x_s",        func_ptr(fmv_x_w)},     // old name for fmv_x_w
    {"fclass_s",       func_ptr(fclass_s)},
    {"fcvt_s_w",       func_ptr(fcvt_s_w)},
    {"fcvt_s_wu",      func_ptr(fcvt_s_wu)},
    {"fcvt_s_l",       func_ptr(fcvt_s_l)},
    {"fcvt_s_lu",      func_ptr(fcvt_s_lu)},
    {"fmv_w_x",        func_ptr(fmv_w_x)},
    {"fmv_s_x",        func_ptr(fmv_w_x)},     // old name for fmv_w_x
    {"flw",            func_ptr(flw)},
    {"fsw",            func_ptr(fsw)},
    {"fmadd_s",        func_ptr(fmadd_s)},
    {"fmsub_s",        func_ptr(fmsub_s)},
    {"fnmsub_s",       func_ptr(fnmsub_s)},
    {"fnmadd_s",       func_ptr(fnmadd_s)},
// ----- Esperanto mask extension ----------------------------------------------
    {"maskand",        func_ptr(maskand)},
    {"maskor",         func_ptr(maskor)},
    {"maskxor",        func_ptr(maskxor)},
    {"masknot",        func_ptr(masknot)},
    {"mova_x_m",       func_ptr(mova_x_m)},
    {"mova_m_x",       func_ptr(mova_m_x)},
    {"mov_m_x",        func_ptr(mov_m_x)},
    {"maskpopc",       func_ptr(maskpopc)},
    {"maskpopcz",      func_ptr(maskpopcz)},
    {"maskpopc_rast",  func_ptr(maskpopc_rast)},
// ----- Esperanto packed-single extension -------------------------------------
    {"flw_ps",         func_ptr(flw_ps)},
    {"flq2",           func_ptr(flq2)},
    {"fsw_ps",         func_ptr(fsw_ps)},
    {"fswg_ps",        func_ptr(fswg_ps)},
    {"fsq2",           func_ptr(fsq2)},
    {"fbc_ps",         func_ptr(fbc_ps)},
    {"fbci_ps",        func_ptr(fbci_ps)},
    {"fbcx_ps",        func_ptr(fbcx_ps)},
    {"fgw_ps",         func_ptr(fgw_ps)},
    {"fgh_ps",         func_ptr(fgh_ps)},
    {"fgb_ps",         func_ptr(fgb_ps)},
    {"fscw_ps",        func_ptr(fscw_ps)},
    {"fsch_ps",        func_ptr(fsch_ps)},
    {"fscb_ps",        func_ptr(fscb_ps)},
    {"fg32b_ps",       func_ptr(fg32b_ps)},
    {"fg32h_ps",       func_ptr(fg32h_ps)},
    {"fg32w_ps",       func_ptr(fg32w_ps)},
    {"fsc32b_ps",      func_ptr(fsc32b_ps)},
    {"fsc32h_ps",      func_ptr(fsc32h_ps)},
    {"fsc32w_ps",      func_ptr(fsc32w_ps)},
    {"fadd_ps",        func_ptr(fadd_ps)},
    {"fsub_ps",        func_ptr(fsub_ps)},
    {"fmul_ps",        func_ptr(fmul_ps)},
    {"fdiv_ps",        func_ptr(fdiv_ps)},
    {"fsgnj_ps",       func_ptr(fsgnj_ps)},
    {"fsgnjn_ps",      func_ptr(fsgnjn_ps)},
    {"fsgnjx_ps",      func_ptr(fsgnjx_ps)},
    {"fmin_ps",        func_ptr(fmin_ps)},
    {"fmax_ps",        func_ptr(fmax_ps)},
    {"fsqrt_ps",       func_ptr(fsqrt_ps)},
    {"feq_ps",         func_ptr(feq_ps)},
    {"fle_ps",         func_ptr(fle_ps)},
    {"flt_ps",         func_ptr(flt_ps)},
    {"feqm_ps",        func_ptr(feqm_ps)},
    {"flem_ps",        func_ptr(flem_ps)},
    {"fltm_ps",        func_ptr(fltm_ps)},
    {"fsetm_ps",       func_ptr(fsetm_ps)},
    {"fcmov_ps",       func_ptr(fcmov_ps)},
    {"fcmovm_ps",      func_ptr(fcmovm_ps)},
    {"fmvz_x_ps",      func_ptr(fmvz_x_ps)},
    {"fmvs_x_ps",      func_ptr(fmvs_x_ps)},
    {"fswizz_ps",      func_ptr(fswizz_ps)},
    {"fcvt_pw_ps",     func_ptr(fcvt_pw_ps)},
    {"fcvt_pwu_ps",    func_ptr(fcvt_pwu_ps)},
//    fcvt_pl_ps
//    fcvt_plu_ps
    {"fclass_ps",      func_ptr(fclass_ps)},
    {"fcvt_ps_pw",     func_ptr(fcvt_ps_pw)},
    {"fcvt_ps_pwu",    func_ptr(fcvt_ps_pwu)},
//    fcvt_ps_pl
//    fcvt_ps_plu
//    fmv_ps_px -- equivalent to a X-reg broadcast to V-reg
    {"fmadd_ps",       func_ptr(fmadd_ps)},
    {"fmsub_ps",       func_ptr(fmsub_ps)},
    {"fnmsub_ps",      func_ptr(fnmsub_ps)},
    {"fnmadd_ps",      func_ptr(fnmadd_ps)},
    {"fcvt_ps_f16",    func_ptr(fcvt_ps_f16)},
    {"fcvt_ps_un24",   func_ptr(fcvt_ps_un24)},
    {"fcvt_ps_un16",   func_ptr(fcvt_ps_un16)},
    {"fcvt_ps_un10",   func_ptr(fcvt_ps_un10)},
    {"fcvt_ps_un8",    func_ptr(fcvt_ps_un8)},
    {"fcvt_ps_un2",    func_ptr(fcvt_ps_un2)},
    {"fcvt_ps_sn16",   func_ptr(fcvt_ps_sn16)},
    {"fcvt_ps_sn8",    func_ptr(fcvt_ps_sn8)},
    {"fcvt_ps_f11",    func_ptr(fcvt_ps_f11)},
    {"fcvt_ps_f10",    func_ptr(fcvt_ps_f10)},
    {"fcvt_f16_ps",    func_ptr(fcvt_f16_ps)},
    {"fcvt_un24_ps",   func_ptr(fcvt_un24_ps)},
    {"fcvt_un16_ps",   func_ptr(fcvt_un16_ps)},
    {"fcvt_un10_ps",   func_ptr(fcvt_un10_ps)},
    {"fcvt_un8_ps",    func_ptr(fcvt_un8_ps)},
    {"fcvt_un2_ps",    func_ptr(fcvt_un2_ps)},
    {"fcvt_sn16_ps",   func_ptr(fcvt_sn16_ps)},
    {"fcvt_sn8_ps",    func_ptr(fcvt_sn8_ps)},
    {"fcvt_f11_ps",    func_ptr(fcvt_f11_ps)},
    {"fcvt_f10_ps",    func_ptr(fcvt_f10_ps)},
    {"fsin_ps",        func_ptr(fsin_ps)},
    {"fexp_ps",        func_ptr(fexp_ps)},
    {"flog_ps",        func_ptr(flog_ps)},
    {"ffrc_ps",        func_ptr(ffrc_ps)},
    {"fround_ps",      func_ptr(fround_ps)},
    {"frcp_ps",        func_ptr(frcp_ps)},
    {"frsq_ps",        func_ptr(frsq_ps)},
//    {"fltabs_ps",     func_ptr(fltabs_ps)},
    {"cubeface_ps",    func_ptr(cubeface_ps)},
    {"cubefaceidx_ps", func_ptr(cubefaceidx_ps)},
    {"cubesgnsc_ps",   func_ptr(cubesgnsc_ps)},
    {"cubesgntc_ps",   func_ptr(cubesgntc_ps)},
    {"fcvt_ps_rast",   func_ptr(fcvt_ps_rast)},
    {"fcvt_rast_ps",   func_ptr(fcvt_rast_ps)},
    {"frcp_fix_rast",  func_ptr(frcp_fix_rast)},
// NB: these are scalar instructions!
    {"packb",          func_ptr(packb)},
    {"bitmixb",        func_ptr(bitmixb)},
// FIXME: THIS INSTRUCTION IS OBSOLETE
    {"frcpfxp_ps",     func_ptr(frcpfxp_ps)},
// Texture sampling -- FIXME: THESE INSTRUCTIONS ARE OBSOLETE
    {"texsndh",        func_ptr(texsndh)},
    {"texsnds",        func_ptr(texsnds)},
    {"texsndt",        func_ptr(texsndt)},
    {"texsndr",        func_ptr(texsndr)},
    {"texrcv",         func_ptr(texrcv)},
// ----- Esperanto packed-integer extension ------------------------------------
    {"fbci_pi",        func_ptr(fbci_pi)},
    {"feq_pi",         func_ptr(feq_pi)},   // RV64I: beq
//  {"fne_pi",          func_ptr(fne_pi)},   // RV64I: bne
    {"fle_pi",         func_ptr(fle_pi)},   // RV64I: bge
    {"flt_pi",         func_ptr(flt_pi)},   // RV64I: blt
//  {"fleu_pi",         func_ptr(fleu_pi)},  // RV64I: bgeu
    {"fltu_pi",        func_ptr(fltu_pi)},  // RV64I: bltu
//  {"feqm_pi",         func_ptr(feqm_pi)},
//  {"fnem_pi",         func_ptr(fnem_pi)},
//  {"flem_pi",         func_ptr(flem_pi)},
    {"fltm_pi",        func_ptr(fltm_pi)},
//  {"fleum_pi",        func_ptr(fleum_pi)},
//  {"fltum_pi",        func_ptr(fltum_pi)},
    {"faddi_pi",       func_ptr(faddi_pi)},
    {"fslli_pi",       func_ptr(fslli_pi)},
//  {"fslti_pi",        func_ptr(fslti_pi)},
//  {"fsltiu_pi",       func_ptr(fsltiu_pi)},
    {"fxori_pi",       func_ptr(fxori_pi)},
    {"fsrli_pi",       func_ptr(fsrli_pi)},
    {"fsrai_pi",       func_ptr(fsrai_pi)},
    {"fori_pi",        func_ptr(fori_pi)},
    {"fandi_pi",       func_ptr(fandi_pi)},
    {"fadd_pi",        func_ptr(fadd_pi)},
    {"fsub_pi",        func_ptr(fsub_pi)},
    {"fsll_pi",        func_ptr(fsll_pi)},
//  {"fslt_pi",         func_ptr(fslt_pi)},
//  {"fsltu_pi",        func_ptr(fsltu_pi)},
    {"fxor_pi",        func_ptr(fxor_pi)},
    {"fsrl_pi",        func_ptr(fsrl_pi)},
    {"fsra_pi",        func_ptr(fsra_pi)},
    {"for_pi",         func_ptr(for_pi)},
    {"fand_pi",        func_ptr(fand_pi)},
    {"fnot_pi",        func_ptr(fnot_pi)},
    {"fsat8_pi",       func_ptr(fsat8_pi)},
    {"fsatu8_pi",      func_ptr(fsatu8_pi)},
    {"fpackreph_pi",   func_ptr(fpackreph_pi)},
    {"fpackrepb_pi",   func_ptr(fpackrepb_pi)},
    {"fmul_pi",        func_ptr(fmul_pi)},
    {"fmulh_pi",       func_ptr(fmulh_pi)},
// fmulhsu_pi will not be implemented
    {"fmulhu_pi",      func_ptr(fmulhu_pi)},
    {"fdiv_pi",        func_ptr(fdiv_pi)},
    {"fdivu_pi",       func_ptr(fdivu_pi)},
    {"frem_pi",        func_ptr(frem_pi)},
    {"fremu_pi",       func_ptr(fremu_pi)},
    {"fmin_pi",        func_ptr(fmin_pi)},
    {"fmax_pi",        func_ptr(fmax_pi)},
    {"fminu_pi",       func_ptr(fminu_pi)},
    {"fmaxu_pi",       func_ptr(fmaxu_pi)},
// ----- Esperanto atomic extension --------------------------------------------
    {"amoswapl_w",     func_ptr(amoswapl_w)},
    {"amoaddl_w",      func_ptr(amoaddl_w)},
    {"amoxorl_w",      func_ptr(amoxorl_w)},
    {"amoorl_w",       func_ptr(amoorl_w)},
    {"amoandl_w",      func_ptr(amoandl_w)},
    {"amominl_w",      func_ptr(amominl_w)},
    {"amomaxl_w",      func_ptr(amomaxl_w)},
    {"amominul_w",     func_ptr(amominul_w)},
    {"amomaxul_w",     func_ptr(amomaxul_w)},

    {"amoswapl_d",     func_ptr(amoswapl_d)},
    {"amoaddl_d",      func_ptr(amoaddl_d)},
    {"amoxorl_d",      func_ptr(amoxorl_d)},
    {"amoorl_d",       func_ptr(amoorl_d)},
    {"amoandl_d",      func_ptr(amoandl_d)},
    {"amominl_d",      func_ptr(amominl_d)},
    {"amomaxl_d",      func_ptr(amomaxl_d)},
    {"amominul_d",     func_ptr(amominul_d)},
    {"amomaxul_d",     func_ptr(amomaxul_d)},

    {"famoswapl_pi",   func_ptr(famoswapl_pi)},
    {"famoaddl_pi",    func_ptr(famoaddl_pi)},
    {"famoxorl_pi",    func_ptr(famoxorl_pi)},
    {"famoorl_pi",     func_ptr(famoorl_pi)},
    {"famoandl_pi",    func_ptr(famoandl_pi)},
    {"famominl_pi",    func_ptr(famominl_pi)},
    {"famomaxl_pi",    func_ptr(famomaxl_pi)},
    {"famominul_pi",   func_ptr(famominul_pi)},
    {"famomaxul_pi",   func_ptr(famomaxul_pi)},
    {"famominl_ps",    func_ptr(famominl_ps)},
    {"famomaxl_ps",    func_ptr(famomaxl_ps)},

    {"amoswapg_w",     func_ptr(amoswapg_w)},
    {"amoaddg_w",      func_ptr(amoaddg_w)},
    {"amoxorg_w",      func_ptr(amoxorg_w)},
    {"amoorg_w",       func_ptr(amoorg_w)},
    {"amoandg_w",      func_ptr(amoandg_w)},
    {"amoming_w",      func_ptr(amoming_w)},
    {"amomaxg_w",      func_ptr(amomaxg_w)},
    {"amominug_w",     func_ptr(amominug_w)},
    {"amomaxug_w",     func_ptr(amomaxug_w)},

    {"amoswapg_d",     func_ptr(amoswapg_d)},
    {"amoaddg_d",      func_ptr(amoaddg_d)},
    {"amoxorg_d",      func_ptr(amoxorg_d)},
    {"amoorg_d",       func_ptr(amoorg_d)},
    {"amoandg_d",      func_ptr(amoandg_d)},
    {"amoming_d",      func_ptr(amoming_d)},
    {"amomaxg_d",      func_ptr(amomaxg_d)},
    {"amominug_d",     func_ptr(amominug_d)},
    {"amomaxug_d",     func_ptr(amomaxug_d)},

    {"famoswapg_pi",   func_ptr(famoswapg_pi)},
    {"famoaddg_pi",    func_ptr(famoaddg_pi)},
    {"famoxorg_pi",    func_ptr(famoxorg_pi)},
    {"famoorg_pi",     func_ptr(famoorg_pi)},
    {"famoandg_pi",    func_ptr(famoandg_pi)},
    {"famoming_pi",    func_ptr(famoming_pi)},
    {"famomaxg_pi",    func_ptr(famomaxg_pi)},
    {"famominug_pi",   func_ptr(famominug_pi)},
    {"famomaxug_pi",   func_ptr(famomaxug_pi)},
    {"famoming_ps",    func_ptr(famoming_ps)},
    {"famomaxg_ps",    func_ptr(famomaxg_ps)},

    // {"flwl_ps",        func_ptr(flwl_ps)},
    // {"fswl_ps",        func_ptr(fswl_ps)},
    // {"fgwl_ps",        func_ptr(fgwl_ps)},
    // {"fscwl_ps",       func_ptr(fscwl_ps)},
    // {"flwg_ps",        func_ptr(flwg_ps)},
    // {"fswg_ps",        func_ptr(fswg_ps)},
    // {"fgwg_ps",        func_ptr(fgwg_ps)},
    // {"fscwg_ps",       func_ptr(fscwg_ps)},
// ----- Esperanto cache control extension -------------------------------------
// ----- Esperanto messaging extension -----------------------------------------
// ----- Esperanto tensor extension --------------------------------------------
// ----- Esperanto fast local barrier extension --------------------------------
// ----- Illegal instruction ---------------------------------------------------
    {"unknown",        func_ptr(unknown)},
});

// floating-point instructions with optional rounding-mode as 3rd operand
static const std::unordered_set<std::string> rm2args({
    "fsqrt_s",
    "fcvt_w_s", "fcvt_wu_s", "fcvt_l_s", "fcvt_lu_s",
    "fcvt_s_w", "fcvt_s_wu", "fcvt_s_l", "fcvt_s_lu",
    "fsqrt_ps",
    "fcvt_pw_ps", "fcvt_pwu_ps", "fcvt_pl_ps", "fcvt_plu_ps",
    "fcvt_ps_pw", "fcvt_ps_pwu", "fcvt_ps_pl", "fcvt_ps_plu",
    "ffrc_ps", "fround_ps",
    "fcvt_ps_rast"
});

// floating-point instructions with optional rounding-mode as 4th operand
static const std::unordered_set<std::string> rm3args({
    "fadd_s", "fsub_s", "fmul_s", "fdiv_s",
    "fadd_ps", "fsub_ps", "fmul_ps", "fdiv_ps",
    "fltabs_ps"
});

// floating-point instructions with optional rounding-mode as 5th operand
static const std::unordered_set<std::string> rm4args({
    "fmadd_s", "fmsub_s", "fnmsub_s", "fnmadd_s",
    "fmadd_ps", "fmsub_ps", "fnmsub_ps", "fnmadd_ps"
});

// Typedef for the hash op instruction operands
typedef std::unordered_map<std::string, int> emu_opnd_hash_t;

// Map of resource string to instruction operand
static const emu_opnd_hash_t operand_cache({
    // ----- integer registers -----------------------------------------------
    {"zero",            0},
    {"ra",              1},
    {"sp",              2},
    {"gp",              3},
    {"tp",              4},
    {"t0",              5},
    {"t1",              6},
    {"t2",              7},
    {"s0",              8},
    {"s1",              9},
    {"a0",              10},
    {"a1",              11},
    {"a2",              12},
    {"a3",              13},
    {"a4",              14},
    {"a5",              15},
    {"a6",              16},
    {"a7",              17},
    {"s2",              18},
    {"s3",              19},
    {"s4",              20},
    {"s5",              21},
    {"s6",              22},
    {"s7",              23},
    {"s8",              24},
    {"s9",              25},
    {"s10",             26},
    {"s11",             27},
    {"t3",              28},
    {"t4",              29},
    {"t5",              30},
    {"t6",              31},
    {"x0",              0},
    {"x1",              1},
    {"x2",              2},
    {"x3",              3},
    {"x4",              4},
    {"x5",              5},
    {"x6",              6},
    {"x7",              7},
    {"x8",              8},
    {"x9",              9},
    {"x10",             10},
    {"x11",             11},
    {"x12",             12},
    {"x13",             13},
    {"x14",             14},
    {"x15",             15},
    {"x16",             16},
    {"x17",             17},
    {"x18",             18},
    {"x19",             19},
    {"x20",             20},
    {"x21",             21},
    {"x22",             22},
    {"x23",             23},
    {"x24",             24},
    {"x25",             25},
    {"x26",             26},
    {"x27",             27},
    {"x28",             28},
    {"x29",             29},
    {"x30",             30},
    {"x31",             31},
    // ----- floating-point registers ----------------------------------------
    {"ft0",             0},
    {"ft1",             1},
    {"ft2",             2},
    {"ft3",             3},
    {"ft4",             4},
    {"ft5",             5},
    {"ft6",             6},
    {"ft7",             7},
    {"fs0",             8},
    {"fs1",             9},
    {"fa0",             10},
    {"fa1",             11},
    {"fa2",             12},
    {"fa3",             13},
    {"fa4",             14},
    {"fa5",             15},
    {"fa6",             16},
    {"fa7",             17},
    {"fs2",             18},
    {"fs3",             19},
    {"fs4",             20},
    {"fs5",             21},
    {"fs6",             22},
    {"fs7",             23},
    {"fs8",             24},
    {"fs9",             25},
    {"fs10",            26},
    {"fs11",            27},
    {"ft8",             28},
    {"ft9",             29},
    {"ft10",            30},
    {"ft11",            31},
    {"f0",              0},
    {"f1",              1},
    {"f2",              2},
    {"f3",              3},
    {"f4",              4},
    {"f5",              5},
    {"f6",              6},
    {"f7",              7},
    {"f8",              8},
    {"f9",              9},
    {"f10",             10},
    {"f11",             11},
    {"f12",             12},
    {"f13",             13},
    {"f14",             14},
    {"f15",             15},
    {"f16",             16},
    {"f17",             17},
    {"f18",             18},
    {"f19",             19},
    {"f20",             20},
    {"f21",             21},
    {"f22",             22},
    {"f23",             23},
    {"f24",             24},
    {"f25",             25},
    {"f26",             26},
    {"f27",             27},
    {"f28",             28},
    {"f29",             29},
    {"f30",             30},
    {"f31",             31},
    // ----- mask registers --------------------------------------------------
    {"mt0",             0},
    {"mt1",             1},
    {"mt2",             2},
    {"mt3",             3},
    {"mt4",             4},
    {"mt5",             5},
    {"mt6",             6},
    {"mt7",             7},
    {"m0",              0},
    {"m1",              1},
    {"m2",              2},
    {"m3",              3},
    {"m4",              4},
    {"m5",              5},
    {"m6",              6},
    {"m7",              7},
    // ----- rounding modes --------------------------------------------------
    {"rne",             0},
    {"rtz",             1},
    {"rdn",             2},
    {"rup",             3},
    {"rmm",             4},
    {"dyn",             7},
    // ----- U-mode registers ------------------------------------------------
    {"ustatus",         csr_unknown},
    {"uie",             csr_unknown},
    {"utvec",           csr_unknown},
    {"uscratch",        csr_unknown},
    {"uepc",            csr_unknown},
    {"ucause",          csr_unknown},
    {"utval",           csr_unknown},
    {"uip",             csr_unknown},
    {"fflags",          csr_fflags},
    {"frm",             csr_frm},
    {"fcsr",            csr_fcsr},
    {"cycle",           csr_cycle},
    {"cycleh",          csr_unknown},
    {"time",            csr_unknown},
    {"timeh",           csr_unknown},
    {"instret",         csr_instret},
    {"instreth",        csr_unknown},
    {"hpmcounter3",     csr_unknown},
    {"hpmcounter4",     csr_unknown},
    {"hpmcounter5",     csr_unknown},
    {"hpmcounter6",     csr_unknown},
    {"hpmcounter7",     csr_unknown},
    {"hpmcounter8",     csr_unknown},
    {"hpmcounter9",     csr_unknown},
    {"hpmcounter10",    csr_unknown},
    {"hpmcounter11",    csr_unknown},
    {"hpmcounter12",    csr_unknown},
    {"hpmcounter13",    csr_unknown},
    {"hpmcounter14",    csr_unknown},
    {"hpmcounter15",    csr_unknown},
    {"hpmcounter16",    csr_unknown},
    {"hpmcounter17",    csr_unknown},
    {"hpmcounter18",    csr_unknown},
    {"hpmcounter19",    csr_unknown},
    {"hpmcounter20",    csr_unknown},
    {"hpmcounter21",    csr_unknown},
    {"hpmcounter22",    csr_unknown},
    {"hpmcounter23",    csr_unknown},
    {"hpmcounter24",    csr_unknown},
    {"hpmcounter25",    csr_unknown},
    {"hpmcounter26",    csr_unknown},
    {"hpmcounter27",    csr_unknown},
    {"hpmcounter28",    csr_unknown},
    {"hpmcounter29",    csr_unknown},
    {"hpmcounter30",    csr_unknown},
    {"hpmcounter31",    csr_unknown},
    // ----- U-mode ET registers ---------------------------------------------
    {"tensor_reduce",    csr_treduce},
    {"tensor_fma",       csr_tfmastart},
    {"tensor_conv_size", csr_tconvsize},
    {"tensor_conv_ctrl", csr_tconvctrl},
    {"tensor_coop",      csr_tcoop},
    {"tensor_mask",      csr_tmask},
    //"tensor_quant",
    {"tex_send",         csr_texsend},
    {"tensor_error",     csr_terror},
    {"scratchpad_ctrl",  csr_scpctrl},
    {"flb",              csr_flbarrier},
    {"flb0",             csr_flbarrier},
    {"fcc",              csr_fccounter},
    {"unknown_821",      csr_fccounter},
    {"usr_cache_op",     csr_ucacheop}, // TODO remove
    {"tensor_wait",      csr_twait},
    {"tensor_load",      csr_tloadctrl},
    //"gsc_progress"
    {"tensor_load_l2",   csr_tloadl2ctrl},
    {"tensor_store",     csr_tstore},
    {"validation0",      csr_validation0},
    {"validation1",      csr_validation1},
    {"validation2",      csr_validation2},
    {"validation3",      csr_validation3},
    {"unknown_cd0",      csr_hartid},
    {"unknown_8d5",      csr_sleep_txfma_27},
    {"umsg_port0",       csr_umsg_port0}, //TODO remove
    {"umsg_port1",       csr_umsg_port1}, //TODO remove
    {"umsg_port2",       csr_umsg_port2}, //TODO remove
    {"umsg_port3",       csr_umsg_port3}, //TODO remove
    {"porthead0",        csr_porthead0},
    {"porthead1",        csr_porthead1},
    {"porthead2",        csr_porthead2},
    {"porthead3",        csr_porthead3},
    {"portheadnb0",      csr_portheadnb0},
    {"portheadnb1",      csr_portheadnb1},
    {"portheadnb2",      csr_portheadnb2},
    {"portheadnb3",      csr_portheadnb3},
    {"evict_va",         csr_evict_va},
    {"flush_va",         csr_flush_va},
    {"lock_va",          csr_lock_va},
    {"unlock_va",        csr_unlock_va},
    {"prefetch_va",      csr_prefetch_va},
    // ----- S-mode registers ------------------------------------------------
    {"sstatus",          csr_sstatus},
    {"sedeleg",          csr_unknown},
    {"sideleg",          csr_unknown},
    {"sie",              csr_sie},
    {"stvec",            csr_stvec},
    {"scounteren",       csr_scounteren},
    {"sscratch",         csr_sscratch},
    {"sepc",             csr_sepc},
    {"scause",           csr_scause},
    {"stval",            csr_stval},
    {"sip",              csr_sip},
    {"satp",             csr_satp},
    // ----- S-mode ET registers ---------------------------------------------
    {"sys_cache_op",     csr_scacheop}, // TODO remove
    {"evict_sw",         csr_evict_sw},
    {"flush_sw",         csr_flush_sw},
    {"portctrl0",        csr_portctrl0},
    {"portctrl1",        csr_portctrl1},
    {"portctrl2",        csr_portctrl2},
    {"portctrl3",        csr_portctrl3},
    {"smsg_port0",       csr_portctrl0}, //TODO remove
    {"smsg_port1",       csr_portctrl1}, //TODO remove
    {"smsg_port2",       csr_portctrl2}, //TODO remove
    {"smsg_port3",       csr_portctrl3}, //TODO remove
    // ----- M-mode registers ------------------------------------------------
    {"mvendorid",        csr_mvendorid},
    {"marchid",          csr_marchid},
    {"mimpid",           csr_mimpid},
    {"mhartid",          csr_mhartid},
    {"mstatus",          csr_mstatus},
    {"misa",             csr_misa},
    {"medeleg",          csr_medeleg},
    {"mideleg",          csr_mideleg},
    {"mie",              csr_mie},
    {"mtvec",            csr_mtvec},
    {"mcounteren",       csr_mcounteren},
    {"mscratch",         csr_mscratch},
    {"mepc",             csr_mepc},
    {"mcause",           csr_mcause},
    {"mtval",            csr_mtval},
    {"mip",              csr_mip},
    {"pmpcfg0",          csr_unknown},
    {"pmpcfg1",          csr_unknown},
    {"pmpcfg2",          csr_unknown},
    {"pmpcfg3",          csr_unknown},
    {"pmpaddr0",         csr_unknown},
    {"pmpaddr1",         csr_unknown},
    {"pmpaddr2",         csr_unknown},
    {"pmpaddr3",         csr_unknown},
    {"pmpaddr4",         csr_unknown},
    {"pmpaddr5",         csr_unknown},
    {"pmpaddr6",         csr_unknown},
    {"pmpaddr7",         csr_unknown},
    {"pmpaddr8",         csr_unknown},
    {"pmpaddr9",         csr_unknown},
    {"pmpaddr10",        csr_unknown},
    {"pmpaddr11",        csr_unknown},
    {"pmpaddr12",        csr_unknown},
    {"pmpaddr13",        csr_unknown},
    {"pmpaddr14",        csr_unknown},
    {"pmpaddr15",        csr_unknown},
    {"mcycle",           csr_mcycle},
    {"mcycleh",          csr_unknown},
    {"minstret",         csr_minstret},
    {"minstreth",        csr_unknown},
    {"mhpmcounter3",     csr_unknown},
    {"mhpmcounter4",     csr_unknown},
    {"mhpmcounter5",     csr_unknown},
    {"mhpmcounter6",     csr_unknown},
    {"mhpmcounter7",     csr_unknown},
    {"mhpmcounter8",     csr_unknown},
    {"mhpmcounter9",     csr_unknown},
    {"mhpmcounter10",    csr_unknown},
    {"mhpmcounter11",    csr_unknown},
    {"mhpmcounter12",    csr_unknown},
    {"mhpmcounter13",    csr_unknown},
    {"mhpmcounter14",    csr_unknown},
    {"mhpmcounter15",    csr_unknown},
    {"mhpmcounter16",    csr_unknown},
    {"mhpmcounter17",    csr_unknown},
    {"mhpmcounter18",    csr_unknown},
    {"mhpmcounter19",    csr_unknown},
    {"mhpmcounter20",    csr_unknown},
    {"mhpmcounter21",    csr_unknown},
    {"mhpmcounter22",    csr_unknown},
    {"mhpmcounter23",    csr_unknown},
    {"mhpmcounter24",    csr_unknown},
    {"mhpmcounter25",    csr_unknown},
    {"mhpmcounter26",    csr_unknown},
    {"mhpmcounter27",    csr_unknown},
    {"mhpmcounter28",    csr_unknown},
    {"mhpmcounter29",    csr_unknown},
    {"mhpmcounter30",    csr_unknown},
    {"mhpmcounter31",    csr_unknown},
    {"mhpmevent3",       csr_unknown},
    {"mhpmevent4",       csr_unknown},
    {"mhpmevent5",       csr_unknown},
    {"mhpmevent6",       csr_unknown},
    {"mhpmevent7",       csr_unknown},
    {"mhpmevent8",       csr_unknown},
    {"mhpmevent9",       csr_unknown},
    {"mhpmevent10",      csr_unknown},
    {"mhpmevent11",      csr_unknown},
    {"mhpmevent12",      csr_unknown},
    {"mhpmevent13",      csr_unknown},
    {"mhpmevent14",      csr_unknown},
    {"mhpmevent15",      csr_unknown},
    {"mhpmevent16",      csr_unknown},
    {"mhpmevent17",      csr_unknown},
    {"mhpmevent18",      csr_unknown},
    {"mhpmevent19",      csr_unknown},
    {"mhpmevent20",      csr_unknown},
    {"mhpmevent21",      csr_unknown},
    {"mhpmevent22",      csr_unknown},
    {"mhpmevent23",      csr_unknown},
    {"mhpmevent24",      csr_unknown},
    {"mhpmevent25",      csr_unknown},
    {"mhpmevent26",      csr_unknown},
    {"mhpmevent27",      csr_unknown},
    {"mhpmevent28",      csr_unknown},
    {"mhpmevent29",      csr_unknown},
    {"mhpmevent30",      csr_unknown},
    {"mhpmevent31",      csr_unknown},
    // ----- debug registers -----------
    {"tselect",          csr_unknown},
    {"tdata1",           csr_unknown},
    {"tdata2",           csr_unknown},
    {"tdata3",           csr_unknown},
    {"tinfo",            csr_unknown},
    {"tcontrol",         csr_unknown},
    {"mcontrol",         csr_unknown}, // alias for tdata1
    {"icount",           csr_unknown}, // alias for tdata1
    {"itrigger",         csr_unknown}, // alias for tdata1
    {"etrigger",         csr_unknown}, // alias for tdata1
    {"dcsr",             csr_unknown},
    {"dpc",              csr_unknown},
    {"dscratch0",        csr_unknown},
    {"dscratch1",        csr_unknown},
    // ----- M-mode ET registers ---------------------------------------------
    {"minstmask",        csr_minstmask},
    {"minstmatch",       csr_minstmatch},
    //"amofence_ctrl"
    {"flush_icache",     csr_flush_icache},
    {"sleep_txfma_27",   csr_msleep_txfma_27}, //TODO: change string to 'msleep_txfma_27' when tools are updated
    {"unknown_7d1",      csr_msleep_txfma_27},
    {"unknown_7d2",      csr_menable_shadows},
});

// Returns the pointer to a function based on name
static func_ptr get_function_ptr(std::string func)
{
    emu_ptr_hash_t::const_iterator el = pointer_cache.find(func);
    if (el != pointer_cache.end())
        return el->second;
   
    LOG(WARN, "Unknown opcode '%s' while decoding instruction.",func.c_str());
    return func_ptr(unknown);
}

// Sets the mnemonic. It also starts decoding it to generate
// the emulation routine
void instruction::set_mnemonic(std::string mnemonic_)
{
    mnemonic = mnemonic_;
    mnemonic.erase(std::remove(mnemonic.begin(), mnemonic.end(), '\n'), mnemonic.end());

    boost::regex e("^([^\\s]+)(.*)");
    boost::smatch m;
    std::string tmp = mnemonic;

    // First separate opcode from arguments
    boost::regex_search(tmp, m, e);
    std::string opcode = m[1];
    std::string args = m[2];

    if (opcode == "")
    {
        str_error = "Failed extracting opcode from instruction mnemonic";
        return;
    }

    // Change any "." in the ocpode name to underscore
    e = boost::regex("\\.");
    opcode = boost::regex_replace(opcode, e, std::string("_"));

    // Gets if the instruction is a load
    if ((opcode == "ld") || (opcode == "lw") || (opcode == "lwu") || (opcode == "lh") || (opcode == "lhu") || (opcode == "lb") || (opcode == "lbu"))
        is_load = true;

    // Gets if the instruction is a floating point load
    if ((opcode == "flw") || (opcode == "flw_ps"))
        is_fpload = true;

    if (opcode == "wfi")
        is_wfi = true;

    if (opcode=="texrcv")
      is_texrcv=true;

    if (opcode=="texsndh")
      is_texsndh=true;

    if ((opcode=="frsq_ps") || (opcode=="flog_ps") || (opcode=="fexp_ps") || (opcode=="fsin_ps"))
        is_1ulp = true;

    if (   (opcode=="amoswap_d") || (opcode=="amoadd_d") || (opcode=="amoxor_d") || (opcode=="amoand_d")
        || (opcode=="amoor_d")   || (opcode=="amomin_d") || (opcode=="amomax_d") || (opcode=="amominu_d")
        || (opcode=="amomaxu_d"))
        is_amo = true;

    if (opcode.find("csrr") != std::string::npos) {
        is_csr_read = true;
    }

    // Cleanup whitespace from $args
    e = boost::regex("\\s");
    args = boost::regex_replace(args, e, std::string(""));

    // Convert load and store addresses of the form off(xreg) into "off,xreg"
    e = boost::regex("(\\d+)\\((.*)\\)");
    args = boost::regex_replace(args, e, std::string("$1,$2"));

    // Convert amo operations from (reg) to reg
    e = boost::regex("\\((.*)\\)");
    args = boost::regex_replace(args, e, std::string("$1"));

    // Remove pc
    e = boost::regex("pc\\+");
    args = boost::regex_replace(args, e, std::string(""));

    // Remove pc except for csr operations
    if (!boost::regex_match(opcode, boost::regex("csr.*")))
    {
        e = boost::regex("pc");
        args = boost::regex_replace(args, e, std::string(""));
    }

    // Converts to vector
    std::vector<std::string> arg_array;

    e = boost::regex("^([^,]+),");
    while(boost::regex_search(args, m, e))
    {
        arg_array.push_back(m[1]);
        args = boost::regex_replace(args, e, std::string(""));
    }
    if (args != "")
        arg_array.push_back(args);

    // Pseudoinstructions (convert to another instruction).
    // From riscv spec Chapter 21
    // Integer
    if (opcode == "li")
    {
        opcode = "addi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    if (opcode == "nop")
    {
        opcode = "addi";
        arg_array.push_back("x0");
        arg_array.push_back("x0");
        arg_array.push_back("0");
    }
    else if (opcode == "mv")
    {
        opcode = "addi";
        arg_array.push_back("0");
    }
    else if (opcode == "not")
    {
        opcode = "xori";
        arg_array.push_back("-1");
    }
    else if (opcode == "neg")
    {
        opcode = "sub";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "negw")
    {
        opcode = "subw";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "sext_w")
    {
        opcode = "addiw";
        arg_array.push_back("x0");
    }
    else if (opcode == "seqz")
    {
        opcode = "sltiu";
        arg_array.push_back("1");
    }
    else if (opcode == "snez")
    {
        opcode = "sltu";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "sltz")
    {
        opcode = "slt";
        arg_array.push_back("x0");
    }
    else if (opcode == "sgtz")
    {
        opcode = "slt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    // FP
    else if (opcode == "fmv_s")
    {
        opcode = "fsgnj_s";
        arg_array.push_back(arg_array[1]);
    }
    else if (opcode == "fabs_s")
    {
        opcode = "fsgnjx_s";
        arg_array.push_back(arg_array[1]);
    }
    else if (opcode == "fneg_s")
    {
        opcode = "fsgnjn_s";
        arg_array.push_back(arg_array[1]);
    }
    else if (opcode == "fmv_x_s")
    {
        opcode = "fmv_x_w";
    }
    else if (opcode == "fmv_s_x")
    {
        opcode = "fmv_w_x";
    }
    // Branches
    else if (opcode == "beqz")
    {
        opcode = "beq";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "bnez")
    {
        opcode = "bne";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "blez")
    {
        opcode = "bge";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "bgez")
    {
        opcode = "bge";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "bltz")
    {
        opcode = "blt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if (opcode == "bgtz")
    {
        opcode = "blt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    // Jumps
    else if (opcode == "j")
    {
        opcode = "jal";
        arg_array.push_back(arg_array[0]);
        arg_array[0] = "x0";
    }
    else if (opcode == "jal")
    {
        // If one argument means that link register dest is implicit and is x1
        if (arg_array.size() == 1)
        {
            arg_array.push_back(arg_array[0]);
            arg_array[0] = "x1";
        }
    }
    else if (opcode == "jr")
    {
        opcode = "jalr";
        arg_array.push_back(arg_array[0]);
        arg_array[0] = "x0";
        arg_array.push_back("0");
    }
    else if (opcode == "jalr")
    {
        // If one argument means that link register dest is implicit and is x1 and offset is 0
        if (arg_array.size() == 1)
        {
            arg_array.push_back(arg_array[0]);
            arg_array[0] = "x1";
            arg_array.push_back("0");
        }
        // If two arguments means that link register dest is implicit and is x1
        if (arg_array.size() == 2)
        {
            arg_array.push_back(arg_array[1]);
            arg_array[1] = arg_array[0];
            arg_array[0] = "x1";
        }
    }
    else if (opcode == "ret")
    {
        opcode = "jalr";
        arg_array.push_back("x0");
        arg_array.push_back("x1");
        arg_array.push_back("0");
    }
    // CSRs
    else if (opcode == "csrr")
    {
        opcode = "csrrs";
        arg_array.push_back("x0");
    }
    else if (opcode == "csrw")
    {
        opcode = "csrrw";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "csrs")
    {
        opcode = "csrrs";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "csrc")
    {
        opcode = "csrrc";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "csrwi")
    {
        opcode = "csrrwi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "csrsi")
    {
        opcode = "csrrsi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (opcode == "csrci")
    {
        opcode = "csrrci";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if (!is_fpload && opcode[0] == 'f')
    {
        // Add implicit rounding mode operand to floating-point operations
        if (  (arg_array.size() == 2 && rm2args.find(opcode) != rm2args.end())
           || (arg_array.size() == 3 && rm3args.find(opcode) != rm3args.end())
           || (arg_array.size() == 4 && rm4args.find(opcode) != rm4args.end()))
        {
            arg_array.push_back("dyn");
        }
    }

    // JALR has different behaviour for compressed
    if (is_compressed && (opcode == "jalr"))
    {
        opcode = "c_jalr";
    }
    if (is_compressed && (opcode == "jal"))
    {
        opcode = "c_jal";
    }

    // Stores the arguments
    for(auto a:arg_array)
        add_parameter(a);

    // 0x0000 is an illegal instruction, but spike-dasm returns "addi s0, sp0, 0"
    // 0xffffffff is an illegal instruction, and spike-dasm returns "unknown"
    if (enc_bits == 0)
    {
        opcode = "unknown";
        num_params = 0;
    }

    // Checks if it is a tensor/reduce operation
    if ((opcode == "csrrw") || (opcode == "cssrwi"))
    {
        is_reduce      = (params[1] == csr_treduce);
        is_tensor_load = (params[1] == csr_tloadctrl);
        is_tensor_fma  = (params[1] == csr_tfmastart);
        is_flb         = (params[1] == csr_flbarrier);
        is_fcc         = (params[1] == csr_fccounter);
    }

    // Get the emulation function pointer for the opcode
    if (str_error.empty())
    {
        emu_func = get_function_ptr(opcode);
        emu_func0 = (func_ptr_0) emu_func;
        emu_func1 = (func_ptr_1) emu_func;
        emu_func2 = (func_ptr_2) emu_func;
        emu_func3 = (func_ptr_3) emu_func;
        emu_func4 = (func_ptr_4) emu_func;
        emu_func5 = (func_ptr_5) emu_func;
    }
    
}

// Instruction execution
void instruction::exec()
{
    // If instruction had an error during decoding, report it when it is executed
    if (!str_error.empty())
        throw std::runtime_error(str_error);

    // Check if we should trap the instruction to microcode
    check_minst_match(get_enc());

    switch (num_params)
    {
        case 0: (emu_func0(nullptr)); break;
        case 1: (emu_func1(params[0], nullptr)); break;
        case 2: (emu_func2(params[0], params[1], nullptr)); break;
        case 3: (emu_func3(params[0], params[1], params[2], nullptr)); break;
        case 4: (emu_func4(params[0], params[1], params[2], params[3], nullptr)); break;
        case 5: (emu_func5(params[0], params[1], params[2], params[3], params[4], nullptr)); break;
    }
}

// Adds a parameter
void instruction::add_parameter(std::string param)
{
    // Args unknown
    if (param == "argsunknown")
    {
        // Weird case seen in WFI where despite having no parameter the disasm returns this parameter
        return;
    }
    // Check for the usual suspects
    emu_opnd_hash_t::const_iterator el = operand_cache.find(param);
    if (el != operand_cache.end())
    {
        params[num_params] = el->second;
    }
    // Hex constant
    else if (param.find("0x") != std::string::npos)
    {
        // Negative
        bool neg = false;
        if (param[0] == '-')
        {
            param.erase(0, 1);
            neg = true;
        }
        int c = sscanf(param.c_str(), "0x%X", &params[num_params]);
        if (c != 1)
        {
            str_error = "Error parsing parameter " + param + ". Expecting hex immediate";
        }
        if (neg)
            params[num_params] = -params[num_params];
    }
    // Dec constant
    else if (boost::regex_match(param, boost::regex("^-?[[:d:]]+")))
    {
        int c = sscanf(param.c_str(), "%i", &params[num_params]);
        if (c != 1)
        {
            str_error = "Error parsing parameter " + param + ". Expecting dec immediate";
        }
    }
    // Unknown CSR
    else if (param.find("unknown_") !=std::string::npos)
    {
        params[num_params] = csr_unknown;
    }
    else
    {
       str_error = "Unknown parameter " + param + ".";
    }
    num_params++;
}
