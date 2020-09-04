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
#include "emu_defines.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "tensor.h"
#include "traps.h"
#include "txs.h"
#include "utility.h"


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


namespace bemu {


extern bool m_emu_done;


// Fast local barrier
uint64_t write_flb(const Hart&, uint64_t);

// Messaging
uint32_t read_port_control(const Hart&, unsigned);
int64_t read_port_head(Hart&, unsigned, bool);
uint32_t legalize_portctrl(uint32_t);
void configure_port(Hart&, unsigned, uint32_t);
uint64_t read_port_base_address(unsigned , unsigned);

// Cache management
void dcache_change_mode(Hart&, uint8_t);
void dcache_evict_flush_set_way(Hart&, bool, uint64_t);
void dcache_evict_flush_vaddr(Hart&, bool, uint64_t);
void dcache_prefetch_vaddr(Hart&, uint64_t);
void dcache_lock_vaddr(Hart&, uint64_t);
void dcache_unlock_vaddr(Hart&, uint64_t);
void dcache_lock_paddr(Hart&, uint64_t);
void dcache_unlock_set_way(Hart&, uint64_t);

// Tensor extension
void tensor_coop_write(Hart&, uint64_t);
void tensor_fma16a32_start(Hart&, uint64_t);
void tensor_fma32_start(Hart&, uint64_t);
void tensor_ima8a32_start(Hart&, uint64_t);
void tensor_load_l2_start(Hart&, uint64_t);
void tensor_load_start(Hart&, uint64_t);
void tensor_mask_update(Hart&);
void tensor_quant_start(Hart&, uint64_t);
void tensor_reduce_start(Hart&, uint64_t);
void tensor_store_start(Hart&, uint64_t);


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


static inline void check_csr_privilege(const Hart& cpu, uint16_t csr)
{
    int curprv = PRV;
    int csrprv = (csr >> 8) & 3;

    if (csrprv > curprv)
        throw trap_illegal_instruction(cpu.inst.bits);

    if ((csr == CSR_SATP) && (curprv == PRV_S) && ((cpu.mstatus >> 20) & 1))
        throw trap_illegal_instruction(cpu.inst.bits);
}


static void check_counter_is_enabled(const Hart& cpu, int n)
{
    uint64_t enabled = (cpu.mcounteren & (1 << n));

    switch (PRV) {
    case PRV_U:
        if ((cpu.scounteren & enabled) == 0)
            throw trap_illegal_instruction(cpu.inst.bits);
        break;
    case PRV_S:
        if (enabled == 0)
            throw trap_illegal_instruction(cpu.inst.bits);
        break;
    default:
        break;
    }
}


static uint64_t csrget(Hart& cpu, uint16_t csr)
{
    uint64_t val;

    switch (csr) {
    case CSR_FFLAGS:
        require_fp_active();
        val = cpu.fcsr & 0x8000001f;
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu.fcsr >> 5) & 0x7;
        break;
    case CSR_FCSR:
        require_fp_active();
        val = cpu.fcsr;
        break;
    case CSR_SSTATUS:
        // Hide sxl, tsr, tw, tvm, mprv, mpp, mpie, mie
        val = cpu.mstatus & 0x80000003000DE133ULL;
        break;
    case CSR_SIE:
        val = cpu.mie & cpu.mideleg;
        break;
    case CSR_STVEC:
        val = cpu.stvec;
        break;
    case CSR_SCOUNTEREN:
        val = cpu.scounteren;
        break;
    case CSR_SSCRATCH:
        val = cpu.sscratch;
        break;
    case CSR_SEPC:
        val = cpu.sepc;
        break;
    case CSR_SCAUSE:
        val = cpu.scause;
        break;
    case CSR_STVAL:
        val = cpu.stval;
        break;
    case CSR_SIP:
        val = cpu.mip & cpu.mideleg;
        break;
    case CSR_SATP:
        val = cpu.core->satp;
        break;
    case CSR_MSTATUS:
        val = cpu.mstatus;
        break;
    case CSR_MISA:
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        val = cpu.medeleg;
        break;
    case CSR_MIDELEG:
        val = cpu.mideleg;
        break;
    case CSR_MIE:
        val = cpu.mie;
        break;
    case CSR_MTVEC:
        val = cpu.mtvec;
        break;
    case CSR_MCOUNTEREN:
        val = cpu.mcounteren;
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val = cpu.mhpmevent[csr - CSR_MHPMEVENT3];
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
        val = cpu.mscratch;
        break;
    case CSR_MEPC:
        val = cpu.mepc;
        break;
    case CSR_MCAUSE:
        val = cpu.mcause;
        break;
    case CSR_MTVAL:
        val = cpu.mtval;
        break;
    case CSR_MIP:
        val = cpu.mip;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        val = cpu.tdata1;
        break;
    case CSR_TDATA2:
        val = cpu.tdata2;
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
        check_counter_is_enabled(cpu, csr - CSR_CYCLE);
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
        throw trap_illegal_instruction(cpu.inst.bits);
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
        val = cpu.mhartid;
        break;
        // ----- Esperanto registers -------------------------------------
    case CSR_MATP:
        val = cpu.core->matp;
        break;
    case CSR_MINSTMASK:
        val = cpu.minstmask;
        break;
    case CSR_MINSTMATCH:
        val = cpu.minstmatch;
        break;
    case CSR_CACHE_INVALIDATE:
        val = 0;
        break;
    case CSR_MENABLE_SHADOWS:
        val = cpu.core->menable_shadows;
        break;
    case CSR_EXCL_MODE:
        val = cpu.core->excl_mode & 1;
        break;
    case CSR_MBUSADDR:
        val = cpu.mbusaddr;
        break;
    case CSR_MCACHE_CONTROL:
        val = cpu.core->mcache_control;
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
        val = cpu.tensor_mask.to_ulong();
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
        val = cpu.tensor_error;
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        val = cpu.core->ucache_control;
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
        val = cpu.gsc_progress;
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
        val = cpu.validation0;
        break;
    case CSR_VALIDATION1:
#ifdef SYS_EMU
        val = (cpu.validation1 == ET_DIAG_CYCLE) ? sys_emu::get_emu_cycle() : 0;
#else
        val = 0;
#endif
        break;
    case CSR_VALIDATION2:
        val = cpu.validation2;
        break;
    case CSR_VALIDATION3:
        val = cpu.validation3;
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
        val = read_port_control(cpu, csr - CSR_PORTCTRL0);
        break;
    case CSR_FCCNB:
        require_feature_ml();
        val = (uint64_t(cpu.fcc[1]) << 16) + uint64_t(cpu.fcc[0]);
        break;
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
        val = read_port_head(cpu, csr - CSR_PORTHEAD0, true);
        break;
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
        val = read_port_head(cpu, csr - CSR_PORTHEADNB0, false);
        break;
    case CSR_HARTID:
        if (PRV != PRV_M && (cpu.core->menable_shadows & 1) == 0) {
            throw trap_illegal_instruction(cpu.inst.bits);
        }
        val = cpu.mhartid;
        break;
    case CSR_DCACHE_DEBUG:
        val = 0;
        break;
        // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(cpu.inst.bits);
    }
    return val;
}


static uint64_t csrset(Hart& cpu, uint16_t csr, uint64_t val)
{
    uint64_t msk;
#ifdef SYS_EMU
    int orig_mcache_control;
#endif

    switch (csr) {
    case CSR_FFLAGS:
        require_fp_active();
        val = (cpu.fcsr & 0x000000E0) | (val & 0x8000001F);
        cpu.fcsr = val;
        dirty_fp_state();
        // Return 'fflags' view of 'fcsr'
        val &= 0x8000001f;
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu.fcsr & 0x8000001F) | ((val & 0x7) << 5);
        cpu.fcsr = val;
        dirty_fp_state();
        // Return 'frm' view of 'fcsr'
        val = (val >> 5) & 0x7;
        break;
    case CSR_FCSR:
        require_fp_active();
        val &= 0x800000FF;
        cpu.fcsr = val;
        dirty_fp_state();
        break;
    case CSR_SSTATUS:
        // Preserve sd, sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
        // Modify mxr, sum, fs, spp, spie, (upie=0), sie, (uie=0)
        val = (val & 0x00000000000C6122ULL) | (cpu.mstatus & 0x0000000F00739888ULL);
        // Setting fs=1 or fs=2 will set fs=3
        if (val & 0x6000ULL) {
            val |= 0x6000ULL;
        }
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3)) {
            val |= 0x8000000000000000ULL;
        }
        cpu.mstatus = val;
        // Return 'sstatus' view of 'mstatus'
        val &= 0x80000003000DE133ULL;
        break;
    case CSR_SIE:
        // Only ssie, stie, and seie are writeable, and only if they are delegated
        // if mideleg[sei,sti,ssi]==1 then seie, stie, ssie is writeable, otherwise they are reserved
        msk = cpu.mideleg & 0x0000000000000222ULL;
        val = (cpu.mie & ~msk) | (val & msk);
        cpu.mie = val;
        // Return 'sie' view of 'mie'
        val &= cpu.mideleg;
        break;
    case CSR_STVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu.stvec = val;
        cpu.stvec_is_set = true;
        break;
    case CSR_SCOUNTEREN:
        val &= 0x1FF;
        cpu.scounteren = uint16_t(val);
        break;
    case CSR_SSCRATCH:
        cpu.sscratch = val;
        break;
    case CSR_SEPC:
        // sepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu.sepc = val;
        break;
    case CSR_SCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu.scause = val;
        break;
    case CSR_STVAL:
        val = sextVA(val);
        cpu.stval = val;
        break;
    case CSR_SIP:
        // Only ssip is writeable, and only if it is delegated
        msk = cpu.mideleg & 0x0000000000000002ULL;
        val = (cpu.mip & ~msk) | (val & msk);
        cpu.mip = val;
        // Return 'sip' view of 'mip'
        val &= cpu.mideleg;
        break;
    case CSR_SATP: // Shared register
        // MODE is 4 bits, ASID is 0bits, PPN is PPN_M bits
        val &= 0xF000000000000000ULL | PPN_M;
        switch (val >> 60) {
        case SATP_MODE_BARE:
        case SATP_MODE_SV39:
        case SATP_MODE_SV48:
            cpu.core->satp = val;
            break;
        default: // reserved
            // do not write the register if attempting to set an unsupported mode
            break;
        }
        break;
    case CSR_MSTATUS:
        // Preserve sd, sxl, uxl, xs
        // Write all others (except upie=0, uie=0)
        val = (val & 0x00000000007E79AAULL) | (cpu.mstatus & 0x0000000F00018000ULL);
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
        cpu.mstatus = val;
        break;
    case CSR_MISA:
        // Writeable but hardwired
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        // Not all exceptions can be delegated
        val &= 0x0000000000000B108ULL;
        cpu.medeleg = val;
        break;
    case CSR_MIDELEG:
        // Not all interrupts can be delegated
        val &= 0x0000000000000222ULL;
        cpu.mideleg = val;
        break;
    case CSR_MIE:
        // Hard-wire ueie, utie, usie
        val &= 0x0000000000890AAAULL;
        cpu.mie = val;
        break;
    case CSR_MTVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu.mtvec = val;
        cpu.mtvec_is_set = true;
        break;
    case CSR_MCOUNTEREN:
        val &= 0x1FF;
        cpu.mcounteren = uint16_t(val);
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val &= 0x1F;
        cpu.mhpmevent[csr - CSR_MHPMEVENT3] = val;
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
        cpu.mscratch = val;
        break;
    case CSR_MEPC:
        // mepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu.mepc = val;
        break;
    case CSR_MCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu.mcause = val;
        break;
    case CSR_MTVAL:
        val = sextVA(val);
        cpu.mtval = val;
        break;
    case CSR_MIP:
        // Only bus_error, mbad_red, seip, stip, ssip are writeable
        val &= 0x0000000000810222ULL;
        cpu.mip = val;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        if (cpu.debug_mode) {
            // Preserve type, maskmax, timing; clearing dmode clears action too
            val = (val & 0x08000000000010DFULL) | (cpu.tdata1 & 0xF7E0000000040000ULL);
            if (~val & 0x0800000000000000ULL)
            {
                val &= ~0x000000000000F000ULL;
            }
            cpu.set_tdata1(val);
        }
        else if (~cpu.tdata1 & 0x0800000000000000ULL) {
            // Preserve type, dmode, maskmax, timing, action
            val = (val & 0x00000000000000DFULL) | (cpu.tdata1 & 0xFFE000000004F000ULL);
            cpu.set_tdata1(val);
        }
        else {
            // Ignore writes to the register
            val = cpu.tdata1;
        }
        break;
    case CSR_TDATA2:
        // keep only valid virtual or pysical addresses
        if ((~cpu.tdata1 & 0x0800000000000000ULL) || cpu.debug_mode) {
            val &= VA_M;
            cpu.tdata2 = val;
        } else {
            val = cpu.tdata2;
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
        throw trap_illegal_instruction(cpu.inst.bits);
        break;
        // ----- Esperanto registers -------------------------------------
    case CSR_MATP: // Shared register
        // do not write the register if it is locked (L==1)
        if (~cpu.core->matp & 0x800000000000000ULL) {
            // MODE is 4 bits, L is 1 bits, ASID is 0bits, PPN is PPN_M bits
            val &= 0xF800000000000000ULL | PPN_M;
            switch (val >> 60) {
            case MATP_MODE_BARE:
            case MATP_MODE_MV39:
            case MATP_MODE_MV48:
                cpu.core->matp = val;
                break;
            default: // reserved
                // do not write the register if attempting to set an unsupported mode
                break;
            }
        }
        break;
    case CSR_MINSTMASK:
        val &= 0x1ffffffffULL;
        cpu.minstmask = val;
        break;
    case CSR_MINSTMATCH:
        val &= 0xffffffff;
        cpu.minstmatch = val;
        break;
        // TODO: CSR_AMOFENCE_CTRL
    case CSR_CACHE_INVALIDATE:
        val &= 0x3;
        break;
    case CSR_MENABLE_SHADOWS:
        val &= 1;
        cpu.core->menable_shadows = val;
        break;
    case CSR_EXCL_MODE:
        val &= 1;
        if (val) {
            cpu.core->excl_mode = 1 + ((cpu.mhartid & 1) << 1);
        } else {
            cpu.core->excl_mode = 0;
        }
        break;
    case CSR_MBUSADDR:
        val = zextPA(val);
        cpu.mbusaddr = val;
        break;
    case CSR_MCACHE_CONTROL:
#ifdef SYS_EMU
        orig_mcache_control = cpu.core->mcache_control & 0x3;
#endif
        switch (cpu.core->mcache_control) {
        case  0: msk = ((val & 3) == 1) ? 3 : 0; break;
        case  1: msk = ((val & 3) != 2) ? 3 : 0; break;
        case  3: msk = ((val & 3) != 2) ? 3 : 0; break;
        default: assert(0); break;
        }
        val = (val & msk) | (cpu.core->ucache_control & ~msk);
        if (msk) {
            dcache_change_mode(cpu, val);
            cpu.core->ucache_control = val;
            cpu.core->mcache_control = val & 3;
            if (~val & 2)
                cpu.core->tensorload_setupb_topair = false;
        }
        val &= 3;
#ifdef SYS_EMU
        if (sys_emu::get_mem_check() && (orig_mcache_control != (cpu.core->mcache_control & 0x3))) {
            sys_emu::get_mem_checker().mcache_control_up(
                (cpu.mhartid / EMU_THREADS_PER_MINION) / EMU_MINIONS_PER_SHIRE,
                (cpu.mhartid / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE,
                cpu.core->mcache_control);
        }
#endif
        break;
    case CSR_EVICT_SW:
        dcache_evict_flush_set_way(cpu, true, val);
        break;
    case CSR_FLUSH_SW:
        dcache_evict_flush_set_way(cpu, false, val);
        break;
    case CSR_LOCK_SW:
        dcache_lock_paddr(cpu, val);
        break;
    case CSR_UNLOCK_SW:
        dcache_unlock_set_way(cpu, val);
        break;
    case CSR_TENSOR_REDUCE:
        require_feature_ml_on_thread0();
        tensor_reduce_start(cpu, val);
        break;
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        switch ((val >> 1) & 0x7)
        {
        case  0: tensor_fma32_start(cpu, val); break;
        case  1: tensor_fma16a32_start(cpu, val); break;
        case  3: tensor_ima8a32_start(cpu, val); break;
        default: throw trap_illegal_instruction(cpu.inst.bits); break;
        }
        break;
    case CSR_TENSOR_CONV_SIZE:
        require_feature_ml();
        val &= 0xFF00FFFFFF00FFFFULL;
        cpu.tensor_conv_size = val;
        tensor_mask_update(cpu);
        break;
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val &= 0x0000FFFF0000FFFFULL;
        cpu.tensor_conv_ctrl = val;
        tensor_mask_update(cpu);
        break;
    case CSR_TENSOR_COOP:
        require_feature_ml_on_thread0();
        val &= 0x0000000000FFFF0FULL;
        tensor_coop_write(cpu, val);
        break;
    case CSR_TENSOR_MASK:
        require_feature_ml();
        val &= 0xffff;
        cpu.tensor_mask = val;
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        tensor_quant_start(cpu, val);
        break;
    case CSR_TEX_SEND:
        require_feature_gfx();
        //val &= 0xff;
        // Notify to TBOX that a Sample Request is ready
        // Thanks for making the code unreadable
        new_sample_request(hart_index(cpu),
                           val & 0xf,           // port_id
                           (val >> 4) & 0xf,    // num_packets
                           read_port_base_address(hart_index(cpu), val & 0xf /* port id */));
        break;
    case CSR_TENSOR_ERROR:
        val &= 0x3ff;
        cpu.tensor_error = val;
        notify_tensor_error_value(cpu, val);
        break;
    case CSR_UCACHE_CONTROL:
#ifdef SYS_EMU
        orig_mcache_control = cpu.core->mcache_control & 0x3;
#endif
        require_feature_u_scratchpad();
        msk = (!(cpu.mhartid % EMU_THREADS_PER_MINION)
               && (cpu.core->mcache_control & 1)) ? 1 : 3;
        val = (cpu.core->mcache_control & msk) | (val & ~msk & 0x07df);
        assert((val & 3) != 2);
        dcache_change_mode(cpu, val);
        cpu.core->ucache_control = val;
        cpu.core->mcache_control = val & 3;
#ifdef SYS_EMU
        if(sys_emu::get_mem_check() && (orig_mcache_control != (cpu.core->mcache_control & 0x3))) {
            sys_emu::get_mem_checker().mcache_control_up(
                (cpu.mhartid / EMU_THREADS_PER_MINION) / EMU_MINIONS_PER_SHIRE,
                (cpu.mhartid / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE,
                cpu.core->mcache_control);
        }
#endif
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        dcache_prefetch_vaddr(cpu, val);
        break;
        // CSR_FLB is modelled outside this fuction!
    case CSR_FCC:
        require_feature_ml();
        cpu.fcc_cnt = val % 2;
#ifdef SYS_EMU
        // If you are not going to block decrement it
        if (cpu.fcc[val % 2] != 0)
            cpu.fcc[val % 2]--;
#else
        // block if no credits, else decrement
        if (cpu.fcc[val % 2] == 0 ) {
            cpu.fcc_wait = true;
            throw std::domain_error("FCC write with no credits");
        } else {
            cpu.fcc[val % 2]--;
        }
#endif
        break;
    case CSR_STALL:
        require_feature_ml();
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_WAIT:
        tensor_wait_start(cpu, val);
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        tensor_load_start(cpu, val);
        break;
    case CSR_GSC_PROGRESS:
        val &= (VLENW-1);
        cpu.gsc_progress = val;
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        tensor_load_l2_start(cpu, val);
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        tensor_store_start(cpu, val);
        break;
    case CSR_EVICT_VA:
        require_feature_u_cacheops();
        dcache_evict_flush_vaddr(cpu, true, val);
        break;
    case CSR_FLUSH_VA:
        require_feature_u_cacheops();
        dcache_evict_flush_vaddr(cpu, false, val);
        break;
    case CSR_VALIDATION0:
        cpu.validation0 = val;
#ifdef SYS_EMU
        switch (val) {
        case 0x1FEED000:
            LOG_AGENT(INFO, cpu, "%s", "Signal end test with PASS");
            sys_emu::deactivate_thread(hart_index(cpu));
            break;
        case 0x50BAD000:
            LOG_AGENT(INFO, cpu, "%s", "Signal end test with FAIL");
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
                LOG_HART(INFO, cpu, "%s", "Validation1 CSR received End Of Transmission.");
                m_emu_done = true;
                break;
            }
            if (char(val) != '\n') {
                uint32_t thread = hart_index(cpu);
                uart_stream[thread] << char(val);
            } else {
                uint32_t thread = hart_index(cpu);
                // If line feed, flush to stdout
                std::cout << uart_stream[thread].str() << std::endl;
                uart_stream[thread].str("");
                uart_stream[thread].clear();
            }
            break;
#ifdef SYS_EMU
        case ET_DIAG_IRQ_INJ:
            sys_emu::evl_dv_handle_irq_inj((val >> 55) & 1, (val >> 53) & 3, val & 0x3FFFFFFFFULL);
            break;
        case ET_DIAG_CYCLE:
            cpu.validation1 = (val >> 56) & 0xFF;
            break;
#endif
        default:
            break;
        }
        break;
    case CSR_VALIDATION2:
        cpu.validation2 = val;
        break;
    case CSR_VALIDATION3:
        cpu.validation3 = val;
        break;
    case CSR_LOCK_VA:
        require_lock_unlock_enabled();
        dcache_lock_vaddr(cpu, val);
        break;
    case CSR_UNLOCK_VA:
        require_lock_unlock_enabled();
        val &= 0xC000FFFFFFFFFFCFULL;
        dcache_unlock_vaddr(cpu, val);
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = legalize_portctrl(val);
        configure_port(cpu, csr - CSR_PORTCTRL0, val);
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
        throw trap_illegal_instruction(cpu.inst.bits);
        // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(cpu.inst.bits);
    }

    return val;
}


static inline void csrswap(Hart& cpu, uint16_t csr, uint64_t& oldval, uint64_t& newval)
{
    if (csr == CSR_FLB) {
        require_feature_ml();
        oldval = write_flb(cpu, newval);
    } else {
        newval = csrset(cpu, csr, newval);
    }
}


static inline uint64_t external_supervisor_software_interrupt(const Hart& cpu, uint16_t csr)
{
    switch (csr) {
    case CSR_SIP:
        return cpu.ext_seip & cpu.mideleg;
    case CSR_MIP:
        return cpu.ext_seip;
    default:
        return 0;
    }
}


void insn_csrrc(Hart& cpu)
{
    DISASM_RD_CSR_RS1("csrrc");

    uint16_t csr = cpu.inst.csrimm();
    xreg     rd  = cpu.inst.rd();
    xreg     rs1 = cpu.inst.rs1();

    check_csr_privilege(cpu, csr);

    uint64_t oldval = csrget(cpu, csr);
    if (rs1 != x0) {
        uint64_t newval = oldval & (~RS1);
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}



void insn_csrrci(Hart& cpu)
{
    DISASM_RD_CSR_UIMM5("csrrci");

    uint16_t csr = cpu.inst.csrimm();
    xreg     rd  = cpu.inst.rd();
    uint64_t imm = cpu.inst.uimm5();

    check_csr_privilege(cpu, csr);

    uint64_t oldval = csrget(cpu, csr);
    if (imm != 0) {
        uint64_t newval = oldval & (~imm);
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrs(Hart& cpu)
{
    DISASM_RD_CSR_RS1("csrrs");

    uint16_t csr = cpu.inst.csrimm();
    xreg     rd  = cpu.inst.rd();
    xreg     rs1 = cpu.inst.rs1();

    check_csr_privilege(cpu, csr);

    uint64_t oldval = csrget(cpu, csr);
    if (rs1 != x0) {
        uint64_t newval = oldval | RS1;
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrsi(Hart& cpu)
{
    DISASM_RD_CSR_UIMM5("csrrsi");

    uint16_t csr = cpu.inst.csrimm();
    xreg     rd  = cpu.inst.rd();
    uint64_t imm = cpu.inst.uimm5();

    check_csr_privilege(cpu, csr);

    uint64_t oldval = csrget(cpu, csr);
    if (imm != 0) {
        uint64_t newval = oldval | imm;
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        LOG_CSR(":", csr, oldval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrw(Hart& cpu)
{
    DISASM_RD_CSR_RS1("csrrw");

    uint16_t csr    = cpu.inst.csrimm();
    xreg     rd     = cpu.inst.rd();
    uint64_t newval = RS1;

    check_csr_privilege(cpu, csr);

    uint64_t oldval = 0;
    if (rd != x0) {
        oldval = csrget(cpu, csr);
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR("=", csr, newval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


void insn_csrrwi(Hart& cpu)
{
    DISASM_RD_CSR_UIMM5("csrrwi");

    uint16_t csr    = cpu.inst.csrimm();
    xreg     rd     = cpu.inst.rd();
    uint64_t newval = cpu.inst.uimm5();

    check_csr_privilege(cpu, csr);

    uint64_t oldval = 0;
    if (rd != x0) {
        oldval = csrget(cpu, csr);
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR(":", csr, oldval);
        LOG_CSR("=", csr, newval);
        oldval |= external_supervisor_software_interrupt(cpu, csr);
    } else {
        csrswap(cpu, csr, oldval, newval);
        LOG_CSR("=", csr, newval);
    }
    WRITE_REG(rd, oldval, csr == CSR_FLB);
}


uint64_t get_csr(unsigned thread, uint16_t csr)
{
    extern std::array<Hart,EMU_NUM_THREADS>  cpu;

    uint64_t retval = 0;
    try {
        retval = csrget(cpu[thread], csr);
    } catch (const trap_t&) {
        /* do nothing */
    }
    return retval;
}


void set_csr(unsigned thread, uint16_t csr, uint64_t value)
{
    extern std::array<Hart,EMU_NUM_THREADS>  cpu;

    try {
        csrset(cpu[thread], csr, value);
    } catch (const trap_t&) {
        /* do nothing */
    }
}


} // namespace bemu
