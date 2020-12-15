/*-------------------------------------------------------------------------
* Copyright (C) 2018,2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef _TRAP_HANDLER_H_
#define _TRAP_HANDLER_H_

#include <stdint.h>

#define CAUSE_ILLEGAL_INSTRUCTION 2

//                                     mhartid  |    rs1=0     |        csrrs
#define INST_READ_MHARTID_MASK  ((0xFFFULL<<20) | (0x1f << 15) | (0x7<<12) | (0x7f))
#define INST_READ_MHARTID_MATCH ((0xF14ULL<<20) | (0<<15)      | (0x2<<12) | (0x73))

#define INST_WRITE_TXSLEEP27_MASK  ((0xFFFULL<<20) | (0x7<<12) | (0x7f))
#define INST_WRITE_TXSLEEP27_MATCH ((0x7D1ULL<<20) | (0x1<<12) | (0x73))

uint64_t mtrap_handler(uint64_t cause, uint64_t epc, uint64_t mtval, uint64_t *regs);

void mtrap_vector(void);

#endif
