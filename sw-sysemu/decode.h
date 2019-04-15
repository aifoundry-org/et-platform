/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */
#ifndef BEMU_DECODE_H
#define BEMU_DECODE_H

#include "profiling.h"

// -----------------------------------------------------------------------------
// Convenience macros to simplify instruction emulation sequences
// -----------------------------------------------------------------------------

// Log operands

#define LOG_REG(str, n) \
    LOG_IF(DEBUG, n != x0, "\tx%d " str " 0x%" PRIx64, n, xregs[current_thread][n])

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

#define LOG_MREG0 \
    LOG(DEBUG, "\tm0 : 0x%02lx", mregs[current_thread][0].to_ulong())

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


// Access instruction fields and program state

#define PC      current_pc
#define NPC     sextVA(current_pc + 4/*FIXME: inst.size()*/)
#define C_NPC   sextVA(current_pc + 2/*FIXME: inst.size()*/)

#define BIMM    b_imm
#define F32IMM  f32imm
#define I32IMM  i32imm
#define IIMM    i_imm
#define JIMM    j_imm
#define SHAMT5  shamt5
#define SHAMT6  shamt6
#define UIMM    u_imm
#define UIMM3   uimm3
#define UIMM8   uimm8
#define UMSK4   umsk4
#define VIMM    v_imm

#define RS1     xregs[current_thread][rs1]
#define RS2     xregs[current_thread][rs2]

#define M0      mregs[current_thread][0]
#define MD      mregs[current_thread][md]
#define MS1     mregs[current_thread][ms1]
#define MS2     mregs[current_thread][ms2]

#define FD      fregs[current_thread][fd]
#define FS1     fregs[current_thread][fs1]
#define FS2     fregs[current_thread][fs2]
#define FS3     fregs[current_thread][fs3]


// Write destination registers

#define WRITE_PC(expr) do { \
    uint64_t pc = sextVA(expr); \
    PROFILING_WRITE_PC(current_thread, pc); \
    log_pc_update(pc); \
} while (0)

#define WRITE_RD(expr) do { \
    if (rd != x0) { \
        xregs[current_thread][rd] = (expr); \
        LOG_REG("=", rd); \
    } \
    log_xreg_write(rd, xregs[current_thread][rd]); \
} while (0)

#define WRITE_MD(expr) do { \
    mregs[current_thread][md] = (expr); \
    LOG_MREG("=", md); \
    log_mreg_write(md, mregs[current_thread][md]); \
} while (0)

#define WRITE_MREG(n, expr) do { \
    mregs[current_thread][n] = (expr); \
    LOG_MREG("=", n); \
    log_mreg_write(n, mregs[current_thread][n]); \
} while (0)

#define WRITE_FD(expr) do { \
    fregs[current_thread][fd].u32[0] = fpu::UI32(expr); \
    for (size_t e = 1; e < MLEN; ++e) { \
        fregs[current_thread][fd].u32[e] = 0; \
    } \
    LOG_FREG("=", fd); \
    dirty_fp_state(); \
    log_freg_write(fd, fregs[current_thread][fd]); \
} while (0)

#define WRITE_VD(expr) do { \
    LOG_MREG0; \
    if (mregs[current_thread][0].any()) { \
        for (size_t e = 0; e < MLEN; ++e) { \
            if (mregs[current_thread][0][e]) { \
                fregs[current_thread][fd].u32[e] = fpu::UI32(expr); \
            } \
        } \
        LOG_FREG("=", fd); \
        dirty_fp_state(); \
    } \
    log_freg_write(fd, fregs[current_thread][fd]); \
} while (0)

#define WRITE_VD_NOMASK(expr) do { \
    for (size_t e = 0; e < MLEN; ++e) { \
        fregs[current_thread][fd].u32[e] = fpu::UI32(expr); \
    } \
    LOG_FREG("=", fd); \
    dirty_fp_state(); \
    log_freg_write(fd, fregs[current_thread][fd]); \
} while (0)

#define WRITE_VMD(expr) do { \
    LOG_MREG0; \
    if (mregs[current_thread][0].any()) { \
        for (size_t e = 0; e < MLEN; ++e) { \
            if (mregs[current_thread][0][e]) { \
                mregs[current_thread][md][e] = (expr); \
            } \
        } \
        LOG_MREG("=", md); \
    } \
    log_mreg_write(md, mregs[current_thread][md]); \
} while (0)


// Disassemble instruction, and input operands

#define DISASM_RD_ALLMASK(name) do { \
    LOG(DEBUG, "I: %s", name); \
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
    LOG(DEBUG, "I: " name " x%d,x%d,%" PRId64, rs1, rs2, b_imm); \
    LOG_REG(":", rs1); \
    LOG_REG(":", rs2); \
} while (0)

#define DISASM_RD_JIMM(name) \
    LOG(DEBUG, "I: " name " x%d,%" PRId64, rd, j_imm)

#define DISASM_RD_RS1_IIMM(name) do { \
    LOG(DEBUG, "I: " name " x%d,x%d,%" PRId64, rd, rs1, i_imm); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_RD_RS1_RS2(name) do { \
    LOG(DEBUG, "I: " name " x%d,x%d,x%d", rd, rs1, rs2); \
    LOG_REG(":", rs1); \
    LOG_REG(":", rs2); \
} while (0)

#define DISASM_RD_RS1_SHAMT5(name) do { \
    LOG(DEBUG, "I: " name " x%d,x%d,0x%x", rd, rs1, shamt5); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_RD_RS1_SHAMT6(name) do { \
    LOG(DEBUG, "I: " name " x%d,x%d,0x%x", rd, rs1, shamt6); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_RD_UIMM(name) \
    LOG(DEBUG, "I: " name " x%d,0x%" PRIx64, rd, u_imm)

#define DISASM_RS1(name) do { \
    LOG(DEBUG, "I: " name " x%d", rs1); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_MD_FS1(name) do { \
    LOG(DEBUG, "I: " name " m%d,f%d", md, fs1); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_MD_FS1_FS2(name) do { \
    LOG(DEBUG, "I: " name " m%d,f%d,f%d", md, fs1, fs2); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_MD_MS1(name) do { \
    LOG(DEBUG, "I: " name " m%d,m%d", md, ms1); \
    LOG_MREG(":", ms1); \
} while (0)

#define DISASM_MD_MS1_MS2(name) do { \
    LOG(DEBUG, "I: " name " m%d,m%d,m%d", md, ms1, ms2); \
    LOG_MREG(":", ms1); \
    LOG_MREG(":", ms2); \
} while (0)

#define DISASM_MD_RS1_UIMM8(name) do { \
    LOG(DEBUG, "I: " name " m%d,x%d,0x%x", md, rs1, uimm8); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_RD_MS1(name) do { \
    LOG(DEBUG, "I: " name " x%d,m%d", rd, ms1); \
    LOG_MREG(":", ms1); \
} while (0)

#define DISASM_RD_MS1_MS2_UMSK4(name) do { \
    LOG(DEBUG, "I: " name " x%d,m%d,m%d,0x%x", rd, ms1, ms2, umsk4); \
    LOG_MREG(":", ms1); \
    LOG_MREG(":", ms2); \
} while (0)

#define DISASM_FD_F32IMM(name) do { \
    LOG(DEBUG, "I: " name " f%d,0x%x", fd, f32imm); \
} while (0)

#define DISASM_FD_FS1(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d", fd, fs1); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_FS1_FRM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d [%s]", fd, fs1, get_rounding_mode(frm())); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_FS1_FS2(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d", fd, fs1, fs2); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_FD_FS1_FS2_FRM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d [%s]", fd, fs1, fs2, get_rounding_mode(frm())); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_FDS0_FS1_FS2(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d", fd, fs1, fs2); \
    LOG_FREG(":", fd); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d,f%d", fd, fs1, fs2, fs3); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
    LOG_FREG(":", fs3); \
} while (0)

#define DISASM_FD_FS1_FS2_FS3_RM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d,f%d,%s", fd, fs1, fs2, fs3, get_rounding_mode(rm)); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
    LOG_FREG(":", fs3); \
} while (0)

#define DISASM_FD_FS1_FS2_RM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,f%d,%s", fd, fs1, fs2, get_rounding_mode(rm)); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_FD_FS1_RM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,%s", fd, fs1, get_rounding_mode(rm)); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_FS1_UIMM8(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,0x%x", fd, fs1, uimm8); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_FS1_VIMM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,%d", fd, fs1, v_imm); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_FS1_UVIMM(name) do { \
    LOG(DEBUG, "I: " name " f%d,f%d,0x%x", fd, fs1, v_imm); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_FD_I32IMM(name) do { \
    LOG(DEBUG, "I: " name " f%d,%d", fd, i32imm); \
} while (0)

#define DISASM_FD_RS1(name) do { \
    LOG(DEBUG, "I: " name " f%d,x%d", fd, rs1); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_FD_RS1_RM(name) do { \
    LOG(DEBUG, "I: " name " f%d,x%d,%s", fd, rs1, get_rounding_mode(rm)); \
    LOG_REG(":", rs1); \
} while (0)

#define DISASM_RD_FS1(name) do { \
    LOG(DEBUG, "I: " name " x%d,f%d", rd, fs1); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_RD_FS1_FS2(name) do { \
    LOG(DEBUG, "I: " name " x%d,f%d,f%d", rd, fs1, fs2); \
    LOG_FREG(":", fs1); \
    LOG_FREG(":", fs2); \
} while (0)

#define DISASM_RD_FS1_RM(name) do { \
    LOG(DEBUG, "I: " name " x%d,f%d,%s", rd, fs1, get_rounding_mode(rm)); \
    LOG_FREG(":", fs1); \
} while (0)

#define DISASM_RD_FS1_UIMM3(name) do { \
    LOG(DEBUG, "I: " name " x%d,f%d,0x%x", rd, fs1, uimm3); \
    LOG_FREG(":", fs1); \
} while (0)


#endif // BEMU_DECODE_H
