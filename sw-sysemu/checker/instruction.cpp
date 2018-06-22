// Local
#include "instruction.h"
#include "emu.h"

// STD
#include <iostream>
#include <boost/regex.hpp>
#include <algorithm>
#include <vector>

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
    num_params = 0;
    has_error = false;
    str_error = "No error";
}

// Destructor
instruction::~instruction()
{
}

// Access
void instruction::set_pc(uint64 pc_)
{
    pc = pc_;
}

// Access
uint64 instruction::get_pc()
{
    return pc;
}

// Access
void instruction::set_enc(uint32 enc_bits_)
{
    enc_bits = enc_bits_;
}

// Access
uint32 instruction::get_enc()
{
    return enc_bits;
}

// Sets the mnemonic. It also starts decoding it to generate
// the emulation routine
void instruction::set_mnemonic(std::string mnemonic_, function_pointer_cache * func_cache, testLog * log_)
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
    if((opcode == "flw") || (opcode == "fld") || (opcode == "flw_ps"))
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
    else if(opcode == "fmv.s")
    {
        opcode = "fsgnj.s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fabs.s")
    {
        opcode = "fsgnjx.s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fneg.s")
    {
        opcode = "fsgnjn.s";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fmv.d")
    {
        opcode = "fsgnj.d";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fabs.d")
    {
        opcode = "fsgnjx.d";
        arg_array.push_back(arg_array[1]);
    }
    else if(opcode == "fneg.d")
    {
        opcode = "fsgnjn.d";
        arg_array.push_back(arg_array[1]);
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
    }/*
    else if(opcode == "csrw")
    {
        opcode = "csrrw";
        arg_array.push_back(arg_array[1]);
        arg_array[1] = arg_array[0];
        arg_array[0] = "x0";
    }*/
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

    // Special opcodes that need translation due conflicts
    if(opcode == "or")
    {
        opcode = "or_";
    }
    else if(opcode == "and")
    {
        opcode = "and_";
    }
    else if(opcode == "xor")
    {
        opcode = "xor_";
    }
    else if(opcode == "div")
    {
        opcode = "div_";
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
    if (opcode == "csrw") {
        is_reduce      = (params[0] == csr_treduce);
        is_tensor_load = (params[0] == csr_tloadctrl);
        is_tensor_fma  = (params[0] == csr_tfmastart);
        is_flb         = (params[0] == csr_flbarrier);
    }
    else if(boost::regex_match(opcode, boost::regex("csr.*")))
    {
        is_reduce      = (params[1] == csr_treduce);
        is_tensor_load = (params[1] == csr_tloadctrl);
        is_tensor_fma  = (params[1] == csr_tfmastart);
        is_flb         = (params[1] == csr_flbarrier);
    }

    // Get the emulation function pointer for the opcode
    if(!has_error)
    {
        emu_func = func_cache->get_function_ptr(opcode, &has_error, &str_error);
        if(!has_error)
        {
            emu_func0 = (func_ptr_0) emu_func;
            emu_func1 = (func_ptr_1) emu_func;
            emu_func2 = (func_ptr_2) emu_func;
            emu_func3 = (func_ptr_3) emu_func;
            emu_func4 = (func_ptr_4) emu_func;
        }
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
    * log << LOG_DEBUG << "Executing instrucion PC: 0x" << std::hex << pc << ", Bits: 0x" << enc_bits << std::dec << ", Mnemonic: " << mnemonic << endm;
    switch(num_params)
    {
        case 0: (emu_func0("")); break;
        case 1: (emu_func1(params[0], "")); break;
        case 2: (emu_func2(params[0], params[1], "")); break;
        case 3: (emu_func3(params[0], params[1], params[2], "")); break;
        case 4: (emu_func4(params[0], params[1], params[2], params[3], "")); break;
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
        else if(param == "unknown_800")  params[num_params] = csr_treduce;
        else if(param == "unknown_801")  params[num_params] = csr_tfmastart;
        else if(param == "unknown_802")  params[num_params] = csr_tconvsize;
        else if(param == "unknown_803")  params[num_params] = csr_tconvctrl;
        else if(param == "unknown_81f")  params[num_params] = csr_ucacheop;
        else if(param == "unknown_820")  params[num_params] = csr_flbarrier;
        else if(param == "unknown_83f")  params[num_params] = csr_tloadctrl;
        else if(param == "unknown_87f")  params[num_params] = csr_tstore;
        else if(param == "unknown_8cc")  params[num_params] = csr_umsg_port0;
        else if(param == "unknown_8cd")  params[num_params] = csr_umsg_port1;
        else if(param == "unknown_8ce")  params[num_params] = csr_umsg_port2;
        else if(param == "unknown_8cf")  params[num_params] = csr_umsg_port3;
        else if(param == "unknown_51f")  params[num_params] = csr_scacheop;
        else if(param == "unknown_9cc")  params[num_params] = csr_smsg_port0;
        else if(param == "unknown_9cd")  params[num_params] = csr_smsg_port1;
        else if(param == "unknown_9ce")  params[num_params] = csr_smsg_port2;
        else if(param == "unknown_9cf")  params[num_params] = csr_smsg_port3;
        else if(param == "unknown_7c0" || param == "mt1rvect")  params[num_params] = csr_mt1rvect;
        else if(param == "unknown_7c1" || param == "mt1en")     params[num_params] = csr_mt1en;
        else if(param == "unknown_7cb")  params[num_params] = csr_icache_ctrl;
        else if(param == "unknown_7cc")  params[num_params] = csr_write_ctrl;
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

