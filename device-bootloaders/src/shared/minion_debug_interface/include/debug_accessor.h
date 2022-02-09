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
/*! \file debug_accessor.h
    \brief A C header that defines the functions used internally by 
    run control and state inspections APIs
*/
/***********************************************************************/
#ifndef DEBUG_ACCESSOR_H
#define DEBUG_ACCESSOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esr.h"
#include "etsoc_hal/inc/minion_csr.h"
#include "hwinc/sp_misc.h"
#include "hwinc/etsoc_shire_other_esr.h"
#include "etsoc/isa/io.h"
#include <system/layout.h>
#include "bl_error_code.h"

#define HARTS_PER_NEIGH     16
#define MINIONS_PER_NEIGH   HARTS_PER_NEIGH / 2
#define NUM_NEIGH_PER_SHIRE 4
#define MAX_RETRIES         1000
#define TDATA1(mode)        (((uint64_t)1 << 59) | (1 << 12) | (0 << 7) | ((mode & 7) << 3) | (1 << 2))
#define RUNNING(treel2)     (treel2 >> 5 & 1U)
#define HALTED(treel2)      (treel2 >> 3 & 1U)

#define DMACTIVE         (1)
#define RESUMEREQ        (1U << 30)
#define DMCTRL_RES_MASK  (RESUMEREQ | DMACTIVE)
#define HALTREQ          (1U << 31)
#define DMCTRL_HALT_MASK (HALTREQ | DMACTIVE)

#define UNSELECT_HART_OP(hactrl, mask) (hactrl & (~(mask | (mask << 16)) & 0xFFFFUL))
#define SELECT_HART_OP(hactrl, mask)   (hactrl | mask | mask << 16)

#define ABSCMD       0xf8
#define NXDATA0      0xf0
#define NXDATA1      0xf1
#define ABSCMD_BYTE  (ABSCMD << 3)
#define NXDATA0_BYTE (NXDATA0 << 3)
#define NXDATA1_BYTE (NXDATA1 << 3)

#define BITS64_ALLF_MASK 0xFFFFFFFFFFFFFFFFUL

#define NEIGH_ID_0 0
#define NEIGH_ID_1 1
#define NEIGH_ID_2 2
#define NEIGH_ID_3 3

#define PER_NEIGH_MINION_MASK 0xFFU
#define DISABLE_MINION_MASK(neigh_id) \
    (PER_NEIGH_MINION_MASK << (MINIONS_PER_NEIGH * neigh_id))
#define ENABLE_MINION_MASK(neigh_id) ~DISABLE_MINION_MASK(neigh_id)

#define ALL_MINIONS_MASK 0xFFFFU

#define GET_THREAD_ID(hart_id) (hart_id & 1)
#define GET_MINION_ID(hart_id) (hart_id >> 1 & 31)
#define GET_SHIRE_ID(hart_id)  (hart_id >> 6 & 255)
#define GET_NEIGH_ID(hart_id)  (hart_id >> 4 & 3)

#define NO_OF_GPR_REGS     32
#define GDB_RISCV_PC_INDEX 32

#define IS_XCPT_FIELD 16

#define WAIT_COND(COND, REPEAT)     \
    ({                              \
        int32_t __repeat = REPEAT;  \
        bool __cond_meet = false;   \
        while (__repeat--)          \
        {                           \
            if (COND)               \
            {                       \
                __cond_meet = true; \
                break;              \
            }                       \
        }                           \
        __cond_meet;                \
    })

#define WAIT(COND) WAIT_COND(COND, MAX_RETRIES)

#define READ_HACTRL(shire_id, neigh_id)                                 \
    read_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                 ETSOC_NEIGH_ESR_HACTRL_BYTE_ADDRESS, 0)

#define WRITE_HACTRL(shire_id, neigh_id, data)                           \
    write_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                  ETSOC_NEIGH_ESR_HACTRL_BYTE_ADDRESS, data, 0)

#define READ_THREAD0_DISABLE(shire_id)                                                     \
    (uint32_t) read_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                            ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_BYTE_ADDRESS, 0)

#define READ_THREAD1_DISABLE(shire_id)                                                     \
    (uint32_t) read_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                            ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_BYTE_ADDRESS, 0)

#define WRITE_THREAD0_DISABLE(shire_id, data)                                    \
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                  ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_BYTE_ADDRESS, data, 0)

#define WRITE_THREAD1_DISABLE(shire_id, data)                                    \
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                  ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_BYTE_ADDRESS, data, 0)

#define READ_HASTATUS0(shire_id, neigh_id)                              \
    read_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                 ETSOC_NEIGH_ESR_HASTATUS0_BYTE_ADDRESS, 0)

#define HART_HALT_STATUS(hart_id)                                     \
    ETSOC_NEIGH_ESR_HASTATUS0_HALTED_GET(                             \
        READ_HASTATUS0(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define HART_RUNNING_STATUS(hart_id)                                  \
    ETSOC_NEIGH_ESR_HASTATUS0_RUNNING_GET(                            \
        READ_HASTATUS0(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define HART_RESUME_STATUS(hart_id)                                   \
    ETSOC_NEIGH_ESR_HASTATUS0_RESUMEACK_GET(                          \
        READ_HASTATUS0(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define HART_RESET_STATUS(hart_id)                                    \
    ETSOC_NEIGH_ESR_HASTATUS0_HAVERESET_GET(                          \
        READ_HASTATUS0(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define WRITE_HASTATUS0(shire_id, neigh_id, data)                        \
    write_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                  ETSOC_NEIGH_ESR_HASTATUS0_BYTE_ADDRESS, data, 0)

#define READ_HASTATUS1(shire_id, neigh_id)                              \
    read_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                 ETSOC_NEIGH_ESR_HASTATUS1_BYTE_ADDRESS, 0)

#define HART_ERROR_STATUS(hart_id)                                    \
    ETSOC_NEIGH_ESR_HASTATUS1_ERROR_GET(                              \
        READ_HASTATUS1(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define HART_EXCEPTION_STATUS(hart_id)                                \
    ETSOC_NEIGH_ESR_HASTATUS1_EXCEPTION_GET(                          \
        READ_HASTATUS1(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define HART_BUSY_STATUS(hart_id)                                     \
    ETSOC_NEIGH_ESR_HASTATUS1_BUSY_GET(                               \
        READ_HASTATUS1(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id))) \
    &(hart_id % HARTS_PER_NEIGH);

#define WRITE_HASTATUS1(shire_id, neigh_id, data)                        \
    write_esr_new(PP_MESSAGES, shire_id, REGION_NEIGHBOURHOOD, neigh_id, \
                  ETSOC_NEIGH_ESR_HASTATUS1_BYTE_ADDRESS, data, 0)

/* Functions required by minion run control APIs*/
uint32_t read_andortreel2(void);
uint32_t read_dmctrl(void);
void write_dmctrl(uint32_t data);
void assert_halt(void);
void deassert_halt(void);
void select_hart_op(uint8_t shire_id, uint8_t neigh_id, uint16_t hart_mask);
void unselect_hart_op(uint8_t shire_id, uint8_t neigh_id, uint16_t hart_mask);
void disable_shire_threads(uint8_t shire_id);
void enable_shire_threads(uint8_t shire_id);
uint64_t get_enabled_harts(uint8_t shire_id);

/* Functions required by minion state control APIs*/
uint64_t read_nxdata0(uint64_t hart_id);
void write_nxdata0(uint64_t hart_id, uint64_t data);
uint64_t read_nxdata1(uint64_t hart_id);
void write_nxdata1(uint64_t hart_id, uint64_t data);
void execute_instructions(uint64_t hart_id, const uint32_t *inst_list, uint32_t num_inst);
uint64_t read_ddata(uint64_t hart_id);
void write_ddata(uint64_t hart_id, uint64_t data);

#endif /* DEBUG_ACCESSOR_H */
