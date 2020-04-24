/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DECODE_H
#define BEMU_DECODE_H

#include "esrs.h"

#ifdef BEMU_PROFILING
// sys_emu's profiling
#include "profiling.h"
#else
#define PROFILING_WRITE_PC(thread_id, pc) do {} while(0)
#endif

// -----------------------------------------------------------------------------
// Convenience macros to simplify instruction emulation sequences
// -----------------------------------------------------------------------------


// namespace bemu {


// -----------------------------------------------------------------------------
// Log operands

#define PRVNAME ("USHM"[cpu[current_thread].prv])

#define RMDYN   (RM==7)
#define RMNAME  (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0dyn"[RM * 4])
#define FRMNAME (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0rm7"[FRM * 4])

#define LOG_FRM(str, cond) do { \
    if (cond) \
        LOG(DEBUG, "\tfrm " str " 0x%x (%s)", FRM, FRMNAME); \
} while (0)

#define LOG_REG(str, n) \
    LOG_IF(DEBUG, n != 0, "\tx%d " str " 0x%" PRIx64, n, cpu[current_thread].xregs[n])

#define LOG_FREG(str, n) \
    LOG(DEBUG, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        cpu[current_thread].fregs[n].u32[0], cpu[current_thread].fregs[n].u32[1], \
        cpu[current_thread].fregs[n].u32[2], cpu[current_thread].fregs[n].u32[3], \
        cpu[current_thread].fregs[n].u32[4], cpu[current_thread].fregs[n].u32[5], \
        cpu[current_thread].fregs[n].u32[6], cpu[current_thread].fregs[n].u32[7])

#define LOG_MREG(str, n) \
    LOG(DEBUG, "\tm%d " str " 0x%02lx", n, cpu[current_thread].mregs[n].to_ulong())

#define LOG_SCP(str, row, col) \
    LOG(DEBUG, "\t%s[%d] " str " {" \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        (col)+0, SCP[row].u32[(col)+0], (col)+1, SCP[row].u32[(col)+1], \
        (col)+2, SCP[row].u32[(col)+2], (col)+3, SCP[row].u32[(col)+3], \
        (col)+4, SCP[row].u32[(col)+4], (col)+5, SCP[row].u32[(col)+5], \
        (col)+6, SCP[row].u32[(col)+6], (col)+7, SCP[row].u32[(col)+7])

#define LOG_SCP_32x16(str, row) \
    LOG(DEBUG, "\t%s[%d] " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
        " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
        " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        SCP[row].u32[0], SCP[row].u32[1], SCP[row].u32[2], SCP[row].u32[3], \
        SCP[row].u32[4], SCP[row].u32[5], SCP[row].u32[6], SCP[row].u32[7], \
        SCP[row].u32[8], SCP[row].u32[9], SCP[row].u32[10], SCP[row].u32[11], \
        SCP[row].u32[12], SCP[row].u32[13], SCP[row].u32[14], SCP[row].u32[15])

#define LOG_SCP_32x1(str, row, col) \
    LOG(DEBUG, "\t%s[%d] " str " { %d:0x%08" PRIx32 " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        col, SCP[row].u32[col])

#define LOG_FREG_OTHER(other_thread, str, n) \
    LOG(DEBUG, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        cpu[other_thread].fregs[n].u32[0], cpu[other_thread].fregs[n].u32[1], \
        cpu[other_thread].fregs[n].u32[2], cpu[other_thread].fregs[n].u32[3], \
        cpu[other_thread].fregs[n].u32[4], cpu[other_thread].fregs[n].u32[5], \
        cpu[other_thread].fregs[n].u32[6], cpu[other_thread].fregs[n].u32[7])

#define LOG_FFLAGS(str, n) \
    LOG(DEBUG, "\tfflags " str " 0x%" PRIx32, uint32_t(n))

#define LOG_PC(str) \
    LOG(DEBUG, "\tpc " str " 0x%" PRIx64, PC)

#define LOG_GSC_PROGRESS(str) \
    LOG(DEBUG, "\tgsc_progress " str " %u", unsigned(cpu[current_thread].gsc_progress))

#define LOG_MEMWRITE(size, addr, value) \
   LOG(DEBUG, "\tMEM%zu[0x%" PRIx64 "] = 0x%llx", size_t(size) , addr, static_cast<unsigned long long>(value))

#define LOG_MEMWRITE128(addr, ptr) \
   LOG(DEBUG, "\tMEM128[0x%" PRIx64 "] = {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3])

#define LOG_MEMWRITE512(addr, ptr) \
   LOG(DEBUG, "\tMEM512[0x%" PRIx64 "] = {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
       " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], \
       ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15])

#define LOG_MEMREAD(size, addr, value) \
   LOG(DEBUG, "\tMEM%zu[0x%" PRIx64 "] : 0x%llx", size_t(size), addr, static_cast<unsigned long long>(value))

#define LOG_MEMREAD128(addr, ptr) \
   LOG(DEBUG, "\tMEM128[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " }", addr, ptr[0], ptr[1], ptr[2], ptr[3])

#define LOG_MEMREAD256(addr, ptr) \
   LOG(DEBUG, "\tMEM256[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " }", addr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7])

#define LOG_MEMREAD512(addr, ptr) \
   LOG(DEBUG, "\tMEM512[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
       " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], \
       ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15])

#define LOG_PRV(str, value) \
    LOG(DEBUG, "\tprv " str " %c", "USHM"[int(value) % 4])

#define LOG_MSTATUS(str, value) \
    LOG(DEBUG, "\tmstatus " str " 0x%" PRIx64, value)

#define LOG_CSR(str, index, value) \
    LOG(DEBUG, "\t%s " str " 0x%" PRIx64, csr_name(index), value)

#define LOG_TENSOR_MASK(str) \
    LOG(DEBUG, "\ttensor_mask " str " 0x%" PRIx16, cpu[current_thread].tensor_mask)


// -----------------------------------------------------------------------------
// Access instruction fields

#define PC      current_pc
#define NPC     sextVA(current_pc + inst.size())

#define BIMM    inst.b_imm()
#define F32IMM  inst.f32imm()
#define I32IMM  inst.i32imm()
#define IIMM    inst.i_imm()
#define SIMM    inst.s_imm()
#define JIMM    inst.j_imm()
#define SHAMT5  inst.shamt5()
#define SHAMT6  inst.shamt6()
#define UIMM    inst.u_imm()
#define UIMM3   inst.uimm3()
#define UIMM8   inst.uimm8()
#define UMSK4   inst.umsk4()
#define VIMM    inst.v_imm()

#define RM      inst.rm()

#define C_BIMM           inst.rvc_b_imm()
#define C_JIMM           inst.rvc_j_imm()
#define C_IMM6           inst.rvc_imm6()
#define C_IMMLDSP        inst.rvc_imm_ldsp()
#define C_IMMLWSP        inst.rvc_imm_lwsp()
#define C_IMMSDSP        inst.rvc_imm_sdsp()
#define C_IMMSWSP        inst.rvc_imm_swsp()
#define C_IMMLSW         inst.rvc_imm_lsw()
#define C_IMMLSD         inst.rvc_imm_lsd()
#define C_SHAMT          inst.rvc_shamt()
#define C_NZIMMLUI       inst.rvc_nzimm_lui()
#define C_NZIMMADDI16SP  inst.rvc_nzimm_addi16sp()
#define C_NZUIMMADDI4SPN inst.rvc_nzuimm_addi4spn()


// -----------------------------------------------------------------------------
// Access program state

#define X2      cpu[current_thread].xregs[2]
#define X31     cpu[current_thread].xregs[31]
#define RD      cpu[current_thread].xregs[inst.rd()]
#define RS1     cpu[current_thread].xregs[inst.rs1()]
#define RS2     cpu[current_thread].xregs[inst.rs2()]
#define C_RS1   cpu[current_thread].xregs[inst.rvc_rs1()]
#define C_RS1P  cpu[current_thread].xregs[inst.rvc_rs1p()]
#define C_RS2   cpu[current_thread].xregs[inst.rvc_rs2()]
#define C_RS2P  cpu[current_thread].xregs[inst.rvc_rs2p()]

#define M0      cpu[current_thread].mregs[0]
#define MD      cpu[current_thread].mregs[inst.md()]
#define MS1     cpu[current_thread].mregs[inst.ms1()]
#define MS2     cpu[current_thread].mregs[inst.ms2()]

#define FD      cpu[current_thread].fregs[inst.fd()]
#define FS1     cpu[current_thread].fregs[inst.fs1()]
#define FS2     cpu[current_thread].fregs[inst.fs2()]
#define FS3     cpu[current_thread].fregs[inst.fs3()]

#define FRM     ((cpu[current_thread].fcsr >> 5) & 7)

#define PRV     cpu[current_thread].prv


#define set_rounding_mode(expr) do { \
    extern uint32_t current_inst; \
    uint_fast8_t round = (expr); \
    if (round == 7) round = FRM; \
    if (round > 4) throw trap_illegal_instruction(current_inst); \
    softfloat_roundingMode = round; \
} while (0)


#define require_fp_active() do { \
    extern uint32_t current_inst; \
    if ((cpu[current_thread].mstatus & 0x6000ULL) == 0) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


#define require_feature_gfx() do { \
    extern uint32_t current_inst; \
    if (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x1) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


#define require_feature_ml() do { \
    extern uint32_t current_inst; \
    if (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x2) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


#define require_feature_ml_on_thread0() do { \
    extern uint32_t current_inst; \
    if ((current_thread % EMU_THREADS_PER_MINION) || \
        (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x2)) \
        throw trap_illegal_instruction(current_inst); \
} while (0)

#define require_feature_u_cacheops() do { \
    extern uint32_t current_inst; \
    if (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x4) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


#define require_feature_u_scratchpad() do { \
    extern uint32_t current_inst; \
    if (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x8) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


#define require_lock_unlock_enabled() do { \
    extern uint32_t current_inst; \
    if (bemu::shire_other_esrs[current_thread / EMU_THREADS_PER_SHIRE].minion_feature & 0x24) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


// -----------------------------------------------------------------------------
// Write destination registers

inline mreg_t mkmask(unsigned len) {
    return mreg_t((1ull << len) - 1ull);
}


#define WRITE_PC(expr) do { \
    uint64_t pc = sextVA(expr); \
    PROFILING_WRITE_PC(current_thread, pc); \
    log_pc_update(pc); \
} while (0)


#define WRITE_REG(n, expr, late) do { \
    if (n != 0) { \
        cpu[current_thread].xregs[n] = (expr); \
        LOG_REG("=", n); \
    } \
    if (late) \
        log_xreg_late_write(n, cpu[current_thread].xregs[n]); \
    else \
        log_xreg_write(n, cpu[current_thread].xregs[n]); \
} while (0)


#define WRITE_X0(expr)      WRITE_REG(0, expr, false)
#define WRITE_X1(expr)      WRITE_REG(1, expr, false)
#define WRITE_X2(expr)      WRITE_REG(2, expr, false)
#define WRITE_RD(expr)      WRITE_REG(inst.rd(), expr, false)
#define WRITE_C_RS1(expr)   WRITE_REG(inst.rvc_rs1(), expr, false)
#define WRITE_C_RS1P(expr)  WRITE_REG(inst.rvc_rs1p(), expr, false)
#define WRITE_C_RS2P(expr)  WRITE_REG(inst.rvc_rs2p(), expr, false)

#define LATE_WRITE_RD(expr) WRITE_REG(inst.rd(), expr, true)

#define LOAD_WRITE_RD(expr)     WRITE_REG(inst.rd(), expr, true)
#define LOAD_WRITE_C_RS1(expr)  WRITE_REG(inst.rvc_rs1(), expr, true)
#define LOAD_WRITE_C_RS2P(expr) WRITE_REG(inst.rvc_rs2p(), expr, true)

#define WRITE_MREG(n, expr) do { \
    cpu[current_thread].mregs[n] = (expr); \
    LOG_MREG("=", n); \
    dirty_fp_state(); \
    log_mreg_write(n, cpu[current_thread].mregs[n]); \
} while (0)


#define WRITE_MD(expr)  WRITE_MREG(inst.md(), expr)


#define WRITE_FD_REG(expr, load) do { \
    FD.u32[0] = fpu::UI32(expr); \
    for (size_t e = 1; e < MLEN; ++e) { \
        FD.u32[e] = 0; \
    } \
    LOG_FREG("=", inst.fd()); \
    dirty_fp_state(); \
    if (load) \
        log_freg_load(inst.fd(), mreg_t(-1), FD); \
    else \
        log_freg_write(inst.fd(), mreg_t(-1), FD); \
} while (0)

#define WRITE_FD(expr)  WRITE_FD_REG(expr, false)
#define LOAD_FD(expr)   WRITE_FD_REG(expr, true)

#define WRITE_VD_REG(expr, load) do { \
    LOG_MREG(":", 0); \
    if (M0.any()) { \
        for (size_t e = 0; e < MLEN; ++e) { \
            if (M0[e]) { \
                FD.u32[e] = fpu::UI32(expr); \
            } \
        } \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    if (load) \
        log_freg_load(inst.fd(), M0, FD); \
    else \
        log_freg_write(inst.fd(), M0, FD); \
} while (0)

#define WRITE_VD(expr)  WRITE_VD_REG(expr, false)
#define LOAD_VD(expr)   WRITE_VD_REG(expr, true)

#define SCATTER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    for (size_t e = 0; e < cpu[current_thread].gsc_progress; ++e) \
        log_mem_write(false, -1, 0, 0, 0); \
    for (size_t e = cpu[current_thread].gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                expr; \
            } \
            catch (const trap_t&) { \
                cpu[current_thread].gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                throw; \
            } \
        } else { \
            log_mem_write(false, -1, 0, 0, 0); \
        } \
    } \
    cpu[current_thread].gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
} while (0)


#define GATHER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (size_t e = 0; e < cpu[current_thread].gsc_progress; ++e) \
        log_mem_read(false, -1, 0, 0); \
    for (size_t e = cpu[current_thread].gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu[current_thread].gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                if (dirty) { \
                    LOG_FREG("=", inst.fd()); \
                    dirty_fp_state(); \
                    log_freg_load(inst.fd(), mkmask(e) & M0, FD); \
                } \
                throw; \
            } \
        } else { \
            log_mem_read(false, -1, 0, 0); \
        } \
    } \
    cpu[current_thread].gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
    if (dirty) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_load(inst.fd(), M0, FD); \
} while (0)


#define SCATTER32(expr) do { \
    for (size_t e = 0; e < MLEN; ++e) { \
        if (M0[e]) { \
            expr; \
        } else { \
            log_mem_write(false, -1, 0, 0, 0); \
        } \
    } \
} while (0)


#define GATHER32(expr) do { \
    for (size_t e = 0; e < MLEN; ++e) { \
        if (M0[e]) { \
            FD.u32[e] = fpu::UI32(expr); \
        } else { \
            log_mem_read(false, -1, 0, 0); \
        } \
    } \
    if (M0.any()) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_load(inst.fd(), M0, FD); \
} while (0)


#ifdef ZSIM
// Do not write the fregs, the value will come from the monitor
#define GSCAMO(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (size_t e = 0; e < cpu[current_thread].gsc_progress; ++e) \
        log_mem_read_write(false, -1, 0, 0, 0); \
    freg_t tmp(FD); \
    for (size_t e = cpu[current_thread].gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu[current_thread].gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                if (dirty) { \
                    LOG_FREG("=", inst.fd()); \
                    dirty_fp_state(); \
                    log_freg_load(inst.fd(), mkmask(e) & M0, FD); \
                } \
                std::swap(tmp, FD); \
                throw; \
            } \
        } else { \
            log_mem_read_write(false, -1, 0, 0, 0); \
        } \
    } \
    cpu[current_thread].gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
    if (dirty) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_load(inst.fd(), M0, FD); \
    std::swap(tmp, FD); \
} while (0)
#else // ZSIM
#define GSCAMO(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (size_t e = 0; e < cpu[current_thread].gsc_progress; ++e) \
        log_mem_read_write(false, -1, 0, 0, 0); \
    for (size_t e = cpu[current_thread].gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu[current_thread].gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                if (dirty) { \
                    LOG_FREG("=", inst.fd()); \
                    dirty_fp_state(); \
                    log_freg_load(inst.fd(), mkmask(e) & M0, FD); \
                } \
                throw; \
            } \
        } else { \
            log_mem_read_write(false, -1, 0, 0, 0); \
        } \
    } \
    cpu[current_thread].gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
    if (dirty) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_load(inst.fd(), M0, FD); \
} while (0)
#endif // ZSIM

#define WRITE_VD_NOMASK(expr) do { \
    for (size_t e = 0; e < MLEN; ++e) { \
        FD.u32[e] = fpu::UI32(expr); \
    } \
    LOG_FREG("=", inst.fd()); \
    dirty_fp_state(); \
    log_freg_write(inst.fd(), mreg_t(-1), FD); \
} while (0)


#define LOAD_VD_NODATA(msk) do { \
    if (msk.any()) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_load(inst.fd(), msk, FD); \
} while (0)


#define WRITE_VMD(expr) do { \
    LOG_MREG(":", 0); \
    if (M0.any()) { \
        for (size_t e = 0; e < MLEN; ++e) { \
            if (M0[e]) { \
                MD[e] = (expr); \
            } \
        } \
        LOG_MREG("=", inst.md()); \
        dirty_fp_state(); \
    } \
    log_mreg_write(inst.md(), MD); \
} while (0)


#define dirty_fp_state() do { \
    /*cpu[current_thread].mstatus |= 0x8000000000006000ULL;*/ \
} while (0)


#define set_fp_exceptions() do { \
    if (softfloat_exceptionFlags) { \
        dirty_fp_state(); \
        uint32_t newval = \
                (softfloat_exceptionFlags & 0x1F) | \
                (uint32_t(softfloat_exceptionFlags & 0x20) << 26); \
        cpu[current_thread].fcsr |= newval; \
        LOG_FFLAGS("|=", newval); \
        log_fflags_write(newval); \
        softfloat_exceptionFlags = 0; \
    } \
} while (0)


// -----------------------------------------------------------------------------
// Disassemble instruction, and input operands

#define DISASM_NOARG(name) do { \
    LOG(DEBUG, "I(%c): " name, PRVNAME); \
} while (0)

#define DISASM_RD_ALLMASK(name) do { \
    LOG(DEBUG, "I(%c): " name, PRVNAME); \
    LOG_MREG(":", 0); \
    LOG_MREG(":", 1); \
    LOG_MREG(":", 2); \
    LOG_MREG(":", 3); \
    LOG_MREG(":", 4); \
    LOG_MREG(":", 5); \
    LOG_MREG(":", 6); \
    LOG_MREG(":", 7); \
} while (0)

#define DISASM_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d", PRVNAME, inst.rs1(), inst.rs2()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_RS1_RS2_BIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,%" PRId64, PRVNAME, inst.rs1(), inst.rs2(), BIMM); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_RD_JIMM(name) \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rd(), JIMM)

#define DISASM_RD_RS1_IIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,%" PRId64, PRVNAME, inst.rd(), inst.rs1(), IIMM); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_CSR_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%s,x%d", PRVNAME, inst.rd(), csr_name(inst.csrimm()), inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_CSR_UIMM5(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%s,0x%" PRIx64, PRVNAME, inst.rd(), csr_name(inst.csrimm()), inst.uimm5()); \
} while (0)

#define DISASM_RD_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,x%d", PRVNAME, inst.rd(), inst.rs1(), inst.rs2()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_RD_RS1_SHAMT5(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,0x%x", PRVNAME, inst.rd(), inst.rs1(), SHAMT5); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_RS1_SHAMT6(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,0x%x", PRVNAME, inst.rd(), inst.rs1(), SHAMT6); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_UIMM(name) \
    LOG(DEBUG, "I(%c): " name " x%d,0x%x", PRVNAME, inst.rd(), unsigned((UIMM>>12) & 0xFFFFF))

#define DISASM_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d", PRVNAME, inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_MD_FS1(name) do { \
    LOG(DEBUG, "I(%c): " name " m%d,f%d", PRVNAME, inst.md(), inst.fs1()); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_MD_FS1_FS2(name) do { \
    LOG(DEBUG, "I(%c): " name " m%d,f%d,f%d", PRVNAME, inst.md(), inst.fs1(), inst.fs2()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_MD_MS1(name) do { \
    LOG(DEBUG, "I(%c): " name " m%d,m%d", PRVNAME, inst.md(), inst.ms1()); \
    LOG_MREG(":", inst.ms1()); \
} while (0)

#define DISASM_MD_MS1_MS2(name) do { \
    LOG(DEBUG, "I(%c): " name " m%d,m%d,m%d", PRVNAME, inst.md(), inst.ms1(), inst.ms2()); \
    LOG_MREG(":", inst.ms1()); \
    LOG_MREG(":", inst.ms2()); \
} while (0)

#define DISASM_MD_RS1_UIMM8(name) do { \
    LOG(DEBUG, "I(%c): " name " m%d,x%d,0x%x", PRVNAME, inst.md(), inst.rs1(), UIMM8); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_MS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,m%d", PRVNAME, inst.rd(), inst.ms1()); \
    LOG_MREG(":", inst.ms1()); \
} while (0)

#define DISASM_RD_MS1_MS2_UMSK4(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,m%d,m%d,0x%x", PRVNAME, inst.rd(), inst.ms1(), inst.ms2(), UMSK4); \
    LOG_MREG(":", inst.ms1()); \
    LOG_MREG(":", inst.ms2()); \
} while (0)

#define DISASM_FD_F32IMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,0x%x", PRVNAME, inst.fd(), F32IMM); \
} while (0)

#define DISASM_FD_FS1(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d", PRVNAME, inst.fd(), inst.fs1()); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_FRM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d", PRVNAME, inst.fd(), inst.fs1()); \
    LOG_FRM(":", true); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_FS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d", PRVNAME, inst.fd(), inst.fs1(), inst.fs2()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_FS2_FRM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d", PRVNAME, inst.fd(), inst.fs1(), inst.fs2()); \
    LOG_FRM(":", true); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FDS0_FS1_FS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d", PRVNAME, inst.fd(), inst.fs1(), inst.fs2()); \
    LOG_FREG(":", inst.fd()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d,f%d", PRVNAME, inst.fd(), inst.fs1(), inst.fs2(), inst.fs3()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
    LOG_FREG(":", inst.fs3()); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d,f%d,%s", PRVNAME, inst.fd(), inst.fs1(), inst.fs2(), inst.fs3(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
    LOG_FREG(":", inst.fs3()); \
} while (0)

#define DISASM_FD_FS1_FS2_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d,%s", PRVNAME, inst.fd(), inst.fs1(), inst.fs2(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,%s", PRVNAME, inst.fd(), inst.fs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_UIMM8(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, inst.fd(), inst.fs1(), UIMM8); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_VIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,%d", PRVNAME, inst.fd(), inst.fs1(), VIMM); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_UVIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, inst.fd(), inst.fs1(), VIMM); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_SHAMT5(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, inst.fd(), inst.fs1(), SHAMT5); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_I32IMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,%d", PRVNAME, inst.fd(), I32IMM); \
} while (0)

#define DISASM_FD_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,x%d", PRVNAME, inst.fd(), inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_FD_RS1_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,x%d,%s", PRVNAME, inst.fd(), inst.rs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_RD_FS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,f%d", PRVNAME, inst.rd(), inst.fs1()); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_RD_FS1_FS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,f%d,f%d", PRVNAME, inst.rd(), inst.fs1(), inst.fs2()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_RD_FS1_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,f%d,%s", PRVNAME, inst.rd(), inst.fs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_RD_FS1_UIMM3(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,f%d,0x%x", PRVNAME, inst.rd(), inst.fs1(), UIMM3); \
    LOG_FREG(":", inst.fs1()); \
} while (0)


#define DISASM_LOAD_RD_RS1_IIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rd(), IIMM, inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_STORE_RS2_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,(x%d)", PRVNAME, inst.rs2(), inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_STORE_RS2_RS1_SIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rs2(), SIMM, inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_AMO_RD_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d,(x%d)", PRVNAME, inst.rd(), inst.rs2(), inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
} while (0)

#define DISASM_LOAD_FD_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,(x%d)", PRVNAME, inst.fd(), inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_LOAD_FD_RS1_IIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,%" PRId64 "(x%d)", PRVNAME, inst.fd(), IIMM, inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_STORE_FD_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,(x%d)", PRVNAME, inst.fd(), inst.rs1()); \
    LOG_FREG(":", inst.fd()); \
    LOG_REG(":", inst.rs1()); \
} while (0)

#define DISASM_STORE_FS2_RS1_SIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,%" PRId64 "(x%d)", PRVNAME, inst.fs2(), SIMM, inst.rs1()); \
    LOG_REG(":", inst.rs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_GATHER_FD_FS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, inst.fd(), inst.fs1(), inst.rs2()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_REG(":", inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_GATHER_FD_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,x%d(x%d)", PRVNAME, inst.fd(), inst.rs1(), inst.rs2()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_SCATTER_FD_FS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, inst.fd(), inst.fs1(), inst.rs2()); \
    LOG_FREG(":", inst.fd()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_REG(":", inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_SCATTER_FD_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,x%d(x%d)", PRVNAME, inst.fd(), inst.rs1(), inst.rs2()); \
    LOG_FREG(":", inst.fd()); \
    LOG_REG(":", inst.rs1()); \
    LOG_REG(":", inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_AMO_FD_FS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, inst.fd(), inst.fs1(), inst.rs2()); \
    LOG_FREG(":", inst.fd()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_REG(":", inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)


#define C_DISASM_JIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " %" PRId64, PRVNAME, C_JIMM); \
} while (0)

#define C_DISASM_RS1(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d", PRVNAME, inst.rvc_rs1()); \
    LOG_REG(":", inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RS1P_BIMM(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rvc_rs1p(), C_BIMM); \
    LOG_REG(":", inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RS1_IMM6(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rvc_rs1(), C_IMM6); \
} while (0)

#define C_DISASM_RS1_NZIMMLUI(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rvc_rs1(), C_NZIMMLUI); \
    LOG_REG(":", inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RDS1_IMM6(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rvc_rs1(), C_IMM6); \
    LOG_REG(":", inst.rvc_rs1()); \
} while (0)

#define C_DISASM_NZIMMADDI16SP(name) do { \
    LOG(DEBUG, "I(%c): " name " x2,%" PRId64, PRVNAME, C_NZIMMADDI16SP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_RS2P_NZUIMMADDI4SPN(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x2,%" PRId64, PRVNAME, inst.rvc_rs2p(), C_NZUIMMADDI4SPN); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_RDS1_SHAMT(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,0x%x", PRVNAME, inst.rvc_rs1(), C_SHAMT); \
    LOG_REG(":", inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RDS1P_SHAMT(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,0x%x", PRVNAME, inst.rvc_rs1p(), C_SHAMT); \
    LOG_REG(":", inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RDS1P_IMM6(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64, PRVNAME, inst.rvc_rs1p(), C_IMM6); \
    LOG_REG(":", inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d", PRVNAME, inst.rvc_rs1(), inst.rvc_rs2()); \
    LOG_REG(":", inst.rvc_rs2()); \
} while (0)

#define C_DISASM_RDS1_RS2(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d", PRVNAME, inst.rvc_rs1(), inst.rvc_rs2()); \
    LOG_REG(":", inst.rvc_rs1()); \
    LOG_REG(":", inst.rvc_rs2()); \
} while (0)

#define C_DISASM_RDS1P_RS2P(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,x%d", PRVNAME, inst.rvc_rs1p(), inst.rvc_rs2p()); \
    LOG_REG(":", inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs2p()); \
} while (0)

#define C_DISASM_LOAD_LDSP(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, inst.rvc_rs1(), C_IMMLDSP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_LOAD_LWSP(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, inst.rvc_rs1(), C_IMMLWSP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_STORE_SDSP(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, inst.rvc_rs1(), C_IMMSDSP); \
    LOG_REG(":", 2); \
    LOG_REG(":", inst.rvc_rs2()); \
} while (0)

#define C_DISASM_STORE_SWSP(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, inst.rvc_rs1(), C_IMMSWSP); \
    LOG_REG(":", 2); \
    LOG_REG(":", inst.rvc_rs2()); \
} while (0)

#define C_DISASM_LOAD_RS2P_RS1P_IMMLSD(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rvc_rs2p(), C_IMMLSD, inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_LOAD_RS2P_RS1P_IMMLSW(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rvc_rs2p(), C_IMMLSW, inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_STORE_RS2P_RS1P_IMMLSD(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rvc_rs2p(), C_IMMLSD, inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs2p()); \
} while (0)

#define C_DISASM_STORE_RS2P_RS1P_IMMLSW(name) do { \
    LOG(DEBUG, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, inst.rvc_rs2p(), C_IMMLSW, inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs1p()); \
    LOG_REG(":", inst.rvc_rs2p()); \
} while (0)


//} namespace bemu

#endif // BEMU_DECODE_H
