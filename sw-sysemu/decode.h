/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_DECODE_H
#define BEMU_DECODE_H

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

#define LOG_REG(str, n) \
    LOG_IF(DEBUG, n != 0, "\tx%d " str " 0x%" PRIx64, n, xregs[current_thread][n])

#define LOG_FREG(str, n) \
    LOG(DEBUG, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        fregs[current_thread][n].u32[0], fregs[current_thread][n].u32[1], \
        fregs[current_thread][n].u32[2], fregs[current_thread][n].u32[3], \
        fregs[current_thread][n].u32[4], fregs[current_thread][n].u32[5], \
        fregs[current_thread][n].u32[6], fregs[current_thread][n].u32[7])

#define LOG_MREG(str, n) \
    LOG(DEBUG, "\tm%d " str " 0x%02lx", n, mregs[current_thread][n].to_ulong())

#define LOG_SCP(str, row, col) \
    LOG(DEBUG, "\tSCP[%d] " str " {" \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 " %d:0x%08" PRIx32 \
        " }", row, \
        (col)+0, SCP[row].u32[(col)+0], (col)+1, SCP[row].u32[(col)+1], \
        (col)+2, SCP[row].u32[(col)+2], (col)+3, SCP[row].u32[(col)+3], \
        (col)+4, SCP[row].u32[(col)+4], (col)+5, SCP[row].u32[(col)+5], \
        (col)+6, SCP[row].u32[(col)+6], (col)+7, SCP[row].u32[(col)+7])

#define LOG_FREG_OTHER(other_thread, str, n) \
    LOG(DEBUG, "\tf%d " str " {" \
        " 0:0x%08" PRIx32 " 1:0x%08" PRIx32 " 2:0x%08" PRIx32 " 3:0x%08" PRIx32 \
        " 4:0x%08" PRIx32 " 5:0x%08" PRIx32 " 6:0x%08" PRIx32 " 7:0x%08" PRIx32 \
        " }", n, \
        fregs[other_thread][n].u32[0], fregs[other_thread][n].u32[1], \
        fregs[other_thread][n].u32[2], fregs[other_thread][n].u32[3], \
        fregs[other_thread][n].u32[4], fregs[other_thread][n].u32[5], \
        fregs[other_thread][n].u32[6], fregs[other_thread][n].u32[7])

#define LOG_FFLAGS(str, n) \
    LOG(DEBUG, "\tfflags " str " 0x%" PRIx8, uint8_t(n))

#define LOG_PC(str) \
    LOG(DEBUG, "\tpc " str " 0x%" PRIx64, PC)

#define LOG_GSC_PROGRESS(str) \
    LOG(DEBUG, "\tgsc_progress " str " %u", unsigned(csr_gsc_progress[current_thread]))

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

#define X2      xregs[current_thread][2]
#define RD      xregs[current_thread][inst.rd()]
#define RS1     xregs[current_thread][inst.rs1()]
#define RS2     xregs[current_thread][inst.rs2()]
#define C_RS1   xregs[current_thread][inst.rvc_rs1()]
#define C_RS1P  xregs[current_thread][inst.rvc_rs1p()]
#define C_RS2   xregs[current_thread][inst.rvc_rs2()]
#define C_RS2P  xregs[current_thread][inst.rvc_rs2p()]

#define M0      mregs[current_thread][0]
#define MD      mregs[current_thread][inst.md()]
#define MS1     mregs[current_thread][inst.ms1()]
#define MS2     mregs[current_thread][inst.ms2()]

#define FD      fregs[current_thread][inst.fd()]
#define FS1     fregs[current_thread][inst.fs1()]
#define FS2     fregs[current_thread][inst.fs2()]
#define FS3     fregs[current_thread][inst.fs3()]

#define FRM     ((csr_fcsr[current_thread] >> 5) & 7)


#define set_rounding_mode(expr) do { \
    extern uint32_t current_inst; \
    uint_fast8_t round = (expr); \
    if (round == 7) round = FRM; \
    if (round > 4) throw trap_illegal_instruction(current_inst); \
    softfloat_roundingMode = round; \
} while (0)


#define require_fp_active() do { \
    extern uint32_t current_inst; \
    if ((csr_mstatus[current_thread] & 0x0006000ULL) == 0) \
        throw trap_illegal_instruction(current_inst); \
} while (0)


// -----------------------------------------------------------------------------
// Write destination registers

#define WRITE_PC(expr) do { \
    uint64_t pc = sextVA(expr); \
    PROFILING_WRITE_PC(current_thread, pc); \
    log_pc_update(pc); \
} while (0)


#define WRITE_REG(n, expr) do { \
    if (n != 0) { \
        xregs[current_thread][n] = (expr); \
        LOG_REG("=", n); \
    } \
    log_xreg_write(n, xregs[current_thread][n]); \
} while (0)


#define WRITE_X0(expr)      WRITE_REG(0, expr)
#define WRITE_X1(expr)      WRITE_REG(1, expr)
#define WRITE_X2(expr)      WRITE_REG(2, expr)
#define WRITE_RD(expr)      WRITE_REG(inst.rd(), expr)
#define WRITE_C_RS1(expr)   WRITE_REG(inst.rvc_rs1(), expr)
#define WRITE_C_RS1P(expr)  WRITE_REG(inst.rvc_rs1p(), expr)
#define WRITE_C_RS2P(expr)  WRITE_REG(inst.rvc_rs2p(), expr)


#define WRITE_MREG(n, expr) do { \
    mregs[current_thread][n] = (expr); \
    LOG_MREG("=", n); \
    log_mreg_write(n, mregs[current_thread][n]); \
} while (0)


#define WRITE_MD(expr)  WRITE_MREG(inst.md(), expr)


#define WRITE_FD(expr) do { \
    FD.u32[0] = fpu::UI32(expr); \
    for (size_t e = 1; e < MLEN; ++e) { \
        FD.u32[e] = 0; \
    } \
    LOG_FREG("=", inst.fd()); \
    dirty_fp_state(); \
    log_freg_write(inst.fd(), FD); \
} while (0)


#define WRITE_VD(expr) do { \
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
    log_freg_write(inst.fd(), FD); \
} while (0)


#define SCATTER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    for (size_t e = csr_gsc_progress[current_thread]; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                expr; \
            } \
            catch (const trap_t&) { \
                csr_gsc_progress[current_thread] = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                throw; \
            } \
        } else { \
            log_mem_write(false, -1, 0, 0); \
        } \
    } \
    csr_gsc_progress[current_thread] = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
} while (0)


#define GATHER(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (size_t e = csr_gsc_progress[current_thread]; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                csr_gsc_progress[current_thread] = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                if (dirty) { \
                    LOG_FREG("=", inst.fd()); \
                    dirty_fp_state(); \
                    log_freg_write(inst.fd(), FD); \
                } \
                throw; \
            } \
        } \
    } \
    csr_gsc_progress[current_thread] = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
    if (dirty) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_write(inst.fd(), FD); \
} while (0)


#define GSCAMO(expr) do { \
    LOG_GSC_PROGRESS(":"); \
    bool dirty = false; \
    for (size_t e = csr_gsc_progress[current_thread]; e < MLEN; ++e) { \
        if (M0[e]) { \
            try { \
                FD.u32[e] = fpu::UI32(expr); \
                dirty = true; \
            } \
            catch (const trap_t&) { \
                csr_gsc_progress[current_thread] = e; \
                LOG_GSC_PROGRESS("="); \
                log_gsc_progress(e); \
                if (dirty) { \
                    LOG_FREG("=", inst.fd()); \
                    dirty_fp_state(); \
                    log_freg_write(inst.fd(), FD); \
                } \
                throw; \
            } \
        } else { \
            log_mem_write(false, -1, 0, 0); \
        } \
    } \
    csr_gsc_progress[current_thread] = 0; \
    LOG_GSC_PROGRESS("="); \
    log_gsc_progress(0); \
    if (dirty) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_write(inst.fd(), FD); \
} while (0)


#define WRITE_VD_NOMASK(expr) do { \
    for (size_t e = 0; e < MLEN; ++e) { \
        FD.u32[e] = fpu::UI32(expr); \
    } \
    LOG_FREG("=", inst.fd()); \
    dirty_fp_state(); \
    log_freg_write(inst.fd(), FD); \
} while (0)


#define WRITE_VD_NODATA(msk) do { \
    if (msk.any()) { \
        LOG_FREG("=", inst.fd()); \
        dirty_fp_state(); \
    } \
    log_freg_write(inst.fd(), FD); \
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
    } \
    log_mreg_write(inst.md(), MD); \
} while (0)


#define dirty_fp_state() do { \
    csr_mstatus[current_thread] |= 0x8000000000006000ULL; \
} while (0)


#define set_fp_exceptions() do { \
    if (softfloat_exceptionFlags) { \
        dirty_fp_state(); \
        uint32_t newval = \
                (softfloat_exceptionFlags & 0x1F) | \
                (uint32_t(softfloat_exceptionFlags & 0x20) << 26); \
        csr_fcsr[current_thread] |= newval; \
        LOG_FFLAGS("|=", newval); \
        log_fflags_write(newval); \
        softfloat_exceptionFlags = 0; \
    } \
} while (0)


// -----------------------------------------------------------------------------
// Disassemble instruction, and input operands

#define PRVNAME ("USHM"[csr_prv[current_thread] & 3])
#define RMNAME  (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0dyn"[((RM==7) ? FRM : RM) * 4])
#define FRMNAME (&"rne\0rtz\0rdn\0rup\0rmm\0rm5\0rm6\0rm7"[FRM * 4])


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
    LOG(DEBUG, "I(%c): " name " f%d,f%d [%s]", PRVNAME, inst.fd(), inst.fs1(), FRMNAME); \
    LOG_FREG(":", inst.fs1()); \
} while (0)

#define DISASM_FD_FS1_FS2(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d", PRVNAME, inst.fd(), inst.fs1(), inst.fs2()); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_FS2_FRM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d [%s]", PRVNAME, inst.fd(), inst.fs1(), inst.fs2(), FRMNAME); \
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
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
    LOG_FREG(":", inst.fs3()); \
} while (0)

#define DISASM_FD_FS1_FS2_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,f%d,%s", PRVNAME, inst.fd(), inst.fs1(), inst.fs2(), RMNAME); \
    LOG_FREG(":", inst.fs1()); \
    LOG_FREG(":", inst.fs2()); \
} while (0)

#define DISASM_FD_FS1_RM(name) do { \
    LOG(DEBUG, "I(%c): " name " f%d,f%d,%s", PRVNAME, inst.fd(), inst.fs1(), RMNAME); \
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
    LOG(DEBUG, "I(%c): " name " f%d,f%d,(x%d)", PRVNAME, inst.fd(), inst.fs1(), inst.rs2()); \
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
