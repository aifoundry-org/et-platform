// Local
#include "instruction.h"
#include "emu.h"

// STD
#include <iostream>
#include <boost/regex.hpp>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// Typedef for the hash of functions
typedef std::unordered_map<std::string, func_ptr> emu_ptr_hash_t;

// Map of mnemonic string to function pointer
static const emu_ptr_hash_t pointer_cache({
// ----- RV64I -----------------------------------------------------------------
    {"beq",     func_ptr(beq)},
    {"bne",     func_ptr(bne)},
    {"blt",     func_ptr(blt)},
    {"bge",     func_ptr(bge)},
    {"bltu",    func_ptr(bltu)},
    {"bgeu",    func_ptr(bgeu)},
    {"jalr",    func_ptr(jalr)},
    {"jal",     func_ptr(jal)},
    {"lui",     func_ptr(lui)},
    {"auipc",   func_ptr(auipc)},
    {"addi",    func_ptr(addi)},
    {"slli",    func_ptr(slli)},
    {"slti",    func_ptr(slti)},
    {"sltiu",   func_ptr(sltiu)},
    {"xori",    func_ptr(xori)},
    {"srli",    func_ptr(srli)},
    {"srai",    func_ptr(srai)},
    {"ori",     func_ptr(ori)},
    {"andi",    func_ptr(andi)},
    {"add",     func_ptr(add)},
    {"sub",     func_ptr(sub)},
    {"sll",     func_ptr(sll)},
    {"slt",     func_ptr(slt)},
    {"sltu",    func_ptr(sltu)},
    {"xor",     func_ptr(xor_)},
    {"srl",     func_ptr(srl)},
    {"sra",     func_ptr(sra)},
    {"or",      func_ptr(or_)},
    {"and",     func_ptr(and_)},
    {"addiw",   func_ptr(addiw)},
    {"slliw",   func_ptr(slliw)},
    {"srliw",   func_ptr(srliw)},
    {"sraiw",   func_ptr(sraiw)},
    {"addw",    func_ptr(addw)},
    {"subw",    func_ptr(subw)},
    {"sllw",    func_ptr(sllw)},
    {"srlw",    func_ptr(srlw)},
    {"sraw",    func_ptr(sraw)},
    {"lb",      func_ptr(lb)},
    {"lh",      func_ptr(lh)},
    {"lw",      func_ptr(lw)},
    {"ld",      func_ptr(ld)},
    {"lbu",     func_ptr(lbu)},
    {"lhu",     func_ptr(lhu)},
    {"lwu",     func_ptr(lwu)},
    {"sd",      func_ptr(sd)},
    {"sw",      func_ptr(sw)},
    {"sh",      func_ptr(sh)},
    {"sb",      func_ptr(sb)},
    {"fence",   func_ptr(fence)},
    {"fence_i", func_ptr(fence_i)},
// ----- RV32M -----------------------------------------------------------------
    {"mul",     func_ptr(mul)},
    {"mulh",    func_ptr(mulh)},
//  {"mulhsu",  func_ptr(mulhsu)},
    {"mulhu",   func_ptr(mulhu)},
    {"div",     func_ptr(div_)},
    {"divu",    func_ptr(divu)},
    {"rem",     func_ptr(rem)},
    {"remu",    func_ptr(remu)},
// ----- RV64M -----------------------------------------------------------------
    {"mulw",    func_ptr(mulw)},
    {"divw",    func_ptr(divw)},
    {"divuw",   func_ptr(divuw)},
    {"remw",    func_ptr(remw)},
    {"remuw",   func_ptr(remuw)},
// ----- RV32A -----------------------------------------------------------------
    {"amoadd_w",        func_ptr(amoadd_w)},
    {"amoxor_w",        func_ptr(amoxor_w)},
    {"amoor_w",         func_ptr(amoor_w)},
    {"amoand_w",        func_ptr(amoand_w)},
    {"amomin_w",        func_ptr(amomin_w)},
    {"amomax_w",        func_ptr(amomax_w)},
    {"amominu_w",       func_ptr(amominu_w)},
    {"amomaxu_w",       func_ptr(amomaxu_w)},
    {"amoswap_w",       func_ptr(amoswap_w)},
//  {"lr_w",            func_ptr(lr_w)},
//  {"sc_w",            func_ptr(sc_w)},
// ----- RV64A -----------------------------------------------------------------
    {"amoadd_d",        func_ptr(amoadd_d)},
    {"amoxor_d",        func_ptr(amoxor_d)},
    {"amoor_d",         func_ptr(amoor_d)},
    {"amoand_d",        func_ptr(amoand_d)},
    {"amomin_d",        func_ptr(amomin_d)},
    {"amomax_d",        func_ptr(amomax_d)},
    {"amominu_d",       func_ptr(amominu_d)},
    {"amomaxu_d",       func_ptr(amomaxu_d)},
    {"amoswap_d",       func_ptr(amoswap_d)},
//  {"lr_d",            func_ptr(lr_d)},
//  {"sc_d",            func_ptr(sc_d)},
// ----- SYSTEM ----------------------------------------------------------------
    {"ecall",           func_ptr(ecall)},
    {"ebreak",          func_ptr(ebreak)},
//  {"uret",            func_ptr(uret)},
    {"sret",            func_ptr(sret)},
    {"mret",            func_ptr(mret)},
//  {"dret",            func_ptr(dret)},
//  {"sfence_vma",      func_ptr(sfence_vma)},
    {"wfi",             func_ptr(wfi)},
    {"csrrw",           func_ptr(csrrw)},
    {"csrrs",           func_ptr(csrrs)},
    {"csrrc",           func_ptr(csrrc)},
    {"csrrwi",          func_ptr(csrrwi)},
    {"csrrsi",          func_ptr(csrrsi)},
    {"csrrci",          func_ptr(csrrci)},
// ----- RV64C -----------------------------------------------------------------
//  {"c_jalr",  func_ptr(c_jalr)},
// ----- RV64F -----------------------------------------------------------------
    {"fadd_s",          func_ptr(fadd_s)},
    {"fsub_s",          func_ptr(fsub_s)},
    {"fmul_s",          func_ptr(fmul_s)},
    {"fdiv_s",          func_ptr(fdiv_s)},
    {"fsgnj_s",         func_ptr(fsgnj_s)},
    {"fsgnjn_s",        func_ptr(fsgnjn_s)},
    {"fsgnjx_s",        func_ptr(fsgnjx_s)},
    {"fmin_s",          func_ptr(fmin_s)},
    {"fmax_s",          func_ptr(fmax_s)},
    {"fsqrt_s",         func_ptr(fsqrt_s)},
    {"feq_s",           func_ptr(feq_s)},
    {"fle_s",           func_ptr(fle_s)},
    {"flt_s",           func_ptr(flt_s)},
    {"fcvt_w_s",        func_ptr(fcvt_w_s)},
    {"fcvt_wu_s",       func_ptr(fcvt_wu_s)},
//  {"fcvt_l_s",        func_ptr(fcvt_l_s)},
//  {"fcvt_lu_s",       func_ptr(fcvt_lu_s)},
    {"fmv_x_w",         func_ptr(fmv_x_w)},
    {"fmv_x_s",         func_ptr(fmv_x_w)},     // old name for fmv_x_w
    {"fclass_s",        func_ptr(fclass_s)},
    {"fcvt_s_w",        func_ptr(fcvt_s_w)},
    {"fcvt_s_wu",       func_ptr(fcvt_s_wu)},
//  {"fcvt_s_l",        func_ptr(fcvt_s_l)},
//  {"fcvt_s_lu",       func_ptr(fcvt_s_lu)},
    {"fmv_w_x",         func_ptr(fmv_w_x)},
    {"fmv_s_x",         func_ptr(fmv_w_x)},     // old name for fmv_w_x
    {"flw",             func_ptr(flw)},
    {"fsw",             func_ptr(fsw)},
    {"fmadd_s",         func_ptr(fmadd_s)},
    {"fmsub_s",         func_ptr(fmsub_s)},
    {"fnmsub_s",        func_ptr(fnmsub_s)},
    {"fnmadd_s",        func_ptr(fnmadd_s)},
// ----- Esperanto mask extension ----------------------------------------------
    {"maskand",         func_ptr(maskand)},
    {"maskor",          func_ptr(maskor)},
    {"maskxor",         func_ptr(maskxor)},
    {"masknot",         func_ptr(masknot)},
    {"mova_x_m",        func_ptr(mova_x_m)},
    {"mova_m_x",        func_ptr(mova_m_x)},
    {"mov_m_x",         func_ptr(mov_m_x)},
    {"maskpopc",        func_ptr(maskpopc)},
    {"maskpopcz",       func_ptr(maskpopcz)},
    {"maskpopc_rast",   func_ptr(maskpopc_rast)},
// ----- Esperanto packed-single extension -------------------------------------
    {"flw_ps",          func_ptr(flw_ps)},
    {"flq",             func_ptr(flq)},
    {"fsw_ps",          func_ptr(fsw_ps)},
//  {"fswpc_ps",        func_ptr(fswpc_ps)},
    {"fsq",             func_ptr(fsq)},
    {"fbc_ps",          func_ptr(fbc_ps)},
    {"fbci_ps",         func_ptr(fbci_ps)},
    {"fbcx_ps",         func_ptr(fbcx_ps)},
    {"fgw_ps",          func_ptr(fgw_ps)},
    {"fgh_ps",          func_ptr(fgh_ps)},
    {"fgb_ps",          func_ptr(fgb_ps)},
    {"fscw_ps",         func_ptr(fscw_ps)},
    {"fsch_ps",         func_ptr(fsch_ps)},
    {"fscb_ps",         func_ptr(fscb_ps)},
    {"fg32b_ps",        func_ptr(fg32b_ps)},
    {"fg32h_ps",        func_ptr(fg32h_ps)},
    {"fg32w_ps",        func_ptr(fg32w_ps)},
    {"fsc32b_ps",       func_ptr(fsc32b_ps)},
    {"fsc32h_ps",       func_ptr(fsc32h_ps)},
    {"fsc32w_ps",       func_ptr(fsc32w_ps)},
    {"fadd_ps",         func_ptr(fadd_ps)},
    {"fsub_ps",         func_ptr(fsub_ps)},
    {"fmul_ps",         func_ptr(fmul_ps)},
    {"fdiv_ps",         func_ptr(fdiv_ps)},
    {"fsgnj_ps",        func_ptr(fsgnj_ps)},
    {"fsgnjn_ps",       func_ptr(fsgnjn_ps)},
    {"fsgnjx_ps",       func_ptr(fsgnjx_ps)},
    {"fmin_ps",         func_ptr(fmin_ps)},
    {"fmax_ps",         func_ptr(fmax_ps)},
    {"fsqrt_ps",        func_ptr(fsqrt_ps)},
    {"feq_ps",          func_ptr(feq_ps)},
    {"fle_ps",          func_ptr(fle_ps)},
    {"flt_ps",          func_ptr(flt_ps)},
    {"feqm_ps",         func_ptr(feqm_ps)},
    {"flem_ps",         func_ptr(flem_ps)},
    {"fltm_ps",         func_ptr(fltm_ps)},
    {"fsetm_ps",        func_ptr(fsetm_ps)},
    {"fcmov_ps",        func_ptr(fcmov_ps)},
    {"fcmovm_ps",       func_ptr(fcmovm_ps)},
    {"fmvz_x_ps",       func_ptr(fmvz_x_ps)},
    {"fmvs_x_ps",       func_ptr(fmvs_x_ps)},
    {"fswizz_ps",       func_ptr(fswizz_ps)},
    {"fcvt_pw_ps",      func_ptr(fcvt_pw_ps)},
    {"fcvt_pwu_ps",     func_ptr(fcvt_pwu_ps)},
//    fcvt_pl_ps
//    fcvt_plu_ps
    {"fclass_ps",       func_ptr(fclass_ps)},
    {"fcvt_ps_pw",      func_ptr(fcvt_ps_pw)},
    {"fcvt_ps_pwu",     func_ptr(fcvt_ps_pwu)},
//    fcvt_ps_pl
//    fcvt_ps_plu
//    fmv_ps_px -- equivalent to a X-reg broadcast to V-reg
    {"fmadd_ps",        func_ptr(fmadd_ps)},
    {"fmsub_ps",        func_ptr(fmsub_ps)},
    {"fnmsub_ps",       func_ptr(fnmsub_ps)},
    {"fnmadd_ps",       func_ptr(fnmadd_ps)},
    {"fcvt_ps_f16",     func_ptr(fcvt_ps_f16)},
    {"fcvt_ps_un24",    func_ptr(fcvt_ps_un24)},
    {"fcvt_ps_un16",    func_ptr(fcvt_ps_un16)},
    {"fcvt_ps_un10",    func_ptr(fcvt_ps_un10)},
    {"fcvt_ps_un8",     func_ptr(fcvt_ps_un8)},
    {"fcvt_ps_un2",     func_ptr(fcvt_ps_un2)},
    {"fcvt_ps_sn16",    func_ptr(fcvt_ps_sn16)},
    {"fcvt_ps_sn8",     func_ptr(fcvt_ps_sn8)},
    {"fcvt_ps_f11",     func_ptr(fcvt_ps_f11)},
    {"fcvt_ps_f10",     func_ptr(fcvt_ps_f10)},
    {"fcvt_f16_ps",     func_ptr(fcvt_f16_ps)},
    {"fcvt_un24_ps",    func_ptr(fcvt_un24_ps)},
    {"fcvt_un16_ps",    func_ptr(fcvt_un16_ps)},
    {"fcvt_un10_ps",    func_ptr(fcvt_un10_ps)},
    {"fcvt_un8_ps",     func_ptr(fcvt_un8_ps)},
    {"fcvt_un2_ps",     func_ptr(fcvt_un2_ps)},
    {"fcvt_sn16_ps",    func_ptr(fcvt_sn16_ps)},
    {"fcvt_sn8_ps",     func_ptr(fcvt_sn8_ps)},
    {"fcvt_f11_ps",     func_ptr(fcvt_f11_ps)},
    {"fcvt_f10_ps",     func_ptr(fcvt_f10_ps)},
    {"fsin_ps",         func_ptr(fsin_ps)},
    {"fexp_ps",         func_ptr(fexp_ps)},
    {"flog_ps",         func_ptr(flog_ps)},
    {"ffrc_ps",         func_ptr(ffrc_ps)},
    {"fround_ps",       func_ptr(fround_ps)},
    {"frcp_ps",         func_ptr(frcp_ps)},
    {"frsq_ps",         func_ptr(frsq_ps)},
//    {"fltabs_ps",     func_ptr(fltabs_ps)},
    {"cubeface_ps",     func_ptr(cubeface_ps)},
    {"cubefaceidx_ps",  func_ptr(cubefaceidx_ps)},
    {"cubesgnsc_ps",    func_ptr(cubesgnsc_ps)},
    {"cubesgntc_ps",    func_ptr(cubesgntc_ps)},
    {"fcvt_ps_rast",    func_ptr(fcvt_ps_rast)},
    {"fcvt_rast_ps",    func_ptr(fcvt_rast_ps)},
    {"frcp_fix_rast",   func_ptr(frcp_fix_rast)},
// NB: these are scalar instructions!
    {"packb",           func_ptr(packb)},
    {"bitmixb",         func_ptr(bitmixb)},
// FIXME: THIS INSTRUCTION IS OBSOLETE
    {"frcpfxp_ps",      func_ptr(frcpfxp_ps)},
// Texture sampling -- FIXME: THESE INSTRUCTIONS ARE OBSOLETE
    {"texsndh",         func_ptr(texsndh)},
    {"texsnds",         func_ptr(texsnds)},
    {"texsndt",         func_ptr(texsndt)},
    {"texsndr",         func_ptr(texsndr)},
    {"texrcv",          func_ptr(texrcv)},
// ----- Esperanto packed-integer extension ------------------------------------
    {"fbci_pi",         func_ptr(fbci_pi)},
    {"feq_pi",          func_ptr(feq_pi)},   // RV64I: beq
//  {"fne_pi",          func_ptr(fne_pi)},   // RV64I: bne
    {"fle_pi",          func_ptr(fle_pi)},   // RV64I: bge
    {"flt_pi",          func_ptr(flt_pi)},   // RV64I: blt
//  {"fleu_pi",         func_ptr(fleu_pi)},  // RV64I: bgeu
    {"fltu_pi",         func_ptr(fltu_pi)},  // RV64I: bltu
//  {"feqm_pi",         func_ptr(feqm_pi)},
//  {"fnem_pi",         func_ptr(fnem_pi)},
//  {"flem_pi",         func_ptr(flem_pi)},
    {"fltm_pi",         func_ptr(fltm_pi)},
//  {"fleum_pi",        func_ptr(fleum_pi)},
//  {"fltum_pi",        func_ptr(fltum_pi)},
    {"faddi_pi",        func_ptr(faddi_pi)},
    {"fslli_pi",        func_ptr(fslli_pi)},
//  {"fslti_pi",        func_ptr(fslti_pi)},
//  {"fsltiu_pi",       func_ptr(fsltiu_pi)},
    {"fxori_pi",        func_ptr(fxori_pi)},
    {"fsrli_pi",        func_ptr(fsrli_pi)},
    {"fsrai_pi",        func_ptr(fsrai_pi)},
    {"fori_pi",         func_ptr(fori_pi)},
    {"fandi_pi",        func_ptr(fandi_pi)},
    {"fadd_pi",         func_ptr(fadd_pi)},
    {"fsub_pi",         func_ptr(fsub_pi)},
    {"fsll_pi",         func_ptr(fsll_pi)},
//  {"fslt_pi",         func_ptr(fslt_pi)},
//  {"fsltu_pi",        func_ptr(fsltu_pi)},
    {"fxor_pi",         func_ptr(fxor_pi)},
    {"fsrl_pi",         func_ptr(fsrl_pi)},
    {"fsra_pi",         func_ptr(fsra_pi)},
    {"for_pi",          func_ptr(for_pi)},
    {"fand_pi",         func_ptr(fand_pi)},
    {"fnot_pi",         func_ptr(fnot_pi)},
    {"fsat8_pi",        func_ptr(fsat8_pi)},
    {"fpackreph_pi",    func_ptr(fpackreph_pi)},
    {"fpackrepb_pi",    func_ptr(fpackrepb_pi)},
    {"fmul_pi",         func_ptr(fmul_pi)},
    {"fmulh_pi",        func_ptr(fmulh_pi)},
// fmulhsu_pi will not be implemented
    {"fmulhu_pi",       func_ptr(fmulhu_pi)},
    {"fdiv_pi",         func_ptr(fdiv_pi)},
    {"fdivu_pi",        func_ptr(fdivu_pi)},
    {"frem_pi",         func_ptr(frem_pi)},
    {"fremu_pi",        func_ptr(fremu_pi)},
    {"fmin_pi",         func_ptr(fmin_pi)},
    {"fmax_pi",         func_ptr(fmax_pi)},
    {"fminu_pi",        func_ptr(fminu_pi)},
    {"fmaxu_pi",        func_ptr(fmaxu_pi)},
// ----- Esperanto atomic extension --------------------------------------------
// ----- Esperanto cache control extension -------------------------------------
// ----- Esperanto messaging extension -----------------------------------------
// ----- Esperanto tensor extension --------------------------------------------
// ----- Esperanto fast local barrier extension --------------------------------
// ----- Illegal instruction ---------------------------------------------------
    {"unknown",         func_ptr(unknown)},
});

// floating-point instructions with optional rounding-mode as 3rd operand
static const std::unordered_set<std::string> rm2args({
    "fsqrt_s",
    "fcvt_w_s", "fcvt_wu_s", "fcvt_l_s", "fcvt_lu_s",
    "fcvt_s_w", "fcvt_s_wu", "fcvt_s_l", "fcvt_s_lu",
    "fsqrt_ps",
    "fcvt_pw_ps", "fcvt_pwu_ps", "fcvt_pl_ps", "fcvt_plu_ps",
    "fcvt_ps_pw", "fcvt_ps_pwu", "fcvt_ps_pl", "fcvt_ps_plu",
    "fcvt_ps_f16",
    "fcvt_ps_un24", "fcvt_ps_un16", "fcvt_ps_un10", "fcvt_ps_un8", "fcvt_ps_un2",
    "fcvt_ps_sn24", "fcvt_ps_sn16", "fcvt_ps_sn10", "fcvt_ps_sn8", "fcvt_ps_sn2",
    "fcvt_ps_f11", "fcvt_ps_f10",
    "fsin_ps", "fexp_ps", "flog_ps", "ffrc_ps", "fround_ps",
    "frcp_ps", "frsq_ps", "frcpfxp_ps",
    "fcvt_ps_rast", "fcvt_rast_ps"
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

// Constructor
instruction::instruction()
{
    pc = 0;
    enc_bits = 0;
    is_load = false;
    is_fpload = false;
    is_wfi = false;
    is_reduce = false;
    is_tensor_load = false;
    is_tensor_fma = false;
    is_texsndh = false;
    is_texrcv = false;
    is_1ulp = false;
    is_amo = false;
    is_flb = false;
    emu_func = NULL;
    emu_func0 = NULL;
    emu_func1 = NULL;
    emu_func2 = NULL;
    emu_func3 = NULL;
    emu_func4 = NULL;
    emu_func5 = NULL;
    num_params = 0;
    has_error = false;
    str_error = "No error";
}

// Destructor
instruction::~instruction()
{
}

// Access
void instruction::set_pc(uint64_t pc_)
{
    pc = pc_;
}

// Access
uint64_t instruction::get_pc()
{
    return pc;
}

// Access
void instruction::set_enc(uint32_t enc_bits_)
{
    enc_bits = enc_bits_;
}

// Access
uint32_t instruction::get_enc()
{
    return enc_bits;
}

// Sets the mnemonic. It also starts decoding it to generate
// the emulation routine
void instruction::set_mnemonic(std::string mnemonic_, testLog * log_)
{
    mnemonic = mnemonic_;
    log = log_;
    mnemonic.erase(std::remove(mnemonic.begin(), mnemonic.end(), '\n'), mnemonic.end());

    * log << LOG_DEBUG << "Mnemonic is " << mnemonic << endm;

    boost::regex e("^([^\\s]+)(.*)");
    boost::smatch m;
    std::string tmp = mnemonic;

    // First separate opcode from arguments
    boost::regex_search(tmp, m, e);
    std::string opcode = m[1];
    std::string args = m[2];

    if(opcode == "")
    {
        has_error = true;
        str_error = "Failed extracting opcode from instruction mnemonic";
    }

    // Change any "." in the ocpode name to underscore
    e = boost::regex("\\.");
    opcode = boost::regex_replace(opcode, e, std::string("_"));
    * log << LOG_DEBUG << "Opcode is " << opcode << endm;

    // Gets if the instruction is a load
    if((opcode == "ld") || (opcode == "lw") || (opcode == "lwu") || (opcode == "lh") || (opcode == "lhu") || (opcode == "lb") || (opcode == "lbu"))
        is_load = true;

    // Gets if the instruction is a floating point load
    if((opcode == "flw") || (opcode == "flw_ps"))
        is_fpload = true;

    if(opcode == "wfi")
        is_wfi = true;

    if(opcode=="texrcv")
      is_texrcv=true;

    if(opcode=="texsndh")
      is_texsndh=true;

    if((opcode=="frsq_ps") || (opcode=="flog_ps") || (opcode=="fexp_ps") || (opcode=="fsin_ps"))
        is_1ulp = true;

    if(   (opcode=="amoswap_d") || (opcode=="amoadd_d") || (opcode=="amoxor_d") || (opcode=="amoand_d")
       || (opcode=="amoor_d")   || (opcode=="amomin_d") || (opcode=="amomax_d") || (opcode=="amominu_d")
       || (opcode=="amomaxu_d"))
        is_amo = true;

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
    if(!boost::regex_match(opcode, boost::regex("csr.*")))
    {
        e = boost::regex("pc");
        args = boost::regex_replace(args, e, std::string(""));
    }

    // Converts to vector
    std::vector<std::string> arg_array;

    * log << LOG_DEBUG << "Args are " << args << endm;
    e = boost::regex("^([^,]+),");
    while(boost::regex_search(args, m, e))
    {
        arg_array.push_back(m[1]);
        args = boost::regex_replace(args, e, std::string(""));
    }
    if(args != "")
        arg_array.push_back(args);

    // Pseudoinstructions (convert to another instruction).
    // From riscv spec Chapter 21
    // Integer
    if(opcode == "li")
    {
        opcode = "addi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    if(opcode == "nop")
    {
        opcode = "addi";
        arg_array.push_back("x0");
        arg_array.push_back("x0");
        arg_array.push_back("0");
    }
    else if(opcode == "mv")
    {
        opcode = "addi";
        arg_array.push_back("0");
    }
    else if(opcode == "not")
    {
        opcode = "xori";
        arg_array.push_back("-1");
    }
    else if(opcode == "neg")
    {
        opcode = "sub";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "negw")
    {
        opcode = "subw";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "sext_w")
    {
        opcode = "addiw";
        arg_array.push_back("x0");
    }
    else if(opcode == "seqz")
    {
        opcode = "sltiu";
        arg_array.push_back("1");
    }
    else if(opcode == "snez")
    {
        opcode = "sltu";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "sltz")
    {
        opcode = "slt";
        arg_array.push_back("x0");
    }
    else if(opcode == "sgtz")
    {
        opcode = "slt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    // FP
    else if(opcode == "fmv_s")
    {
        opcode = "fsgnj_s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fabs_s")
    {
        opcode = "fsgnjx_s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fneg_s")
    {
        opcode = "fsgnjn_s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fmv_x_s")
    {
        opcode = "fmv_x_w";
    }
    else if(opcode == "fmv_s_x")
    {
        opcode = "fmv_w_x";
    }
    // Branches
    else if(opcode == "beqz")
    {
        opcode = "beq";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "bnez")
    {
        opcode = "bne";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "blez")
    {
        opcode = "bge";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "bgez")
    {
        opcode = "bge";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "bltz")
    {
        opcode = "blt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = "x0";
    }
    else if(opcode == "bgtz")
    {
        opcode = "blt";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    // Jumps
    else if(opcode == "j")
    {
        opcode = "jal";
        arg_array.push_back(arg_array[0]);
        arg_array[0] = "x0";
    }
    else if(opcode == "jal")
    {
        // If one argument means that link register dest is implicit and is x1
        if(arg_array.size() == 1)
        {
            arg_array.push_back(arg_array[0]);
            arg_array[0] = "x1";
        }
    }
    else if(opcode == "jr")
    {
        opcode = "jalr";
        arg_array.push_back(arg_array[0]);
        arg_array[0] = "x0";
        arg_array.push_back("0");
    }
    else if(opcode == "jalr")
    {
        // If one argument means that link register dest is implicit and is x1 and offset is 0
        if(arg_array.size() == 1)
        {
            arg_array.push_back(arg_array[0]);
            arg_array[0] = "x1";
            arg_array.push_back("0");
        }
        // If two arguments means that link register dest is implicit and is x1
        if(arg_array.size() == 2)
        {
            arg_array.push_back(arg_array[1]);
            arg_array[1] = arg_array[0];
            arg_array[0] = "x1";
        }
    }
    else if(opcode == "ret")
    {
        opcode = "jalr";
        arg_array.push_back("x0");
        arg_array.push_back("x1");
        arg_array.push_back("0");
    }
    // CSRs
    else if(opcode == "csrr")
    {
        opcode = "csrrs";
        arg_array.push_back("x0");
    }
    else if(opcode == "csrw")
    {
        opcode = "csrrw";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "csrs")
    {
        opcode = "csrrs";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "csrc")
    {
        opcode = "csrrc";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "csrwi")
    {
        opcode = "csrrwi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "csrsi")
    {
        opcode = "csrrsi";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(opcode == "csrci")
    {
        opcode = "csrrci";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }
    else if(!is_fpload && opcode[0] == 'f')
    {
        // Add implicit rounding mode operand to floating-point operations
        if(   (arg_array.size() == 2 && rm2args.find(opcode) != rm2args.end())
           || (arg_array.size() == 3 && rm3args.find(opcode) != rm3args.end())
           || (arg_array.size() == 4 && rm4args.find(opcode) != rm4args.end()))
        {
            arg_array.push_back("dyn");
        }
    }

    // JALR has different behaviour for compressed
    if(is_compressed && (opcode == "jalr"))
    {
        opcode = "c_jalr";
    }

    // Stores the arguments
    for(auto a:arg_array)
        add_parameter(a);

    // 0x0000 is an illegal instruction, but spike-dasm returns "addi s0, sp0, 0"
    // 0xffffffff is an illegal instruction, and spike-dasm returns "unknown"
    if(enc_bits == 0)
    {
        opcode = "unknown";
        num_params = 0;
    }

    // Checks if it is a tensor/reduce operation
    if((opcode == "csrrw") || (opcode == "cssrwi"))
    {
        is_reduce      = (params[1] == csr_treduce);
        is_tensor_load = (params[1] == csr_tloadctrl);
        is_tensor_fma  = (params[1] == csr_tfmastart);
        is_flb         = (params[1] == csr_flbarrier);
    }

    // Get the emulation function pointer for the opcode
    if(!has_error)
    {
        emu_func = get_function_ptr(opcode, &has_error, &str_error);
        if(!has_error)
        {
            emu_func0 = (func_ptr_0) emu_func;
            emu_func1 = (func_ptr_1) emu_func;
            emu_func2 = (func_ptr_2) emu_func;
            emu_func3 = (func_ptr_3) emu_func;
            emu_func4 = (func_ptr_4) emu_func;
            emu_func5 = (func_ptr_5) emu_func;
        }
    }

    if (has_error)
    {
        str_error += " while decoding instruction: " + mnemonic;
    }
}

// Access
std::string instruction::get_mnemonic()
{
    return mnemonic;
}

// Access
void instruction::set_compressed(bool v)
{
    is_compressed = v;
}

// Access
bool instruction::get_is_compressed()
{
    return is_compressed;
}

// Access
bool instruction::get_is_load()
{
    return is_load;
}

// Access
bool instruction::get_is_fpload()
{
    return is_fpload;
}

// Access
bool instruction::get_is_wfi()
{
    return is_wfi;
}

// Access
bool instruction::get_is_reduce()
{
    return is_reduce;
}

// Access
bool instruction::get_is_tensor_load()
{
  return is_tensor_load;
}

// Access
bool instruction::get_is_tensor_fma()
{
  return is_tensor_fma;
}

// Access
bool instruction::get_is_flb()
{
  return is_flb;
}

// Access
bool instruction::get_is_texrcv()
{
    return is_texrcv;
}

// Access
bool instruction::get_is_texsndh()
{
  return is_texsndh;
}

// Access
bool instruction::get_is_1ulp()
{
  return is_1ulp;
}

// Access
bool instruction::get_is_amo()
{
  return is_amo;
}

// Access
int instruction::get_param(int param)
{
    return params[param];
}

// Instruction execution
void instruction::exec()
{
    // If instruction had an error during decoding, report it when it is executed
    if(has_error)
        * log << LOG_FTL << str_error << endm;
    * log << LOG_DEBUG << "Executing instruction PC: 0x" << std::hex << pc << ", Bits: 0x" << enc_bits << std::dec << ", Mnemonic: " << mnemonic << endm;
    switch(num_params)
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
    * log << LOG_DEBUG << "Adding parameter <" << param << ">" << endm;
    // Args unknown
    if(param == "argsunknown")
    {
        // Weird case seen in WFI where despite having no parameter the disasm returns this parameter
        return;
    }
    // Hex constant
    if(param.find("0x") !=std::string::npos)
    {
        // Negative
        bool neg = false;
        if(param[0] == '-')
        {
            param.erase(0, 1);
            neg = true;
        }
        int c = sscanf(param.c_str(), "0x%X", &params[num_params]);
        if(c != 1)
        {
            has_error = true;
            str_error = "Error parsing parameter " + param + ". Expecting hex immediate";
        }
        if(neg)
            params[num_params] = -params[num_params];
    }
    // Dec constat
    else if(boost::regex_match(param, boost::regex("^-?[[:d:]]+")))
    {
        int c = sscanf(param.c_str(), "%i", &params[num_params]);
        if(c != 1)
        {
            has_error = true;
            str_error = "Error parsing parameter " + param + ". Expecting dec immediate";
        }
    }
    // Floating register
    else if(param[0] == 'f')
    {
        if     (param == "ft0")  params[num_params] = 0;
        else if(param == "ft1")  params[num_params] = 1;
        else if(param == "ft2")  params[num_params] = 2;
        else if(param == "ft3")  params[num_params] = 3;
        else if(param == "ft4")  params[num_params] = 4;
        else if(param == "ft5")  params[num_params] = 5;
        else if(param == "ft6")  params[num_params] = 6;
        else if(param == "ft7")  params[num_params] = 7;
        else if(param == "fs0")  params[num_params] = 8;
        else if(param == "fs1")  params[num_params] = 9;
        else if(param == "fa0")  params[num_params] = 10;
        else if(param == "fa1")  params[num_params] = 11;
        else if(param == "fa2")  params[num_params] = 12;
        else if(param == "fa3")  params[num_params] = 13;
        else if(param == "fa4")  params[num_params] = 14;
        else if(param == "fa5")  params[num_params] = 15;
        else if(param == "fa6")  params[num_params] = 16;
        else if(param == "fa7")  params[num_params] = 17;
        else if(param == "fs2")  params[num_params] = 18;
        else if(param == "fs3")  params[num_params] = 19;
        else if(param == "fs4")  params[num_params] = 20;
        else if(param == "fs5")  params[num_params] = 21;
        else if(param == "fs6")  params[num_params] = 22;
        else if(param == "fs7")  params[num_params] = 23;
        else if(param == "fs8")  params[num_params] = 24;
        else if(param == "fs9")  params[num_params] = 25;
        else if(param == "fs10") params[num_params] = 26;
        else if(param == "fs11") params[num_params] = 27;
        else if(param == "ft8")  params[num_params] = 28;
        else if(param == "ft9")  params[num_params] = 29;
        else if(param == "ft10") params[num_params] = 30;
        else if(param == "ft11") params[num_params] = 31;
        // CSRs
        else if(param == "fcsr")    params[num_params] = csr_fcsr;
        else if(param == "frm")     params[num_params] = csr_frm;
        else if(param == "fflags")  params[num_params] = csr_fflags;
        else if(param == "flb0")    params[num_params] = csr_flbarrier;
        else
        {
            int c = sscanf(param.c_str(), "f%i", &params[num_params]);
            if(c != 1)
            {
                has_error = true;
                str_error = "Error parsing parameter " + param + ". Expecting float register";
            }
        }
    }
    // Integer register
    else if(param[0] == 'x')
    {
        int c = sscanf(param.c_str(), "x%i", &params[num_params]);
        if(c != 1)
        {
            has_error = true;
            str_error = "Error parsing parameter " + param + ". Expecting integer register";
        }
    }
    // Integer register, CSR or Mask registers
    else
    {
        if     (param == "zero") params[num_params] = 0;
        else if(param == "ra")   params[num_params] = 1;
        else if(param == "sp")   params[num_params] = 2;
        else if(param == "gp")   params[num_params] = 3;
        else if(param == "tp")   params[num_params] = 4;
        else if(param == "t0")   params[num_params] = 5;
        else if(param == "t1")   params[num_params] = 6;
        else if(param == "t2")   params[num_params] = 7;
        else if(param == "s0")   params[num_params] = 8;
        else if(param == "s1")   params[num_params] = 9;
        else if(param == "a0")   params[num_params] = 10;
        else if(param == "a1")   params[num_params] = 11;
        else if(param == "a2")   params[num_params] = 12;
        else if(param == "a3")   params[num_params] = 13;
        else if(param == "a4")   params[num_params] = 14;
        else if(param == "a5")   params[num_params] = 15;
        else if(param == "a6")   params[num_params] = 16;
        else if(param == "a7")   params[num_params] = 17;
        else if(param == "s2")   params[num_params] = 18;
        else if(param == "s3")   params[num_params] = 19;
        else if(param == "s4")   params[num_params] = 20;
        else if(param == "s5")   params[num_params] = 21;
        else if(param == "s6")   params[num_params] = 22;
        else if(param == "s7")   params[num_params] = 23;
        else if(param == "s8")   params[num_params] = 24;
        else if(param == "s9")   params[num_params] = 25;
        else if(param == "s10")  params[num_params] = 26;
        else if(param == "s11")  params[num_params] = 27;
        else if(param == "t3")   params[num_params] = 28;
        else if(param == "t4")   params[num_params] = 29;
        else if(param == "t5")   params[num_params] = 30;
        else if(param == "t6")   params[num_params] = 31;
        // CSRs
        else if(param == "sstatus")      params[num_params] = csr_sstatus;
        //else if(param == "sedeleg")      params[num_params] = csr_sedeleg;
        //else if(param == "sideleg")      params[num_params] = csr_sideleg;
        else if(param == "sie")          params[num_params] = csr_sie;
        else if(param == "stvec")        params[num_params] = csr_stvec;
        //else if(param == "scounteren")   params[num_params] = csr_scounteren;
        else if(param == "sscratch")     params[num_params] = csr_sscratch;
        else if(param == "sepc")         params[num_params] = csr_sepc;
        else if(param == "scause")       params[num_params] = csr_scause;
        else if(param == "stval")        params[num_params] = csr_stval;
        else if(param == "sip")          params[num_params] = csr_sip;
        else if(param == "satp")         params[num_params] = csr_satp;
        else if(param == "mvendorid")    params[num_params] = csr_mvendorid;
        else if(param == "marchid")      params[num_params] = csr_marchid;
        else if(param == "mimpid")       params[num_params] = csr_mimpid;
        else if(param == "mhartid")      params[num_params] = csr_mhartid;
        else if(param == "mstatus")      params[num_params] = csr_mstatus;
        else if(param == "misa")         params[num_params] = csr_misa;
        else if(param == "medeleg")      params[num_params] = csr_medeleg;
        else if(param == "mideleg")      params[num_params] = csr_mideleg;
        else if(param == "mie")          params[num_params] = csr_mie;
        else if(param == "mtvec")        params[num_params] = csr_mtvec;
        //else if(param == "mcounteren")   params[num_params] = csr_mcounteren;
        else if(param == "mscratch")     params[num_params] = csr_mscratch;
        else if(param == "mepc")         params[num_params] = csr_mepc;
        else if(param == "mcause")       params[num_params] = csr_mcause;
        else if(param == "mtval")        params[num_params] = csr_mtval;
        else if(param == "mip")          params[num_params] = csr_mip;
        else if(param == "tensor_reduce")    params[num_params] = csr_treduce;
        else if(param == "tensor_fma")       params[num_params] = csr_tfmastart;
        else if(param == "tensor_conv_size") params[num_params] = csr_tconvsize;
        else if(param == "tensor_conv_ctrl") params[num_params] = csr_tconvctrl;
        else if(param == "unknown_804")      params[num_params] = csr_tcoop;
        else if(param == "usr_cache_op")     params[num_params] = csr_ucacheop;
        else if(param == "tensor_load")      params[num_params] = csr_tloadctrl;
        else if(param == "tensor_store")     params[num_params] = csr_tstore;
        else if(param == "umsg_port0")       params[num_params] = csr_umsg_port0;
        else if(param == "umsg_port1")       params[num_params] = csr_umsg_port1;
        else if(param == "umsg_port2")       params[num_params] = csr_umsg_port2;
        else if(param == "umsg_port3")       params[num_params] = csr_umsg_port3;
        else if(param == "sys_cache_op")     params[num_params] = csr_scacheop;
        else if(param == "smsg_port0")       params[num_params] = csr_smsg_port0;
        else if(param == "smsg_port1")       params[num_params] = csr_smsg_port1;
        else if(param == "smsg_port2")       params[num_params] = csr_smsg_port2;
        else if(param == "smsg_port3")       params[num_params] = csr_smsg_port3;
        else if(param == "icache_ctrl")      params[num_params] = csr_icache_ctrl;
        else if(param == "write_ctrl")       params[num_params] = csr_write_ctrl;
        else if(param == "validation0")      params[num_params] = validation0;
        else if(param == "validation1")      params[num_params] = validation1;
        else if(param == "validation2")      params[num_params] = validation2;
        else if(param == "validation3")      params[num_params] = validation3;
        // TODO: currently unsupported CSRs
        else if(param == "ustatus"    ||
                param == "uie"        ||
                param == "utvec"      ||
                param == "uscratch"   ||
                param == "uepc"       ||
                param == "ucause"     ||
                param == "utval"      ||
                param == "uip"        ||
                param == "cycle"      ||
                param == "time"       ||
                param == "instret"    ||
                param == "cycleh"     ||
                param == "timeh"      ||
                param == "insreth"    ||
                param == "sedeleg"    ||
                param == "sideleg"    ||
                param == "scouteren"  ||
                param == "mcounteren" ||
                param == "mcycle"     ||
                param == "minstret"   ||
                param == "mcycleh"    ||
                param == "minstreth"  ||
                param == "tselect"    ||
                param == "tdata1"     ||
                param == "tdata2"     ||
                param == "tdata3"     ||
                param == "dcsr"       ||
                param == "dpc"        ||
                param == "dscratch"){
                   has_error = true;
                   str_error = "Unsupported register " + param;
                }
        // Mask register
        else if(param[0] == 'm')
          {
            if     (param == "mt0")  params[num_params] = 0;
            else if(param == "mt1")  params[num_params] = 1;
            else if(param == "mt2")  params[num_params] = 2;
            else if(param == "mt3")  params[num_params] = 3;
            else if(param == "mt4")  params[num_params] = 4;
            else if(param == "mt5")  params[num_params] = 5;
            else if(param == "mt6")  params[num_params] = 6;
            else if(param == "mt7")  params[num_params] = 7;
          }
        // rounding modes
        else if ( param == "rne") params[num_params] = 0;
        else if ( param == "rtz") params[num_params] = 1;
        else if ( param == "rdn") params[num_params] = 2;
        else if ( param == "rup") params[num_params] = 3;
        else if ( param == "rmm") params[num_params] = 4;
        else if ( param == "dyn") params[num_params] = 7;
        else
          {
            has_error = true;
            str_error = "Unknown parameter " + param + ". Expecting integer register or CSR";
          }
    }
    num_params++;
}

// Returns the pointer to a function based on name
func_ptr instruction::get_function_ptr(std::string func, bool * error, std::string * error_msg)
{
    emu_ptr_hash_t::const_iterator el = pointer_cache.find(func);
    if(el != pointer_cache.end())
    {
        if(error != NULL)
            *error = false;
        return el->second;
    }

    std::string msg = std::string("Uknown mnemonic: ") + func;
    if(error != NULL)
    {
        // Reports the error to source
        *error = true;
        *error_msg = msg;
    }
    else
    {
        // No way to report the error, simply
        * log << LOG_FTL << msg << endm;
    }
    return nullptr;
}
