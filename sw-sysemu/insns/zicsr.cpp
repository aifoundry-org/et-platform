/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstdio>       // for snprintf()
#include <unordered_map>

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "traps.h"
#include "tensor.h"
#include "txs.h"
#include "utility.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;
extern uint64_t current_pc;
extern uint32_t current_inst;
extern bool     m_emu_done;


// vendor, arch, imp, ISA values
#define CSR_VENDOR_ID ((11<<7) |        /* bank 11 */ \
                       (0xe5 & 0x7f))   /* 0xE5 (0x65 without parity) */
#define CSR_ARCH_ID 0x8000000000000001ull
#define CSR_IMP_ID  0x0
#define CSR_ISA_MAX ((1ull << 2)  | /* "C" Compressed extension */                      \
                     (1ull << 5)  | /* "F" Single-precision floating-point extension */ \
                     (1ull << 8)  | /* "I" RV32I/64I/128I base ISA */                   \
                     (1ull << 12) | /* "M" Integer Multiply/Divide extension */         \
                     (1ull << 18) | /* "S" Supervisor mode implemented */               \
                     (1ull << 20) | /* "U" User mode implemented */                     \
                     (1ull << 23) | /* "X": Non-standard extensions present */          \
                     (2ull << 62))  /* XLEN = 64-bit */


// Fast local barrier
uint64_t write_flb(uint64_t);

// Messaging
int64_t port_get(unsigned, bool);
uint32_t legalize_portctrl(uint32_t);
void configure_port(unsigned, uint32_t);
uint64_t read_port_base_address(unsigned , unsigned);

// Cache management
void dcache_change_mode(uint8_t, uint8_t);
void dcache_evict_flush_set_way(bool, bool, int, int, int, int);
void dcache_evict_flush_vaddr(bool, bool, int, uint64_t, int, int, uint64_t);
void dcache_prefetch_vaddr(uint64_t);
void dcache_lock_vaddr(bool, uint64_t, int, int, uint64_t);
void dcache_unlock_vaddr(bool, uint64_t, int, int, uint64_t);
void dcache_lock_paddr(int, uint64_t);
void dcache_unlock_set_way(int, int);

// Tensor extension
void tensor_coop_write(uint64_t);
void tensor_fma16a32_start(uint64_t);
void tensor_fma32_start(uint64_t);
void tensor_ima8a32_start(uint64_t);
void tensor_load_l2_start(uint64_t);
void tensor_load_start(uint64_t);
void tensor_mask_update();
void tensor_quant_start(uint64_t);
void tensor_reduce_start(uint64_t);
void tensor_store_start(uint64_t);
void tensor_wait_start(uint64_t);

//namespace bemu {


// UART -- for VALIDATION1
static std::ostringstream uart_stream[EMU_NUM_THREADS];


static const char* csr_name(uint16_t num)
{
    static thread_local char unknown_name[48] = {'\0', };
    static thread_local int  unknown_start = 0;
    static const std::unordered_map<uint16_t, const char*> csr_names = {
#define CSRDEF(num, lower, upper)       { num, #lower },
#include "csrs.h"
#undef CSRDEF
    };

    auto it = csr_names.find(num);
    if (it == csr_names.cend()) {
        (void) snprintf(&unknown_name[unknown_start], 6, "0x%03x",
                        unsigned(num & 0xfff));
        const char* ptr = &unknown_name[unknown_start];
        unknown_start = (unknown_start >= 42) ? 0 : (unknown_start + 6);
        return ptr;
    }
    return it->second;
}


static inline void check_csr_privilege(insn_t inst, uint16_t csr)
{
    int curprv = PRV;
    int csrprv = (csr >> 8) & 3;

    if (csrprv > curprv)
        throw trap_illegal_instruction(inst.bits);

    if ((csr == CSR_SATP) && (curprv == PRV_S) &&
        ((cpu[current_thread].mstatus >> 20) & 1))
        throw trap_illegal_instruction(inst.bits);
}


static void check_counter_is_enabled(int n)
{
    uint64_t enabled = (cpu[current_thread].mcounteren & (1 << n));

    switch (PRV) {
    case PRV_U:
        if ((cpu[current_thread].scounteren & enabled) == 0)
            throw trap_illegal_instruction(current_inst);
        break;
    case PRV_S:
        if (enabled == 0)
            throw trap_illegal_instruction(current_inst);
        break;
    default:
        break;
    }
}


static uint64_t csrget(uint16_t csr)
{
    uint64_t val;

    switch (csr) {
    case CSR_FFLAGS:
        require_fp_active();
        val = cpu[current_thread].fcsr & 0x8000001f;
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu[current_thread].fcsr >> 5) & 0x7;
        break;
    case CSR_FCSR:
        require_fp_active();
        val = cpu[current_thread].fcsr;
        break;
    case CSR_SSTATUS:
        // Hide sxl, tsr, tw, tvm, mprv, mpp, mpie, mie
        val = cpu[current_thread].mstatus & 0x80000003000DE133ULL;
        break;
    case CSR_SIE:
        val = cpu[current_thread].mie & cpu[current_thread].mideleg;
        break;
    case CSR_STVEC:
        val = cpu[current_thread].stvec;
        break;
    case CSR_SCOUNTEREN:
        val = cpu[current_thread].scounteren;
        break;
    case CSR_SSCRATCH:
        val = cpu[current_thread].sscratch;
        break;
    case CSR_SEPC:
        val = cpu[current_thread].sepc;
        break;
    case CSR_SCAUSE:
        val = cpu[current_thread].scause;
        break;
    case CSR_STVAL:
        val = cpu[current_thread].stval;
        break;
    case CSR_SIP:
        val = cpu[current_thread].mip & cpu[current_thread].mideleg;
        break;
    case CSR_SATP:
        val = cpu[current_thread].satp;
        break;
    case CSR_MSTATUS:
        val = cpu[current_thread].mstatus;
        break;
    case CSR_MISA:
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        val = cpu[current_thread].medeleg;
        break;
    case CSR_MIDELEG:
        val = cpu[current_thread].mideleg;
        break;
    case CSR_MIE:
        val = cpu[current_thread].mie;
        break;
    case CSR_MTVEC:
        val = cpu[current_thread].mtvec;
        break;
    case CSR_MCOUNTEREN:
        val = cpu[current_thread].mcounteren;
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val = cpu[current_thread].mhpmevent[csr - CSR_MHPMEVENT3];
        break;
    case CSR_MHPMEVENT9:
    case CSR_MHPMEVENT10:
    case CSR_MHPMEVENT11:
    case CSR_MHPMEVENT12:
    case CSR_MHPMEVENT13:
    case CSR_MHPMEVENT14:
    case CSR_MHPMEVENT15:
    case CSR_MHPMEVENT16:
    case CSR_MHPMEVENT17:
    case CSR_MHPMEVENT18:
    case CSR_MHPMEVENT19:
    case CSR_MHPMEVENT20:
    case CSR_MHPMEVENT21:
    case CSR_MHPMEVENT22:
    case CSR_MHPMEVENT23:
    case CSR_MHPMEVENT24:
    case CSR_MHPMEVENT25:
    case CSR_MHPMEVENT26:
    case CSR_MHPMEVENT27:
    case CSR_MHPMEVENT28:
    case CSR_MHPMEVENT29:
    case CSR_MHPMEVENT30:
    case CSR_MHPMEVENT31:
        val = 0;
        break;
    case CSR_MSCRATCH:
        val = cpu[current_thread].mscratch;
        break;
    case CSR_MEPC:
        val = cpu[current_thread].mepc;
        break;
    case CSR_MCAUSE:
        val = cpu[current_thread].mcause;
        break;
    case CSR_MTVAL:
        val = cpu[current_thread].mtval;
        break;
    case CSR_MIP:
        val = cpu[current_thread].mip;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        val = cpu[current_thread].tdata1;
        break;
    case CSR_TDATA2:
        val = cpu[current_thread].tdata2;
        break;
        // unimplemented: TDATA3
        // TODO: DCSR
        // TODO: DPC
        // unimplemented: DSCRATCH0
        // unimplemented: DSCRATCH1
    case CSR_MCYCLE:
    case CSR_MINSTRET:
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
    case CSR_MHPMCOUNTER9:
    case CSR_MHPMCOUNTER10:
    case CSR_MHPMCOUNTER11:
    case CSR_MHPMCOUNTER12:
    case CSR_MHPMCOUNTER13:
    case CSR_MHPMCOUNTER14:
    case CSR_MHPMCOUNTER15:
    case CSR_MHPMCOUNTER16:
    case CSR_MHPMCOUNTER17:
    case CSR_MHPMCOUNTER18:
    case CSR_MHPMCOUNTER19:
    case CSR_MHPMCOUNTER20:
    case CSR_MHPMCOUNTER21:
    case CSR_MHPMCOUNTER22:
    case CSR_MHPMCOUNTER23:
    case CSR_MHPMCOUNTER24:
    case CSR_MHPMCOUNTER25:
    case CSR_MHPMCOUNTER26:
    case CSR_MHPMCOUNTER27:
    case CSR_MHPMCOUNTER28:
    case CSR_MHPMCOUNTER29:
    case CSR_MHPMCOUNTER30:
    case CSR_MHPMCOUNTER31:
        val = 0;
        break;
    case CSR_CYCLE:
    case CSR_INSTRET:
    case CSR_HPMCOUNTER3:
    case CSR_HPMCOUNTER4:
    case CSR_HPMCOUNTER5:
    case CSR_HPMCOUNTER6:
    case CSR_HPMCOUNTER7:
    case CSR_HPMCOUNTER8:
        check_counter_is_enabled(csr - CSR_CYCLE);
        val = 0;
        break;
    case CSR_HPMCOUNTER9:
    case CSR_HPMCOUNTER10:
    case CSR_HPMCOUNTER11:
    case CSR_HPMCOUNTER12:
    case CSR_HPMCOUNTER13:
    case CSR_HPMCOUNTER14:
    case CSR_HPMCOUNTER15:
    case CSR_HPMCOUNTER16:
    case CSR_HPMCOUNTER17:
    case CSR_HPMCOUNTER18:
    case CSR_HPMCOUNTER19:
    case CSR_HPMCOUNTER20:
    case CSR_HPMCOUNTER21:
    case CSR_HPMCOUNTER22:
    case CSR_HPMCOUNTER23:
    case CSR_HPMCOUNTER24:
    case CSR_HPMCOUNTER25:
    case CSR_HPMCOUNTER26:
    case CSR_HPMCOUNTER27:
    case CSR_HPMCOUNTER28:
    case CSR_HPMCOUNTER29:
    case CSR_HPMCOUNTER30:
    case CSR_HPMCOUNTER31:
        throw trap_illegal_instruction(current_inst);
        break;
    case CSR_MVENDORID:
        val = CSR_VENDOR_ID;
        break;
    case CSR_MARCHID:
        val = CSR_ARCH_ID;
        break;
    case CSR_MIMPID:
        val = CSR_IMP_ID;
        break;
    case CSR_MHARTID:
        val = cpu[current_thread].mhartid;
        break;
        // ----- Esperanto registers -------------------------------------
    case CSR_MATP:
        val = cpu[current_thread].matp;
        break;
    case CSR_MINSTMASK:
        val = cpu[current_thread].minstmask;
        break;
    case CSR_MINSTMATCH:
        val = cpu[current_thread].minstmatch;
        break;
    case CSR_CACHE_INVALIDATE:
        val = 0;
        break;
    case CSR_MENABLE_SHADOWS:
        val = cpu[current_thread].menable_shadows;
        break;
    case CSR_EXCL_MODE:
        val = cpu[current_thread].excl_mode & 1;
        break;
    case CSR_MBUSADDR:
        val = cpu[current_thread].mbusaddr;
        break;
    case CSR_MCACHE_CONTROL:
        val = cpu[current_thread].mcache_control;
        break;
    case CSR_EVICT_SW:
    case CSR_FLUSH_SW:
    case CSR_LOCK_SW:
    case CSR_UNLOCK_SW:
        val = 0;
        break;
    case CSR_TENSOR_REDUCE:
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TENSOR_CONV_SIZE:
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_COOP:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TENSOR_MASK:
        require_feature_ml();
        val = cpu[current_thread].tensor_mask;
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TEX_SEND:
        require_feature_gfx();
        val = 0;
        break;
    case CSR_TENSOR_ERROR:
        val = cpu[current_thread].tensor_error;
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        val = cpu[current_thread].ucache_control;
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_FLB:
    case CSR_FCC:
    case CSR_STALL:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_WAIT:
        val = 0;
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_GSC_PROGRESS:
        val = cpu[current_thread].gsc_progress;
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_EVICT_VA:
    case CSR_FLUSH_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
        // LCOV_EXCL_START
    case CSR_VALIDATION0:
        val = cpu[current_thread].validation0;
        break;
    case CSR_VALIDATION1:
#ifdef SYS_EMU
        val = (cpu[current_thread].validation1 == ET_DIAG_CYCLE) ? sys_emu::get_emu_cycle() : 0;
#else
        val = 0;
#endif
        break;
    case CSR_VALIDATION2:
        val = cpu[current_thread].validation2;
        break;
    case CSR_VALIDATION3:
        val = cpu[current_thread].validation3;
        break;
        // LCOV_EXCL_STOP
    case CSR_LOCK_VA:
    case CSR_UNLOCK_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = cpu[current_thread].portctrl[csr - CSR_PORTCTRL0];
        break;
    case CSR_FCCNB:
        require_feature_ml();
        val = (uint64_t(cpu[current_thread].fcc[1]) << 16) + uint64_t(cpu[current_thread].fcc[0]);
        break;
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
        if (((cpu[current_thread].portctrl[csr-CSR_PORTHEAD0] & 0x1) == 0)
            || ((PRV == PRV_U) && ((cpu[current_thread].portctrl[csr-CSR_PORTHEAD0] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(csr - CSR_PORTHEAD0, true);
        break;
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
        if (((cpu[current_thread].portctrl[csr-CSR_PORTHEADNB0] & 0x1) == 0)
            || ((PRV == PRV_U) && ((cpu[current_thread].portctrl[csr-CSR_PORTHEADNB0] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(csr - CSR_PORTHEADNB0, false);
        break;
    case CSR_HARTID:
        if (PRV != PRV_M && (cpu[current_thread].menable_shadows & 1) == 0) {
            throw trap_illegal_instruction(current_inst);
        }
        val = cpu[current_thread].mhartid;
        break;
    case CSR_DCACHE_DEBUG:
        val = 0;
        break;
        // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }
    return val;
}


static uint64_t csrset(uint16_t csr, uint64_t val)
{
    uint64_t msk;
#ifdef SYS_EMU
    int orig_mcache_control;
#endif

    switch (csr) {
    case CSR_FFLAGS:
        require_fp_active();
        val = (cpu[current_thread].fcsr & 0x000000E0) | (val & 0x8000001F);
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu[current_thread].fcsr & 0x8000001F) | ((val & 0x7) << 5);
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_FCSR:
        require_fp_active();
        val &= 0x800000FF;
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_SSTATUS:
        // Preserve sd, sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
        // Modify mxr, sum, fs, spp, spie, (upie=0), sie, (uie=0)
        val = (val & 0x00000000000C6122ULL) | (cpu[current_thread].mstatus & 0x0000000F00739888ULL);
        // Setting fs=1 or fs=2 will set fs=3
        if (val & 0x6000ULL) {
            val |= 0x6000ULL;
        }
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3)) {
            val |= 0x8000000000000000ULL;
        }
        cpu[current_thread].mstatus = val;
        break;
    case CSR_SIE:
        // Only ssie, stie, and seie are writeable, and only if they are delegated
        // if mideleg[sei,sti,ssi]==1 then seie, stie, ssie is writeable, otherwise they are reserved
        msk = cpu[current_thread].mideleg & 0x0000000000000222ULL;
        val = (cpu[current_thread].mie & ~msk) | (val & msk);
        cpu[current_thread].mie = val;
        break;
    case CSR_STVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu[current_thread].stvec = val;
        cpu[current_thread].stvec_is_set = true;
        break;
    case CSR_SCOUNTEREN:
        val &= 0x1FF;
        cpu[current_thread].scounteren = uint16_t(val);
        break;
    case CSR_SSCRATCH:
        cpu[current_thread].sscratch = val;
        break;
    case CSR_SEPC:
        // sepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu[current_thread].sepc = val;
        break;
    case CSR_SCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu[current_thread].scause = val;
        break;
    case CSR_STVAL:
        val = sextVA(val);
        cpu[current_thread].stval = val;
        break;
    case CSR_SIP:
        // Only ssip is writeable, and only if it is delegated
        msk = cpu[current_thread].mideleg & 0x0000000000000002ULL;
        val = (cpu[current_thread].mip & ~msk) | (val & msk);
        cpu[current_thread].mip = val;
        break;
    case CSR_SATP: // Shared register
        // MODE is 4 bits, ASID is 0bits, PPN is PPN_M bits
        val &= 0xF000000000000000ULL | PPN_M;
        switch (val >> 60) {
        case SATP_MODE_BARE:
        case SATP_MODE_SV39:
        case SATP_MODE_SV48:
            cpu[current_thread].satp = val;
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].satp = val;
            break;
        default: // reserved
            // do not write the register if attempting to set an unsupported mode
            break;
        }
        break;
    case CSR_MSTATUS:
        // Preserve sd, sxl, uxl, xs
        // Write all others (except upie=0, uie=0)
        val = (val & 0x00000000007E79AAULL) | (cpu[current_thread].mstatus & 0x0000000F00018000ULL);
        // Setting fs=1 or fs=2 will set fs=3
        if (val & 0x6000ULL) {
            val |= 0x6000ULL;
        }
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3)) {
            val |= 0x8000000000000000ULL;
        }
        // Attempting to set mpp to 2 will set it to 0 instead
        if (((val >> 11) & 0x3) == 0x2)
            val &= ~(0x3ULL << 11);
        cpu[current_thread].mstatus = val;
        break;
    case CSR_MISA:
        // Writeable but hardwired
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        // Not all exceptions can be delegated
        val &= 0x0000000000000B108ULL;
        cpu[current_thread].medeleg = val;
        break;
    case CSR_MIDELEG:
        // Not all interrupts can be delegated
        val &= 0x0000000000000222ULL;
        cpu[current_thread].mideleg = val;
        break;
    case CSR_MIE:
        // Hard-wire ueie, utie, usie
        val &= 0x0000000000890AAAULL;
        cpu[current_thread].mie = val;
        break;
    case CSR_MTVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu[current_thread].mtvec = val;
        cpu[current_thread].mtvec_is_set = true;
        break;
    case CSR_MCOUNTEREN:
        val &= 0x1FF;
        cpu[current_thread].mcounteren = uint16_t(val);
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val &= 0x1F;
        cpu[current_thread].mhpmevent[csr - CSR_MHPMEVENT3] = val;
        break;
    case CSR_MHPMEVENT9:
    case CSR_MHPMEVENT10:
    case CSR_MHPMEVENT11:
    case CSR_MHPMEVENT12:
    case CSR_MHPMEVENT13:
    case CSR_MHPMEVENT14:
    case CSR_MHPMEVENT15:
    case CSR_MHPMEVENT16:
    case CSR_MHPMEVENT17:
    case CSR_MHPMEVENT18:
    case CSR_MHPMEVENT19:
    case CSR_MHPMEVENT20:
    case CSR_MHPMEVENT21:
    case CSR_MHPMEVENT22:
    case CSR_MHPMEVENT23:
    case CSR_MHPMEVENT24:
    case CSR_MHPMEVENT25:
    case CSR_MHPMEVENT26:
    case CSR_MHPMEVENT27:
    case CSR_MHPMEVENT28:
    case CSR_MHPMEVENT29:
    case CSR_MHPMEVENT30:
    case CSR_MHPMEVENT31:
        val = 0;
        break;
    case CSR_MSCRATCH:
        cpu[current_thread].mscratch = val;
        break;
    case CSR_MEPC:
        // mepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu[current_thread].mepc = val;
        break;
    case CSR_MCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu[current_thread].mcause = val;
        break;
    case CSR_MTVAL:
        val = sextVA(val);
        cpu[current_thread].mtval = val;
        break;
    case CSR_MIP:
        // Only bus_error, mbad_red, seip, stip, ssip are writeable
        val &= 0x0000000000810222ULL;
        cpu[current_thread].mip = val;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        if (cpu[current_thread].debug_mode) {
            // Preserve type, maskmax, timing; clearing dmode clears action too
            val = (val & 0x08000000000010DFULL) | (cpu[current_thread].tdata1 & 0xF7E0000000040000ULL);
            if (~val & 0x0800000000000000ULL)
            {
                val &= ~0x000000000000F000ULL;
            }
            set_tdata1(cpu[current_thread], val);
        }
        else if (~cpu[current_thread].tdata1 & 0x0800000000000000ULL) {
            // Preserve type, dmode, maskmax, timing, action
            val = (val & 0x00000000000000DFULL) | (cpu[current_thread].tdata1 & 0xFFE000000004F000ULL);
            set_tdata1(cpu[current_thread], val);
        }
        else {
            // Ignore writes to the register
            val = cpu[current_thread].tdata1;
        }
        break;
    case CSR_TDATA2:
        // keep only valid virtual or pysical addresses
        if ((~cpu[current_thread].tdata1 & 0x0800000000000000ULL) || cpu[current_thread].debug_mode) {
            val &= VA_M;
            cpu[current_thread].tdata2 = val;
        } else {
            val = cpu[current_thread].tdata2;
        }
        break;
        // unimplemented: TDATA3
        // TODO: DCSR
        // TODO: DPC
        // unimplemented: DSCRATCH0
        // unimplemented: DSCRATCH1
    case CSR_MCYCLE:
    case CSR_MINSTRET:
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
    case CSR_MHPMCOUNTER9:
    case CSR_MHPMCOUNTER10:
    case CSR_MHPMCOUNTER11:
    case CSR_MHPMCOUNTER12:
    case CSR_MHPMCOUNTER13:
    case CSR_MHPMCOUNTER14:
    case CSR_MHPMCOUNTER15:
    case CSR_MHPMCOUNTER16:
    case CSR_MHPMCOUNTER17:
    case CSR_MHPMCOUNTER18:
    case CSR_MHPMCOUNTER19:
    case CSR_MHPMCOUNTER20:
    case CSR_MHPMCOUNTER21:
    case CSR_MHPMCOUNTER22:
    case CSR_MHPMCOUNTER23:
    case CSR_MHPMCOUNTER24:
    case CSR_MHPMCOUNTER25:
    case CSR_MHPMCOUNTER26:
    case CSR_MHPMCOUNTER27:
    case CSR_MHPMCOUNTER28:
    case CSR_MHPMCOUNTER29:
    case CSR_MHPMCOUNTER30:
    case CSR_MHPMCOUNTER31:
        val = 0;
        break;
    case CSR_CYCLE:
    case CSR_INSTRET:
    case CSR_HPMCOUNTER3:
    case CSR_HPMCOUNTER4:
    case CSR_HPMCOUNTER5:
    case CSR_HPMCOUNTER6:
    case CSR_HPMCOUNTER7:
    case CSR_HPMCOUNTER8:
    case CSR_HPMCOUNTER9:
    case CSR_HPMCOUNTER10:
    case CSR_HPMCOUNTER11:
    case CSR_HPMCOUNTER12:
    case CSR_HPMCOUNTER13:
    case CSR_HPMCOUNTER14:
    case CSR_HPMCOUNTER15:
    case CSR_HPMCOUNTER16:
    case CSR_HPMCOUNTER17:
    case CSR_HPMCOUNTER18:
    case CSR_HPMCOUNTER19:
    case CSR_HPMCOUNTER20:
    case CSR_HPMCOUNTER21:
    case CSR_HPMCOUNTER22:
    case CSR_HPMCOUNTER23:
    case CSR_HPMCOUNTER24:
    case CSR_HPMCOUNTER25:
    case CSR_HPMCOUNTER26:
    case CSR_HPMCOUNTER27:
    case CSR_HPMCOUNTER28:
    case CSR_HPMCOUNTER29:
    case CSR_HPMCOUNTER30:
    case CSR_HPMCOUNTER31:
    case CSR_MVENDORID:
    case CSR_MARCHID:
    case CSR_MIMPID:
    case CSR_MHARTID:
        throw trap_illegal_instruction(current_inst);
        break;
        // ----- Esperanto registers -------------------------------------
    case CSR_MATP: // Shared register
        // do not write the register if it is locked (L==1)
        if (~cpu[current_thread].matp & 0x800000000000000ULL) {
            // MODE is 4 bits, L is 1 bits, ASID is 0bits, PPN is PPN_M bits
            val &= 0xF800000000000000ULL | PPN_M;
            switch (val >> 60) {
            case MATP_MODE_BARE:
            case MATP_MODE_MV39:
            case MATP_MODE_MV48:
                cpu[current_thread].matp = val;
                if ((current_thread^1) < EMU_NUM_THREADS)
                    cpu[current_thread^1].matp = val;
                break;
            default: // reserved
                // do not write the register if attempting to set an unsupported mode
                break;
            }
        }
        break;
    case CSR_MINSTMASK:
        val &= 0x1ffffffffULL;
        cpu[current_thread].minstmask = val;
        break;
    case CSR_MINSTMATCH:
        val &= 0xffffffff;
        cpu[current_thread].minstmatch = val;
        break;
        // TODO: CSR_AMOFENCE_CTRL
    case CSR_CACHE_INVALIDATE:
        val &= 0x3;
        break;
    case CSR_MENABLE_SHADOWS:
        val &= 1;
        cpu[current_thread].menable_shadows = val;
        if ((current_thread^1) < EMU_NUM_THREADS)
            cpu[current_thread^1].menable_shadows = val;
        break;
    case CSR_EXCL_MODE:
        val &= 1;
        if (val) {
            cpu[current_thread].excl_mode = 1 + ((current_thread & 1) << 1);
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].excl_mode = 1 + ((current_thread & 1) << 1);
        } else {
            cpu[current_thread].excl_mode = 0;
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].excl_mode = 0;
        }
        break;
    case CSR_MBUSADDR:
        val = zextPA(val);
        cpu[current_thread].mbusaddr = val;
        break;
    case CSR_MCACHE_CONTROL:
#ifdef SYS_EMU
        orig_mcache_control = cpu[current_thread].mcache_control & 0x3;
#endif
        switch (cpu[current_thread].mcache_control) {
        case  0: msk = ((val & 3) == 1) ? 3 : 0; break;
        case  1: msk = ((val & 3) != 2) ? 3 : 0; break;
        case  3: msk = ((val & 3) != 2) ? 3 : 0; break;
        default: assert(0); break;
        }
        val = (val & msk) | (cpu[current_thread].ucache_control & ~msk);
        if (msk) {
            dcache_change_mode(cpu[current_thread].mcache_control, val);
            cpu[current_thread].ucache_control = val;
            cpu[current_thread].mcache_control = val & 3;
            if ((current_thread^1) < EMU_NUM_THREADS) {
                cpu[current_thread^1].ucache_control = val;
                cpu[current_thread^1].mcache_control = val & 3;
            }
            if (~val & 2)
                cpu[current_thread].tensorload_setupb_topair = false;
        }
        val &= 3;
#ifdef SYS_EMU
        if (sys_emu::get_mem_check() && (orig_mcache_control != (cpu[current_thread].mcache_control & 0x3))) {
            sys_emu::get_mem_checker().mcache_control_up(
                (current_thread >> 1) / EMU_MINIONS_PER_SHIRE,
                (current_thread >> 1) % EMU_MINIONS_PER_SHIRE,
                cpu[current_thread].mcache_control);
        }
#endif
        break;
    case CSR_EVICT_SW:
    case CSR_FLUSH_SW:
        {
            bool tm    = (val >> 63) & 0x1;
            int  dest  = (val >> 58) & 0x3;
            int  set   = (val >> 14) & 0xF;
            int  way   = (val >>  6) & 0x3;
            int  count = (val & 0xF) + 1;
            dcache_evict_flush_set_way(csr == CSR_EVICT_SW, tm, dest, set, way, count);
        }
        break;
    case CSR_LOCK_SW:
        {
            int      way   = (val >> 55) & 0x3;
            uint64_t paddr = val & 0x000000FFFFFFFFC0ULL;
            dcache_lock_paddr(way, paddr);
        }
        break;
    case CSR_UNLOCK_SW:
        {
            int way = (val >> 55) & 0x3;
            int set = (val >>  6) & 0xF;
            dcache_unlock_set_way(set, way);
        }
        break;
    case CSR_TENSOR_REDUCE:
        require_feature_ml_on_thread0();
        tensor_reduce_start(val);
        break;
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        switch ((val >> 1) & 0x7)
        {
        case  0: tensor_fma32_start(val); break;
        case  1: tensor_fma16a32_start(val); break;
        case  3: tensor_ima8a32_start(val); break;
        default: throw trap_illegal_instruction(current_inst); break;
        }
        break;
    case CSR_TENSOR_CONV_SIZE:
        require_feature_ml();
        val &= 0xFF00FFFFFF00FFFFULL;
        cpu[current_thread].tensor_conv_size = val;
        tensor_mask_update();
        break;
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val &= 0x0000FFFF0000FFFFULL;
        cpu[current_thread].tensor_conv_ctrl = val;
        tensor_mask_update();
        break;
    case CSR_TENSOR_COOP:
        require_feature_ml_on_thread0();
        val &= 0x0000000000FFFF0FULL;
        tensor_coop_write(val);
        break;
    case CSR_TENSOR_MASK:
        require_feature_ml();
        val &= 0xffff;
        cpu[current_thread].tensor_mask = val;
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        tensor_quant_start(val);
        break;
    case CSR_TEX_SEND:
        require_feature_gfx();
        //val &= 0xff;
        // Notify to TBOX that a Sample Request is ready
        // Thanks for making the code unreadable
        new_sample_request(current_thread,
                           val & 0xf,           // port_id
                           (val >> 4) & 0xf,    // num_packets
                           read_port_base_address(current_thread, val & 0xf /* port id */));
        break;
    case CSR_TENSOR_ERROR:
        val &= 0x3ff;
        cpu[current_thread].tensor_error = val;
        log_tensor_error_value(val);
        break;
    case CSR_UCACHE_CONTROL:
#ifdef SYS_EMU
        orig_mcache_control = cpu[current_thread].mcache_control & 0x3;
#endif
        require_feature_u_scratchpad();
        msk = (!(current_thread % EMU_THREADS_PER_MINION)
               && (cpu[current_thread].mcache_control & 1)) ? 1 : 3;
        val = (cpu[current_thread].mcache_control & msk) | (val & ~msk & 0x07df);
        assert((val & 3) != 2);
        dcache_change_mode(cpu[current_thread].mcache_control, val);
        cpu[current_thread].ucache_control = val;
        cpu[current_thread].mcache_control = val & 3;
        if ((current_thread^1) < EMU_NUM_THREADS) {
            cpu[current_thread^1].ucache_control = val;
            cpu[current_thread^1].mcache_control = val & 3;
        }
#ifdef SYS_EMU
        if(sys_emu::get_mem_check() && (orig_mcache_control != (cpu[current_thread].mcache_control & 0x3))) {
            sys_emu::get_mem_checker().mcache_control_up(
                (current_thread >> 1) / EMU_MINIONS_PER_SHIRE,
                (current_thread >> 1) % EMU_MINIONS_PER_SHIRE,
                cpu[current_thread].mcache_control);
        }
#endif
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        dcache_prefetch_vaddr(val);
        break;
        // CSR_FLB is modelled outside this fuction!
    case CSR_FCC:
        require_feature_ml();
        cpu[current_thread].fcc_cnt = val % 2;
#ifdef SYS_EMU
        // If you are not going to block decrement it
        if (cpu[current_thread].fcc[val % 2] != 0)
            cpu[current_thread].fcc[val % 2]--;
#else
        // block if no credits, else decrement
        if (cpu[current_thread].fcc[val % 2] == 0 ) {
            cpu[current_thread].fcc_wait = true;
            throw std::domain_error("FCC write with no credits");
        } else {
            cpu[current_thread].fcc[val % 2]--;
        }
#endif
        break;
    case CSR_STALL:
        require_feature_ml();
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_WAIT:
        tensor_wait_start(val);
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        tensor_load_start(val);
        break;
    case CSR_GSC_PROGRESS:
        val &= (VL-1);
        cpu[current_thread].gsc_progress = val;
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        tensor_load_l2_start(val);
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        tensor_store_start(val);
        break;
    case CSR_EVICT_VA:
    case CSR_FLUSH_VA:
        require_feature_u_cacheops();
        {
            bool     tm     = (val >> 63) & 0x1;
            int      dest   = (val >> 58) & 0x3;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0x0F) + 1;
            uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
            int      id     = X31 & 0x0000000000000001ULL;
            LOG_REG(":", 31);
            dcache_evict_flush_vaddr(csr == CSR_EVICT_VA, tm, dest, vaddr, count, id, stride);
        }
        break;
    case CSR_VALIDATION0:
        cpu[current_thread].validation0 = val;
#ifdef SYS_EMU
        switch (val) {
        case 0x1FEED000:
            LOG_ALL_MINIONS(INFO, "%s", "Signal end test with PASS");
            sys_emu::deactivate_thread(current_thread);
            break;
        case 0x50BAD000:
            LOG_ALL_MINIONS(INFO, "%s", "Signal end test with FAIL");
            m_emu_done = true;
            break;
        }
#endif
        break;
    case CSR_VALIDATION1:
        switch ((val >> 56) & 0xFF) {
        case ET_DIAG_PUTCHAR:
            val = val & 0xFF;
            // EOT signals end of test
            if (val == 4) {
                LOG(INFO, "%s", "Validation1 CSR received End Of Transmission.");
                m_emu_done = true;
                break;
            }
            if (char(val) != '\n') {
                uart_stream[current_thread] << char(val);
            } else {
                // If line feed, flush to stdout
                std::cout << uart_stream[current_thread].str() << std::endl;
                uart_stream[current_thread].str("");
                uart_stream[current_thread].clear();
            }
            break;
#ifdef SYS_EMU
        case ET_DIAG_IRQ_INJ:
            sys_emu::evl_dv_handle_irq_inj((val >> 55) & 1, (val >> 53) & 3, val & 0x3FFFFFFFFULL);
            break;
        case ET_DIAG_CYCLE:
            cpu[current_thread].validation1 = (val >> 56) & 0xFF;
            break;
#endif
        default:
            break;
        }
        break;
    case CSR_VALIDATION2:
        cpu[current_thread].validation2 = val;
        break;
    case CSR_VALIDATION3:
        cpu[current_thread].validation3 = val;
        break;
    case CSR_LOCK_VA:
        require_lock_unlock_enabled();
        {
            bool     tm     = (val >> 63) & 0x1;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0xF) + 1;
            uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
            int      id     = X31 & 0x0000000000000001ULL;
            LOG_REG(":", 31);
            dcache_lock_vaddr(tm, vaddr, count, id, stride);
        }
        break;
    case CSR_UNLOCK_VA:
        require_lock_unlock_enabled();
        val &= 0xC000FFFFFFFFFFCFULL;
        {
            bool     tm     = (val >> 63) & 0x1;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0xF) + 1;
            uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
            int      id     = X31 & 0x0000000000000001ULL;
            LOG_REG(":", 31);
            dcache_unlock_vaddr(tm, vaddr, count, id, stride);
        }
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = legalize_portctrl(val);
        cpu[current_thread].portctrl[csr - CSR_PORTCTRL0] = val;
        configure_port(csr - CSR_PORTCTRL0, val);
        break;
    case CSR_FCCNB:
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
    case CSR_HARTID:
        throw trap_illegal_instruction(current_inst);
        // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }

    return val;
}


static inline void csrswap(uint16_t csr, uint64_t& oldval, uint64_t& newval)
{
    if (csr == CSR_FLB) {
        require_feature_ml();
        oldval = write_flb(newval);
    } else {
        newval = csrset(csr, newval);
    }
}


static inline uint64_t external_supervisor_software_interrupt(uint16_t csr)
{
    switch (csr) {
    case CSR_SIP:
        return cpu[current_thread].ext_seip & cpu[current_thread].mideleg;
    case CSR_MIP:
        return cpu[current_thread].ext_seip;
    default:
        return 0;
    }
}


void insn_csrrc(insn_t inst)
{
    DISASM_RD_CSR_RS1("csrrc");

    uint16_t csr = inst.csrimm();
    xreg     rd  = inst.rd();
    xreg     rs1 = inst.rs1();

    check_csr_privilege(inst, csr);

    uint64_t oldval = csrget(csr);
    if (rs1 != x0) {
        uint64_t newval = oldval & (~RS1);
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}



void insn_csrrci(insn_t inst)
{
    DISASM_RD_CSR_UIMM5("csrrci");

    uint16_t csr = inst.csrimm();
    xreg     rd  = inst.rd();
    uint64_t imm = inst.uimm5();

    check_csr_privilege(inst, csr);

    uint64_t oldval = csrget(csr);
    if (imm != 0) {
        uint64_t newval = oldval & (~imm);
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrs(insn_t inst)
{
    DISASM_RD_CSR_RS1("csrrs");

    uint16_t csr = inst.csrimm();
    xreg     rd  = inst.rd();
    xreg     rs1 = inst.rs1();

    check_csr_privilege(inst, csr);

    uint64_t oldval = csrget(csr);
    if (rs1 != x0) {
        uint64_t newval = oldval | RS1;
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrsi(insn_t inst)
{
    DISASM_RD_CSR_UIMM5("csrrsi");

    uint16_t csr = inst.csrimm();
    xreg     rd  = inst.rd();
    uint64_t imm = inst.uimm5();

    check_csr_privilege(inst, csr);

    uint64_t oldval = csrget(csr);
    if (imm != 0) {
        uint64_t newval = oldval | imm;
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrw(insn_t inst)
{
    DISASM_RD_CSR_RS1("csrrw");

    uint16_t csr    = inst.csrimm();
    xreg     rd     = inst.rd();
    uint64_t newval = RS1;

    check_csr_privilege(inst, csr);

    uint64_t oldval = 0;
    if (rd != x0) {
        oldval = csrget(csr);
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        csrswap(csr, oldval, newval);
        LOG_CSR("=", csr, newval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrwi(insn_t inst)
{
    DISASM_RD_CSR_UIMM5("csrrwi");

    uint16_t csr    = inst.csrimm();
    xreg     rd     = inst.rd();
    uint64_t newval = inst.uimm5();

    check_csr_privilege(inst, csr);

    uint64_t oldval = 0;
    if (rd != x0) {
        oldval = csrget(csr);
        csrswap(csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(csr);
    } else {
        csrswap(csr, oldval, newval);
        LOG_CSR("=", csr, newval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


uint64_t get_csr(unsigned thread, uint16_t csr)
{
    uint64_t retval = 0;
    std::swap(thread, current_thread);
    try {
        retval = csrget(csr);
    } catch (const trap_t&) {
        /* do nothing */
    }
    std::swap(thread, current_thread);
    return retval;
}


void set_csr(unsigned thread, uint16_t csr, uint64_t value)
{
    std::swap(thread, current_thread);
    try {
        csrset(csr, value);
    } catch (const trap_t&) {
        /* do nothing */
    }
    std::swap(thread, current_thread);
}


//} // namespace bemu
