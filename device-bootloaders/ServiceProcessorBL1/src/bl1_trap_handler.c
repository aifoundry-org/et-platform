/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "trap_handler.h"

#include <inttypes.h>

#include "printx.h"

uint64_t mtrap_handler(uint64_t cause, uint64_t epc, uint64_t mtval, uint64_t *regs)
{
    // only handling reading of mhartid
    if (cause == CAUSE_ILLEGAL_INSTRUCTION &&
        (mtval & INST_READ_MHARTID_MASK) == INST_READ_MHARTID_MATCH) {
        uint64_t rd, id;

        printx("SP BL1 mtrap_handler: illegal instruction MHARTID\r\n");

        rd = (mtval >> 7) & 0x1F;

        __asm__ __volatile__("csrr %[id], mhartid\n" : [id] "=r"(id));

        regs[rd] = id;
    } else if (cause == CAUSE_ILLEGAL_INSTRUCTION &&
               (mtval & INST_WRITE_TXSLEEP27_MASK) == INST_WRITE_TXSLEEP27_MATCH) {
        uint64_t rd, rs, val;

        printx("SP BL1 mtrap_handler: illegal instruction txsleep27\r\n");

        rd = (mtval >> 7) & 0x1F;
        rs = (mtval >> 15) & 0x1F;

        __asm__ __volatile__("csrrw %[val], 0x7d1, %[rs] \n"
                             : [val] "=r"(val)
                             : [rs] "r"(regs[rs]));

        regs[rd] = val;
    } else {
        printx("SP BL1 mtrap_handler: other cause (cause=0x%x, epc=0x%x, mtval=0x%x)\r\n", cause,
               epc, mtval);

        // any other cause => wfi and expect timeout in test
        __asm__ __volatile__("wfi\n");
    }

    return epc +
           4; // should return +2 if compressed... because only implementing csrXX, return 4 directly
}
