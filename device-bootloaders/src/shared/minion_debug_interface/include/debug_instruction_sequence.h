/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file debug_instruction_sequence.h
    \brief A C header that defines the instruction sequence for read/write
     for GPR/CSRs
*/
/***********************************************************************/

#ifndef DEBUG_INSTRUCTION_SEQUENCE_H
#define DEBUG_INSTRUCTION_SEQUENCE_H

/* Ebreak Instruction Sequence */
#define EBREAK_INST 0x100073

/* GRP Read Instruction Sequence */
#define READ_GPR_INSTR 0x7b801073
#define READ_GPR_SEQ(reg)                                               \
    /* Issue Read GRP instruction */                                    \
    (READ_GPR_INSTR | (reg << 15)), /* Insert Ebreak to align buffer */ \
        EBREAK_INST
#define NUM_INST_GPR_READ_SEQ 2

/* GPR Write Instruction Sequence */
#define WRITE_GPR_INSTR 0x7b802073
#define WRITE_GPR_SEQ(reg)                                              \
    /* Issue GPR Write instruction */                                   \
    (WRITE_GPR_INSTR | (reg << 7)), /* Insert Ebreak to align buffer */ \
        EBREAK_INST
#define NUM_INST_GPR_WRITE_SEQ 2

/* CSR Read Instruction Sequence */
#define READ_DDATA0 0x7b8f9073
#define READ_DDATA1 0x7b8f9ff3
#define READ_X31    0x2ff3
#define READ_CSR_SEQ(offset)                                             \
    /* Read DDATA0 Instruction */                                        \
    READ_DDATA0,                     /* Issue CSR Read Instruction */    \
        (READ_X31 | (offset << 20)), /* Read DDATA1 Instruction */       \
        READ_DDATA1,                 /* Insert Ebreak to align buffer */ \
        EBREAK_INST
#define NUM_INST_CSR_READ_SEQ 4

/* CSR Write Instruction Sequence */
#define WRITE_DDATA0 0x7b8f9ff3
#define WRITE_DDATA1 0x7b8f9ff3
#define WRITE_X31    0xf9073
#define WRITE_CSR_SEQ(offset)                                             \
    /* Write DDATA0 Instruction */                                        \
    WRITE_DDATA0,                     /* Issue CSR Read Instruction */    \
        (WRITE_X31 | (offset << 20)), /* Write DDATA1 Instruction */      \
        WRITE_DDATA1,                 /* Insert Ebreak to align buffer */ \
        EBREAK_INST
#define NUM_INST_CSR_WRITE_SEQ 4

/* VPU RF Init Sequence  */
#define CLR_MATP          0x7c005073 /* csrwi matp,0              */
#define CLR_SATP          0x18005073 /* csrwi satp,0              */
#define INIT_T0           0x000062b7 /* lui   t0,0x6              */
#define UPDATE_MSTATUS    0x3002a073 /* csrs  mstatus,t0          */
#define CLR_FCSR          0x00305073 /* csrwi fcsr,0              */
#define FMA_3PORT_RF_READ 0x00007043 /* fmadd.s f0, f0, f0, f0 */
#define FPU_CLR_M0        0x57f0707b /* mov.m.x m0, x0, 0xff */
#define FMA_CLR_RF_F0     0x0000705b /* fmadd.ps f0, f0, f0, f0 */
#define FMA_CLR_RF_F1     0x0810f0db /* fmadd.ps f1, f1, f1, f1 */
#define FMA_CLR_RF_F2     0x1021715b /* fmadd.ps f2, f2, f2, f2 */
#define FMA_CLR_RF_F3     0x1831f1db /* fmadd.ps f3, f3, f3, f3 */
#define FMA_CLR_RF_F4     0x2042725b /* fmadd.ps f4, f4, f4, f4 */
#define FMA_CLR_RF_F5     0x2852f2db /* fmadd.ps f5, f5, f5, f5 */
#define FMA_CLR_RF_F6     0x3063735b /* fmadd.ps f6, f6, f6, f6 */
#define FMA_CLR_RF_F7     0x3873f3db /* fmadd.ps f7, f7, f7, f7 */
#define FMA_CLR_RF_F8     0x4084745b /* fmadd.ps f8, f8, f8, f8 */
#define FMA_CLR_RF_F9     0x4894f4db /* fmadd.ps f9, f9, f9, f9 */
#define FMA_CLR_RF_F10    0x50a5755b /* fmadd.ps f10, f10, f10, f10 */
#define FMA_CLR_RF_F11    0x58b5f5db /* fmadd.ps f11, f11, f11, f11 */
#define FMA_CLR_RF_F12    0x60c6765b /* fmadd.ps f12, f12, f12, f12 */
#define FMA_CLR_RF_F13    0x68d6f6db /* fmadd.ps f13, f13, f13, f13 */
#define FMA_CLR_RF_F14    0x70e7775b /* fmadd.ps f14, f14, f14, f14 */
#define FMA_CLR_RF_F15    0x78f7f7db /* fmadd.ps f15, f15, f15, f15 */
#define FMA_CLR_RF_F16    0x8108785b /* fmadd.ps f16, f16, f16, f16 */
#define FMA_CLR_RF_F17    0x8918f8db /* fmadd.ps f17, f17, f17, f17 */
#define FMA_CLR_RF_F18    0x9129795b /* fmadd.ps f18, f18, f18, f18 */
#define FMA_CLR_RF_F19    0x9939f9db /* fmadd.ps f19, f19, f19, f19 */
#define FMA_CLR_RF_F20    0xa14a7a5b /* fmadd.ps f20, f20, f20, f20 */
#define FMA_CLR_RF_F21    0xa95afadb /* fmadd.ps f21, f21, f21, f21 */
#define FMA_CLR_RF_F22    0xb16b7b5b /* fmadd.ps f22, f22, f22, f22 */
#define FMA_CLR_RF_F23    0xb97bfbdb /* fmadd.ps f23, f23, f23, f23 */
#define FMA_CLR_RF_F24    0xc18c7c5b /* fmadd.ps f24, f24, f24, f24 */
#define FMA_CLR_RF_F25    0xc99cfcdb /* fmadd.ps f25, f25, f25, f25 */
#define FMA_CLR_RF_F26    0xd1ad7d5b /* fmadd.ps f26, f26, f26, f26 */
#define FMA_CLR_RF_F27    0xd9bdfddb /* fmadd.ps f27, f27, f27, f27 */
#define FMA_CLR_RF_F28    0xe1ce7e5b /* fmadd.ps f28, f28, f28, f28 */
#define FMA_CLR_RF_F29    0xe9defedb /* fmadd.ps f29, f29, f29, f29 */
#define FMA_CLR_RF_F30    0xf1ef7f5b /* fmadd.ps f30, f30, f30, f30 */
#define FMA_CLR_RF_F31    0xf9ffffdb /* fmadd.ps f31, f31, f31, f31 */
#define WFI_INST          0x10500073 /* wfi */

#define VPU_RF_INIT_SEQ()                                                                         \
    CLR_MATP, CLR_SATP, INIT_T0, UPDATE_MSTATUS, CLR_FCSR, FMA_3PORT_RF_READ, FPU_CLR_M0,         \
        FMA_CLR_RF_F0, FMA_CLR_RF_F1, FMA_CLR_RF_F2, FMA_CLR_RF_F3, FMA_CLR_RF_F4, FMA_CLR_RF_F5, \
        FMA_CLR_RF_F6, FMA_CLR_RF_F7, FMA_CLR_RF_F8, FMA_CLR_RF_F9, FMA_CLR_RF_F10,               \
        FMA_CLR_RF_F11, FMA_CLR_RF_F12, FMA_CLR_RF_F13, FMA_CLR_RF_F14, FMA_CLR_RF_F15,           \
        FMA_CLR_RF_F16, FMA_CLR_RF_F17, FMA_CLR_RF_F18, FMA_CLR_RF_F19, FMA_CLR_RF_F20,           \
        FMA_CLR_RF_F21, FMA_CLR_RF_F22, FMA_CLR_RF_F23, FMA_CLR_RF_F24, FMA_CLR_RF_F25,           \
        FMA_CLR_RF_F26, FMA_CLR_RF_F27, FMA_CLR_RF_F28, FMA_CLR_RF_F29, FMA_CLR_RF_F30,           \
        FMA_CLR_RF_F31, WFI_INST, EBREAK_INST
#define NUM_INST_VPU_RF_INIT_SEQ 41

/* Memory read instruction sequence */
#define GPR_REG_INDEX_A0             10
#define MINION_MEM_READ              0x00053503 /* ld a0,0(a0) */
#define MINION_MEM_READ_SEQ(addr)    MINION_MEM_READ, EBREAK_INST
#define NUM_INST_MINION_MEM_READ_SEQ 2

/* Minion Local Atomic Memory read instruction sequence */
#define LCL_ATOMIC_READ                     0x4005353b /* amoorl.d a0,zero,(a0) */
#define MINION_LCL_ATOMIC_READ_SEQ()        LCL_ATOMIC_READ, EBREAK_INST
#define NUM_INST_MINION_LCL_ATOMIC_READ_SEQ 2

/* Minion Global Atomic Memory read instruction sequence */
#define GLB_ATOMIC_READ                     0x4205353b /* amoorg.d a0,zero,(a0) */
#define MINION_GLB_ATOMIC_READ_SEQ(addr)    GLB_ATOMIC_READ, EBREAK_INST
#define NUM_INST_MINION_GLB_ATOMIC_READ_SEQ 2

#endif /* DEBUG_INSTRUCTION_SEQUENCE_H */
