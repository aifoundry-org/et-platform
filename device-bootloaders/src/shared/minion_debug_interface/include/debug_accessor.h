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
#include "etsoc_hal/inc/hal_device.h"
#include "etsoc_hal/inc/etsoc_neigh_esr.h"
#include "etsoc_hal/inc/spio_misc_esr.h"
#include "etsoc_hal/inc/etsoc_shire_other_esr.h"
#include "etsoc_hal/inc/minion_csr.h"
#include "etsoc/isa/io.h"
#include <system/layout.h>

#define HARTS_PER_NEIGH     16
#define NUM_NEIGH_PER_SHIRE 4
#define MAX_RETRIES         5
#define THREAD_0_MASK       0x55555555UL
#define THREAD_1_MASK       0xAAAAAAAAUL
#define TDATA1(mode)        (((uint64_t)1 << 59) | (1 << 12) | (0 << 7) | ((mode & 7) << 3) | (1 << 2))
#define RUNNING(treel2)     (treel2 >> 5 & 1U)
#define HALTED(treel2)      (treel2 >> 3 & 1U)

#define DMACTIVE         (1)
#define RESUMEREQ        (1U << 30)
#define DMCTRL_RES_MASK  (RESUMEREQ | DMACTIVE)
#define HALTREQ          (1ULL << 31)
#define DMCTRL_HALT_MASK (HALTREQ | DMACTIVE)

#define UNSELECT_HART_OP(hactrl, mask) (hactrl & (~(mask | (mask << 16)) & 0xFFFFUL))
#define SELECT_HART_OP(hactrl, mask)   (hactrl | mask | mask << 16)

#define ABSCMD           0xf8
#define NXDATA0          0xf0
#define NXDATA1          0xf1
#define BITS64_ALLF_MASK 0xFFFFFFFFFFFFFFFFUL

#define NEIGH_ID_0 0
#define NEIGH_ID_1 1
#define NEIGH_ID_2 2
#define NEIGH_ID_3 3

#define NEIGH_MASK(thread_mask, neigh_id) (thread_mask >> neigh_id & 0xFFFF)

#define GET_THREAD_ID(hart_id) (hart_id & 1)
#define GET_MINION_ID(hart_id) (hart_id >> 1 & 31)
#define GET_SHIRE_ID(hart_id)  (hart_id >> 6 & 255)
#define GET_NEIGH_ID(hart_id)  (hart_id >> 4 & 3)

#define IS_XCPT_FIELD 16

#define READ_HACTRL(shire_id, neigh_id)                                                       \
    read_esr_new(PP_MESSAGES, (uint8_t)(shire_id), REGION_NEIGHBOURHOOD, (uint8_t)(neigh_id), \
                 ETSOC_NEIGH_ESR_HACTRL_ADDRESS, 0)

#define WRITE_HACTRL(shire_id, neigh_id, data)                                                 \
    write_esr_new(PP_MESSAGES, (uint8_t)(shire_id), REGION_NEIGHBOURHOOD, (uint8_t)(neigh_id), \
                  ETSOC_NEIGH_ESR_HACTRL_ADDRESS, data, 0)

#define READ_THREAD0_DISABLE(shire_id)                                                     \
    read_esr_new(PP_MACHINE, (uint8_t)(shire_id), REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                 ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_ADDRESS, 0)

#define READ_THREAD1_DISABLE(shire_id)                                                     \
    read_esr_new(PP_MACHINE, (uint8_t)(shire_id), REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                 ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_ADDRESS, 0)

#define WRITE_THREAD0_DISABLE(shire_id, data)                                               \
    write_esr_new(PP_MACHINE, (uint8_t)(shire_id), REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                  ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_ADDRESS, data, 0)

#define WRITE_THREAD1_DISABLE(shire_id, data)                                               \
    write_esr_new(PP_MACHINE, (uint8_t)(shire_id), REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, \
                  ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_ADDRESS, data, 0)

#define READ_HASTATUS1(shire_id, neigh_id)                                                    \
    read_esr_new(PP_MESSAGES, (uint8_t)(shire_id), REGION_NEIGHBOURHOOD, (uint8_t)(neigh_id), \
                 ETSOC_NEIGH_ESR_HASTATUS1_ADDRESS, 0)

#define WRITE_HASTATUS1(shire_id, neigh_id, data)                                              \
    write_esr_new(PP_MESSAGES, (uint8_t)(shire_id), REGION_NEIGHBOURHOOD, (uint8_t)(neigh_id), \
                  ETSOC_NEIGH_ESR_HASTATUS1_ADDRESS, data, 0)

/* Functions required by minion run control APIs*/
uint64_t read_andortreel2(void);
uint64_t read_dmctrl(void);
void write_dmctrl(uint64_t data);
void assert_halt(void);
void deassert_halt(void);
bool wait_till_core_halt(void);
void select_hart_op(uint64_t shire_id, uint64_t neigh_id, uint16_t hart_mask);
void unselect_hart_op(uint64_t shire_id, uint64_t neigh_id, uint16_t hart_mask);
bool workarround_resume_pre(void);
bool workarround_resume_post(void);

/* Functions required by minion state control APIs*/
uint64_t read_nxdata0(uint64_t hart_id);
void write_nxdata0(uint64_t hart_id, uint64_t data);
uint64_t read_nxdata1(uint64_t hart_id);
void write_nxdata1(uint64_t hart_id, uint64_t data);
void execute_instructions(uint64_t hart_id, const uint32_t *inst_list, uint32_t num_inst);
uint64_t read_ddata(uint64_t hart_id);
void write_ddata(uint64_t hart_id, uint64_t data);

#endif /* DEBUG_ACCESSOR_H */