/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_INSN_UTIL_H
#define BEMU_INSN_UTIL_H

#include "emu_gio.h"
#include "esrs.h"
#include "fpu/fpu.h"
#include "log.h"
#include "processor.h"

#ifdef BEMU_PROFILING
#include "sys_emu/profiling.h"
#else
inline void profiling_write_pc(int, uint64_t) {}
#endif

// -----------------------------------------------------------------------------
// Convenience macros to simplify instruction emulation sequences
// -----------------------------------------------------------------------------

namespace bemu {


// -----------------------------------------------------------------------------
// Log operands

#define PRVNAME ("USHM"[cpu.prv])

#define RMDYN   (RM==7)
#define RMNAME  (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0dyn"[RM * 4])
#define FRMNAME (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0rm7"[FRM * 4])

#define LOG_FRM(str, cond) do { \
    if (cond) \
        LOG_HART(DEBUG, cpu, "\tfrm " str " 0x%x (%s)", FRM, FRMNAME); \
} while (0)

#define LOG_REG(str, n) do { \
    if (n != 0) \
        LOG_HART(DEBUG, cpu, "\tx%d " str " 0x%" PRIx64, n, cpu.xregs[n]); \
} while (0)

#define LOG_FREG(str, n) \
    LOG_HART(DEBUG, cpu, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        cpu.fregs[n].u32[0], cpu.fregs[n].u32[1], \
        cpu.fregs[n].u32[2], cpu.fregs[n].u32[3], \
        cpu.fregs[n].u32[4], cpu.fregs[n].u32[5], \
        cpu.fregs[n].u32[6], cpu.fregs[n].u32[7])

#define LOG_CREG(str, n) \
    LOG_HART(DEBUG, cpu, "\tc%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        cpu.core->tenc[n].u32[0], cpu.core->tenc[n].u32[1], \
        cpu.core->tenc[n].u32[2], cpu.core->tenc[n].u32[3], \
        cpu.core->tenc[n].u32[4], cpu.core->tenc[n].u32[5], \
        cpu.core->tenc[n].u32[6], cpu.core->tenc[n].u32[7])

#define LOG_MREG(str, n) \
    LOG_HART(DEBUG, cpu, "\tm%d " str " 0x%02lx", n, cpu.mregs[n].to_ulong())

#define LOG_SCP(str, row, col) \
    LOG_HART(DEBUG, cpu, "\t%s[%d] " str " {" \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        (col)+0, cpu.core->scp[row].u32[(col)+0], (col)+1, cpu.core->scp[row].u32[(col)+1], \
        (col)+2, cpu.core->scp[row].u32[(col)+2], (col)+3, cpu.core->scp[row].u32[(col)+3], \
        (col)+4, cpu.core->scp[row].u32[(col)+4], (col)+5, cpu.core->scp[row].u32[(col)+5], \
        (col)+6, cpu.core->scp[row].u32[(col)+6], (col)+7, cpu.core->scp[row].u32[(col)+7])

#define LOG_SCP_32x16(str, row) \
    LOG_HART(DEBUG, cpu, "\t%s[%d] " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
        " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
        " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        cpu.core->scp[row].u32[0], cpu.core->scp[row].u32[1], cpu.core->scp[row].u32[2], cpu.core->scp[row].u32[3], \
        cpu.core->scp[row].u32[4], cpu.core->scp[row].u32[5], cpu.core->scp[row].u32[6], cpu.core->scp[row].u32[7], \
        cpu.core->scp[row].u32[8], cpu.core->scp[row].u32[9], cpu.core->scp[row].u32[10], cpu.core->scp[row].u32[11], \
        cpu.core->scp[row].u32[12], cpu.core->scp[row].u32[13], cpu.core->scp[row].u32[14], cpu.core->scp[row].u32[15])

#define LOG_SCP_32x1(str, row, col) \
    LOG_HART(DEBUG, cpu, "\t%s[%d] " str " { %d:0x%08" PRIx32 " }", \
        (row >= L1_SCP_ENTRIES) ? "TenB" : "SCP", \
        (row >= L1_SCP_ENTRIES) ? (row - L1_SCP_ENTRIES) : row, \
        col, cpu.core->scp[row].u32[col])

#define LOG_FREG_HART(cpu, str, n) \
    LOG_HART(DEBUG, cpu, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        cpu.fregs[n].u32[0], cpu.fregs[n].u32[1], \
        cpu.fregs[n].u32[2], cpu.fregs[n].u32[3], \
        cpu.fregs[n].u32[4], cpu.fregs[n].u32[5], \
        cpu.fregs[n].u32[6], cpu.fregs[n].u32[7])

#define LOG_FFLAGS(str, n) \
    LOG_HART(DEBUG, cpu, "\tfflags " str " 0x%" PRIx32, uint32_t(n))

#define LOG_PC(str) \
    LOG_HART(DEBUG, cpu, "\tpc " str " 0x%" PRIx64, PC)

#define LOG_GSC_PROGRESS(str) \
    LOG_HART(DEBUG, cpu, "\tgsc_progress " str " %u", unsigned(cpu.gsc_progress))

#define LOG_MEMWRITE(size, addr, value) \
   LOG_HART(DEBUG, cpu, "\tMEM%zu[0x%" PRIx64 "] = 0x%llx", std::size_t(size) , addr, static_cast<unsigned long long>(value))

#define LOG_MEMWRITE128(addr, ptr) \
   LOG_HART(DEBUG, cpu, "\tMEM128[0x%" PRIx64 "] = {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3])

#define LOG_MEMWRITE512(addr, ptr) \
   LOG_HART(DEBUG, cpu, "\tMEM512[0x%" PRIx64 "] = {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
       " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], \
       ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15])

#define LOG_MEMREAD(size, addr, value) \
   LOG_HART(DEBUG, cpu, "\tMEM%zu[0x%" PRIx64 "] : 0x%llx", std::size_t(size), addr, static_cast<unsigned long long>(value))

#define LOG_MEMREAD128(addr, ptr) \
   LOG_HART(DEBUG, cpu, "\tMEM128[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " }", addr, ptr[0], ptr[1], ptr[2], ptr[3])

#define LOG_MEMREAD256(addr, ptr) \
   LOG_HART(DEBUG, cpu, "\tMEM256[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " }", addr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7])

#define LOG_MEMREAD512(addr, ptr) \
   LOG_HART(DEBUG, cpu, "\tMEM512[0x%" PRIx64 "] : {" \
       " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
       " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
       " 8:0x%08" PRIx32 " 9:0x%08" PRIx32 " 10:0x%08" PRIx32 " 11:0x%08" PRIx32 \
       " 12:0x%08" PRIx32 " 13:0x%08" PRIx32 " 14:0x%08" PRIx32 " 15:0x%08" PRIx32 \
       " }", addr, \
       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], \
       ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15])

#define LOG_PRV(str, value) \
    LOG_HART(DEBUG, cpu, "\tprv " str " %c", "USHM"[int(value) % 4])

#define LOG_MSTATUS(str, value) \
    LOG_HART(DEBUG, cpu, "\tmstatus " str " 0x%" PRIx64, value)

#define LOG_CSR(str, index, value) \
    LOG_HART(DEBUG, cpu, "\t%s " str " 0x%" PRIx64, csr_name(index), value)

#define LOG_TENSOR_MASK(str) \
    LOG_HART(DEBUG, cpu, "\ttensor_mask " str " 0x%lx", cpu.tensor_mask.to_ulong())


// -----------------------------------------------------------------------------
// Access instruction fields

#define PC      cpu.pc
#define NPC     cpu.npc

#define BIMM    cpu.inst.b_imm()
#define F32IMM  cpu.inst.f32imm()
#define I32IMM  cpu.inst.i32imm()
#define IIMM    cpu.inst.i_imm()
#define SIMM    cpu.inst.s_imm()
#define JIMM    cpu.inst.j_imm()
#define SHAMT5  cpu.inst.shamt5()
#define SHAMT6  cpu.inst.shamt6()
#define UIMM    cpu.inst.u_imm()
#define UIMM3   cpu.inst.uimm3()
#define UIMM8   cpu.inst.uimm8()
#define UMSK4   cpu.inst.umsk4()
#define VIMM    cpu.inst.v_imm()

#define RM      cpu.inst.rm()

#define C_BIMM           cpu.inst.rvc_b_imm()
#define C_JIMM           cpu.inst.rvc_j_imm()
#define C_IMM6           cpu.inst.rvc_imm6()
#define C_IMMLDSP        cpu.inst.rvc_imm_ldsp()
#define C_IMMLWSP        cpu.inst.rvc_imm_lwsp()
#define C_IMMSDSP        cpu.inst.rvc_imm_sdsp()
#define C_IMMSWSP        cpu.inst.rvc_imm_swsp()
#define C_IMMLSW         cpu.inst.rvc_imm_lsw()
#define C_IMMLSD         cpu.inst.rvc_imm_lsd()
#define C_SHAMT          cpu.inst.rvc_shamt()
#define C_NZIMMLUI       cpu.inst.rvc_nzimm_lui()
#define C_NZIMMADDI16SP  cpu.inst.rvc_nzimm_addi16sp()
#define C_NZUIMMADDI4SPN cpu.inst.rvc_nzuimm_addi4spn()


// -----------------------------------------------------------------------------
// Access program state

#define X2      cpu.xregs[2]
#define X31     cpu.xregs[31]
#define RD      cpu.xregs[cpu.inst.rd()]
#define RS1     cpu.xregs[cpu.inst.rs1()]
#define RS2     cpu.xregs[cpu.inst.rs2()]
#define C_RS1   cpu.xregs[cpu.inst.rvc_rs1()]
#define C_RS1P  cpu.xregs[cpu.inst.rvc_rs1p()]
#define C_RS2   cpu.xregs[cpu.inst.rvc_rs2()]
#define C_RS2P  cpu.xregs[cpu.inst.rvc_rs2p()]

#define M0      cpu.mregs[0]
#define MD      cpu.mregs[cpu.inst.md()]
#define MS1     cpu.mregs[cpu.inst.ms1()]
#define MS2     cpu.mregs[cpu.inst.ms2()]

#define FD      cpu.fregs[cpu.inst.fd()]
#define FS1     cpu.fregs[cpu.inst.fs1()]
#define FS2     cpu.fregs[cpu.inst.fs2()]
#define FS3     cpu.fregs[cpu.inst.fs3()]

#define FRM     cpu.frm()

#define PRV     cpu.prv


inline void set_rounding_mode(Hart& cpu, uint_fast8_t value) {
    if (value == 7)
        value = cpu.frm();
    if (value > 4)
        throw trap_illegal_instruction(cpu.inst.bits);
    softfloat_roundingMode = value;
}


#define require_fp_active() do { \
    if ((cpu.mstatus & 0x6000ULL) == 0) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


#define require_feature_gfx() do { \
    if (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x1) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


#define require_feature_ml() do { \
    if (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x2) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


#define require_feature_ml_on_thread0() do { \
    if ((cpu.mhartid % EMU_THREADS_PER_MINION) || \
        (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x2)) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)

#define require_feature_u_cacheops() do { \
    if (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x4) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


#define require_feature_u_scratchpad() do { \
    if (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x8) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


#define require_lock_unlock_enabled() do { \
    if (cpu.chip->shire_other_esrs[shire_index(cpu)].minion_feature & 0x24) \
        throw trap_illegal_instruction(cpu.inst.bits); \
} while (0)


// -----------------------------------------------------------------------------
// Write destination registers

inline mreg_t mkmask(unsigned len) {
    return mreg_t((1ull << len) - 1ull);
}


#define WRITE_PC(expr) do { \
    NPC = sextVA(expr); \
    profiling_write_pc(hart_index(cpu), NPC); \
    notify_pc_update(cpu, NPC); \
} while (0)


#define WRITE_REG(n, expr, late) do { \
    if (n != 0) { \
        cpu.xregs[n] = (expr); \
        LOG_REG("=", n); \
    } \
    if (late) \
        notify_xreg_late_write(cpu, n, cpu.xregs[n]); \
    else \
        notify_xreg_write(cpu, n, cpu.xregs[n]); \
} while (0)


#define WRITE_X0(expr)      WRITE_REG(0, expr, false)
#define WRITE_X1(expr)      WRITE_REG(1, expr, false)
#define WRITE_X2(expr)      WRITE_REG(2, expr, false)
#define WRITE_RD(expr)      WRITE_REG(cpu.inst.rd(), expr, false)
#define WRITE_C_RS1(expr)   WRITE_REG(cpu.inst.rvc_rs1(), expr, false)
#define WRITE_C_RS1P(expr)  WRITE_REG(cpu.inst.rvc_rs1p(), expr, false)
#define WRITE_C_RS2P(expr)  WRITE_REG(cpu.inst.rvc_rs2p(), expr, false)

#define LATE_WRITE_RD(expr) WRITE_REG(cpu.inst.rd(), expr, true)

#define LOAD_WRITE_RD(expr)     WRITE_REG(cpu.inst.rd(), expr, true)
#define LOAD_WRITE_C_RS1(expr)  WRITE_REG(cpu.inst.rvc_rs1(), expr, true)
#define LOAD_WRITE_C_RS2P(expr) WRITE_REG(cpu.inst.rvc_rs2p(), expr, true)

#define WRITE_MREG(n, expr) do { \
    cpu.mregs[n] = (expr); \
    LOG_MREG("=", n); \
    dirty_fp_state(); \
    notify_mreg_write(cpu, n, cpu.mregs[n]); \
} while (0)


#define WRITE_MD(expr)  WRITE_MREG(cpu.inst.md(), expr)


#define WRITE_FD_REG(expr, load) do { \
    FD.u32[0] = fpu::UI32(expr); \
    for (std::size_t e = 1; e < MLEN; ++e) { \
        FD.u32[e] = 0; \
    } \
    LOG_FREG("=", cpu.inst.fd()); \
    dirty_fp_state(); \
    if (load) \
        notify_freg_load(cpu, cpu.inst.fd(), mreg_t(-1), FD); \
    else \
        notify_freg_write(cpu, cpu.inst.fd(), mreg_t(-1), FD); \
} while (0)

#define WRITE_FD(expr)  WRITE_FD_REG(expr, false)
#define LOAD_FD(expr)   WRITE_FD_REG(expr, true)

#define WRITE_VD_REG(expr, load) do { \
    LOG_MREG(":", 0); \
    if (M0.any()) { \
        for (std::size_t e = 0; e < MLEN; ++e) { \
            if (M0[e]) { \
                FD.u32[e] = fpu::UI32(expr); \
            } \
        } \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    if (load) \
        notify_freg_load(cpu, cpu.inst.fd(), M0, FD); \
    else \
        notify_freg_write(cpu, cpu.inst.fd(), M0, FD); \
} while (0)

#define WRITE_VD(expr)  WRITE_VD_REG(expr, false)
#define LOAD_VD(expr)   WRITE_VD_REG(expr, true)

#define SCATTER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    for (std::size_t e = 0; e < cpu.gsc_progress; ++e) \
        notify_mem_write(cpu, false, -1, 0, 0, 0); \
    for (std::size_t e = cpu.gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                expr; \
            } \
            catch (const trap_t&) { \
                cpu.gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                notify_gsc_progress(cpu, e); \
                throw; \
            } \
        } else { \
            notify_mem_write(cpu, false, -1, 0, 0, 0); \
        } \
    } \
    cpu.gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    notify_gsc_progress(cpu, 0); \
} while (0)


#define GATHER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (std::size_t e = 0; e < cpu.gsc_progress; ++e) \
        notify_mem_read(cpu, false, -1, 0, 0); \
    for (std::size_t e = cpu.gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu.gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                notify_gsc_progress(cpu, e); \
                if (dirty) { \
                    LOG_FREG("=", cpu.inst.fd()); \
                    dirty_fp_state(); \
                    notify_freg_load(cpu, cpu.inst.fd(), mkmask(e) & M0, FD); \
                } \
                throw; \
            } \
        } else { \
            notify_mem_read(cpu, false, -1, 0, 0); \
        } \
    } \
    cpu.gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    notify_gsc_progress(cpu, 0); \
    if (dirty) { \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    notify_freg_load(cpu, cpu.inst.fd(), M0, FD); \
} while (0)


#define SCATTER32(expr) do { \
    for (std::size_t e = 0; e < MLEN; ++e) { \
        if (M0[e]) { \
            expr; \
        } else { \
            notify_mem_write(cpu, false, -1, 0, 0, 0); \
        } \
    } \
} while (0)


#define GATHER32(expr) do { \
    for (std::size_t e = 0; e < MLEN; ++e) { \
        if (M0[e]) { \
            FD.u32[e] = fpu::UI32(expr); \
        } else { \
            notify_mem_read(cpu, false, -1, 0, 0); \
        } \
    } \
    if (M0.any()) { \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    notify_freg_load(cpu, cpu.inst.fd(), M0, FD); \
} while (0)


#ifdef ZSIM
// Do not write the fregs, the value will come from the monitor
#define GSCAMO(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (std::size_t e = 0; e < cpu.gsc_progress; ++e) \
        notify_mem_read_write(cpu, false, -1, 0, 0, 0); \
    freg_t tmp(FD); \
    for (std::size_t e = cpu.gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu.gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                notify_gsc_progress(cpu, e); \
                if (dirty) { \
                    LOG_FREG("=", cpu.inst.fd()); \
                    dirty_fp_state(); \
                    notify_freg_load(cpu, cpu.inst.fd(), mkmask(e) & M0, FD); \
                } \
                std::swap(tmp, FD); \
                throw; \
            } \
        } else { \
            notify_mem_read_write(cpu, false, -1, 0, 0, 0); \
        } \
    } \
    cpu.gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    notify_gsc_progress(cpu, 0); \
    if (dirty) { \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    notify_freg_load(cpu, cpu.inst.fd(), M0, FD); \
    std::swap(tmp, FD); \
} while (0)
#else // ZSIM
#define GSCAMO(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (std::size_t e = 0; e < cpu.gsc_progress; ++e) \
        notify_mem_read_write(cpu, false, -1, 0, 0, 0); \
    for (std::size_t e = cpu.gsc_progress; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                cpu.gsc_progress = e; \
                LOG_GSC_PROGRESS("="); \
                notify_gsc_progress(cpu, e); \
                if (dirty) { \
                    LOG_FREG("=", cpu.inst.fd()); \
                    dirty_fp_state(); \
                    notify_freg_load(cpu, cpu.inst.fd(), mkmask(e) & M0, FD); \
                } \
                throw; \
            } \
        } else { \
            notify_mem_read_write(cpu, false, -1, 0, 0, 0); \
        } \
    } \
    cpu.gsc_progress = 0; \
    LOG_GSC_PROGRESS("="); \
    notify_gsc_progress(cpu, 0); \
    if (dirty) { \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    notify_freg_load(cpu, cpu.inst.fd(), M0, FD); \
} while (0)
#endif // ZSIM

#define WRITE_VD_NOMASK(expr) do { \
    for (std::size_t e = 0; e < MLEN; ++e) { \
        FD.u32[e] = fpu::UI32(expr); \
    } \
    LOG_FREG("=", cpu.inst.fd()); \
    dirty_fp_state(); \
    notify_freg_write(cpu, cpu.inst.fd(), mreg_t(-1), FD); \
} while (0)


#define LOAD_VD_NODATA(msk) do { \
    if (msk.any()) { \
        LOG_FREG("=", cpu.inst.fd()); \
        dirty_fp_state(); \
    } \
    notify_freg_load(cpu, cpu.inst.fd(), msk, FD); \
} while (0)


#define WRITE_VMD(expr) do { \
    LOG_MREG(":", 0); \
    if (M0.any()) { \
        for (std::size_t e = 0; e < MLEN; ++e) { \
            if (M0[e]) { \
                MD[e] = (expr); \
            } \
        } \
        LOG_MREG("=", cpu.inst.md()); \
        dirty_fp_state(); \
    } \
    notify_mreg_write(cpu, cpu.inst.md(), MD); \
} while (0)


#define dirty_fp_state() do { \
    /*cpu.mstatus |= 0x8000000000006000ULL;*/ \
} while (0)


inline void set_fp_exceptions(Hart& cpu)
{
    if (softfloat_exceptionFlags) {
        dirty_fp_state();
        uint32_t newval =
                (softfloat_exceptionFlags & 0x1F) |
                (uint32_t(softfloat_exceptionFlags & 0x20) << 26);
        cpu.fcsr |= newval;
        LOG_FFLAGS("|=", newval);
        notify_fflags_write(cpu, newval);
        softfloat_exceptionFlags = 0;
    }
}


// -----------------------------------------------------------------------------
// Disassemble instruction, and input operands

#define DISASM_NOARG(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name, PRVNAME); \
} while (0)

#define DISASM_RD_ALLMASK(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name, PRVNAME); \
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
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d", PRVNAME, cpu.inst.rs1(), cpu.inst.rs2()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_RS1_RS2_BIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,%" PRId64, PRVNAME, cpu.inst.rs1(), cpu.inst.rs2(), BIMM); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_RD_JIMM(name) \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rd(), JIMM)

#define DISASM_RD_RS1_IIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,%" PRId64, PRVNAME, cpu.inst.rd(), cpu.inst.rs1(), IIMM); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_CSR_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%s,x%d", PRVNAME, cpu.inst.rd(), csr_name(cpu.inst.csrimm()), cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_CSR_UIMM5(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%s,0x%" PRIx64, PRVNAME, cpu.inst.rd(), csr_name(cpu.inst.csrimm()), cpu.inst.uimm5()); \
} while (0)

#define DISASM_RD_RS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,x%d", PRVNAME, cpu.inst.rd(), cpu.inst.rs1(), cpu.inst.rs2()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_RD_RS1_SHAMT5(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,0x%x", PRVNAME, cpu.inst.rd(), cpu.inst.rs1(), SHAMT5); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_RS1_SHAMT6(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,0x%x", PRVNAME, cpu.inst.rd(), cpu.inst.rs1(), SHAMT6); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_UIMM(name) \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,0x%x", PRVNAME, cpu.inst.rd(), unsigned((UIMM>>12) & 0xFFFFF))

#define DISASM_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d", PRVNAME, cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_MD_FS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " m%d,f%d", PRVNAME, cpu.inst.md(), cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_MD_FS1_FS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " m%d,f%d,f%d", PRVNAME, cpu.inst.md(), cpu.inst.fs1(), cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_MD_MS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " m%d,m%d", PRVNAME, cpu.inst.md(), cpu.inst.ms1()); \
    LOG_MREG(":", cpu.inst.ms1()); \
} while (0)

#define DISASM_MD_MS1_MS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " m%d,m%d,m%d", PRVNAME, cpu.inst.md(), cpu.inst.ms1(), cpu.inst.ms2()); \
    LOG_MREG(":", cpu.inst.ms1()); \
    LOG_MREG(":", cpu.inst.ms2()); \
} while (0)

#define DISASM_MD_RS1_UIMM8(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " m%d,x%d,0x%x", PRVNAME, cpu.inst.md(), cpu.inst.rs1(), UIMM8); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_MS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,m%d", PRVNAME, cpu.inst.rd(), cpu.inst.ms1()); \
    LOG_MREG(":", cpu.inst.ms1()); \
} while (0)

#define DISASM_RD_MS1_MS2_UMSK4(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,m%d,m%d,0x%x", PRVNAME, cpu.inst.rd(), cpu.inst.ms1(), cpu.inst.ms2(), UMSK4); \
    LOG_MREG(":", cpu.inst.ms1()); \
    LOG_MREG(":", cpu.inst.ms2()); \
} while (0)

#define DISASM_FD_F32IMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,0x%x", PRVNAME, cpu.inst.fd(), F32IMM); \
} while (0)

#define DISASM_FD_FS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_FRM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1()); \
    LOG_FRM(":", true); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_FS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_FS2_FRM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2()); \
    LOG_FRM(":", true); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_FDS0_FS1_FS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fd()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d,f%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2(), cpu.inst.fs3()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fs3()); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3_RM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d,f%d,%s", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2(), cpu.inst.fs3(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fs3()); \
} while (0)

#define DISASM_FD_FS1_FS2_RM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,f%d,%s", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.fs2(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_RM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,%s", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_UIMM8(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), UIMM8); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_VIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,%d", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), VIMM); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_UVIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), VIMM); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_SHAMT5(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d,0x%x", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), SHAMT5); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_FD_I32IMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,%d", PRVNAME, cpu.inst.fd(), I32IMM); \
} while (0)

#define DISASM_FD_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,x%d", PRVNAME, cpu.inst.fd(), cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_FD_RS1_RM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,x%d,%s", PRVNAME, cpu.inst.fd(), cpu.inst.rs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_RD_FS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,f%d", PRVNAME, cpu.inst.rd(), cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_RD_FS1_FS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,f%d,f%d", PRVNAME, cpu.inst.rd(), cpu.inst.fs1(), cpu.inst.fs2()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_RD_FS1_RM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,f%d,%s", PRVNAME, cpu.inst.rd(), cpu.inst.fs1(), RMNAME); \
    LOG_FRM(":", RMDYN); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)

#define DISASM_RD_FS1_UIMM3(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,f%d,0x%x", PRVNAME, cpu.inst.rd(), cpu.inst.fs1(), UIMM3); \
    LOG_FREG(":", cpu.inst.fs1()); \
} while (0)


#define DISASM_LOAD_RD_RS1_IIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rd(), IIMM, cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_STORE_RS2_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,(x%d)", PRVNAME, cpu.inst.rs2(), cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_STORE_RS2_RS1_SIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rs2(), SIMM, cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_AMO_RD_RS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d,(x%d)", PRVNAME, cpu.inst.rd(), cpu.inst.rs2(), cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
} while (0)

#define DISASM_LOAD_FD_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_LOAD_FD_RS1_IIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.fd(), IIMM, cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_STORE_FD_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.rs1()); \
    LOG_FREG(":", cpu.inst.fd()); \
    LOG_REG(":", cpu.inst.rs1()); \
} while (0)

#define DISASM_STORE_FS2_RS1_SIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.fs2(), SIMM, cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_FREG(":", cpu.inst.fs2()); \
} while (0)

#define DISASM_GATHER_FD_FS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.rs2()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_GATHER_FD_RS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,x%d(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.rs1(), cpu.inst.rs2()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_SCATTER_FD_FS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.rs2()); \
    LOG_FREG(":", cpu.inst.fd()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_SCATTER_FD_RS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,x%d(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.rs1(), cpu.inst.rs2()); \
    LOG_FREG(":", cpu.inst.fd()); \
    LOG_REG(":", cpu.inst.rs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)

#define DISASM_AMO_FD_FS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " f%d,f%d(x%d)", PRVNAME, cpu.inst.fd(), cpu.inst.fs1(), cpu.inst.rs2()); \
    LOG_FREG(":", cpu.inst.fd()); \
    LOG_FREG(":", cpu.inst.fs1()); \
    LOG_REG(":", cpu.inst.rs2()); \
    LOG_MREG(":", 0); \
} while (0)


#define C_DISASM_JIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " %" PRId64, PRVNAME, C_JIMM); \
} while (0)

#define C_DISASM_RS1(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d", PRVNAME, cpu.inst.rvc_rs1()); \
    LOG_REG(":", cpu.inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RS1P_BIMM(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rvc_rs1p(), C_BIMM); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RS1_IMM6(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rvc_rs1(), C_IMM6); \
} while (0)

#define C_DISASM_RS1_NZIMMLUI(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rvc_rs1(), C_NZIMMLUI); \
    LOG_REG(":", cpu.inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RDS1_IMM6(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rvc_rs1(), C_IMM6); \
    LOG_REG(":", cpu.inst.rvc_rs1()); \
} while (0)

#define C_DISASM_NZIMMADDI16SP(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x2,%" PRId64, PRVNAME, C_NZIMMADDI16SP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_RS2P_NZUIMMADDI4SPN(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x2,%" PRId64, PRVNAME, cpu.inst.rvc_rs2p(), C_NZUIMMADDI4SPN); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_RDS1_SHAMT(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,0x%x", PRVNAME, cpu.inst.rvc_rs1(), C_SHAMT); \
    LOG_REG(":", cpu.inst.rvc_rs1()); \
} while (0)

#define C_DISASM_RDS1P_SHAMT(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,0x%x", PRVNAME, cpu.inst.rvc_rs1p(), C_SHAMT); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RDS1P_IMM6(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64, PRVNAME, cpu.inst.rvc_rs1p(), C_IMM6); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_RS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d", PRVNAME, cpu.inst.rvc_rs1(), cpu.inst.rvc_rs2()); \
    LOG_REG(":", cpu.inst.rvc_rs2()); \
} while (0)

#define C_DISASM_RDS1_RS2(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d", PRVNAME, cpu.inst.rvc_rs1(), cpu.inst.rvc_rs2()); \
    LOG_REG(":", cpu.inst.rvc_rs1()); \
    LOG_REG(":", cpu.inst.rvc_rs2()); \
} while (0)

#define C_DISASM_RDS1P_RS2P(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,x%d", PRVNAME, cpu.inst.rvc_rs1p(), cpu.inst.rvc_rs2p()); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs2p()); \
} while (0)

#define C_DISASM_LOAD_LDSP(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, cpu.inst.rvc_rs1(), C_IMMLDSP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_LOAD_LWSP(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, cpu.inst.rvc_rs1(), C_IMMLWSP); \
    LOG_REG(":", 2); \
} while (0)

#define C_DISASM_STORE_SDSP(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, cpu.inst.rvc_rs1(), C_IMMSDSP); \
    LOG_REG(":", 2); \
    LOG_REG(":", cpu.inst.rvc_rs2()); \
} while (0)

#define C_DISASM_STORE_SWSP(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x2)", PRVNAME, cpu.inst.rvc_rs1(), C_IMMSWSP); \
    LOG_REG(":", 2); \
    LOG_REG(":", cpu.inst.rvc_rs2()); \
} while (0)

#define C_DISASM_LOAD_RS2P_RS1P_IMMLSD(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rvc_rs2p(), C_IMMLSD, cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_LOAD_RS2P_RS1P_IMMLSW(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rvc_rs2p(), C_IMMLSW, cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
} while (0)

#define C_DISASM_STORE_RS2P_RS1P_IMMLSD(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rvc_rs2p(), C_IMMLSD, cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs2p()); \
} while (0)

#define C_DISASM_STORE_RS2P_RS1P_IMMLSW(name) do { \
    LOG_HART(DEBUG, cpu, "I(%c): " name " x%d,%" PRId64 "(x%d)", PRVNAME, cpu.inst.rvc_rs2p(), C_IMMLSW, cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs1p()); \
    LOG_REG(":", cpu.inst.rvc_rs2p()); \
} while (0)


} // namespace bemu

#endif // BEMU_INSN_UTIL_H
