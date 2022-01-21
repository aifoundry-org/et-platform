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

#endif /* DEBUG_INSTRUCTION_SEQUENCE_H */