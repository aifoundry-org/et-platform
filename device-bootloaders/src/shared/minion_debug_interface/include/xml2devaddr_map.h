/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

/* NOTE This file is to be auto-generated : SW-12321 */

// Looks table to map index from tools/debug_server/target.xml
// to Device CSR address space

#include "etsoc_hal/inc/minion_csr.h"

static const uint32_t csr_addr_lookup[112] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                               MINION_CSR_DPC_OFFSET, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, MINION_CSR_FFLAGS_ADDRESS,
                                               MINION_CSR_FRM_ADDRESS, MINION_CSR_FCSR_ADDRESS,
                                               /* User Mode CSRs are not implemented in ETSOC1 rev.
                                                  This shall be enabled in next rev of Silicon.
                                               MINION_CSR_USTATUS_ADDRESS,
                                               MINION_CSR_UIE_ADDRESS,
                                               MINION_CSR_UTVEC_ADDRESS,
                                               MINION_CSR_USCRATCH_ADDRESS,
                                               MINION_CSR_UEPC_ADDRESS,
                                               MINION_CSR_UCAUSE_ADDRESS,
                                               MINION_CSR_UTVAL_ADDRESS,
                                               MINION_CSR_UIP_ADDRESS,
                                               MINION_CSR_CYCLE_OFFSET,
                                               MINION_CSR_TIME_OFFSET,
                                               MINION_CSR_INSTRET_OFFSET,
                                               MINION_CSR_HPMCOUNTER3_OFFSET,
                                               MINION_CSR_HPMCOUNTER4_OFFSET,
                                               MINION_CSR_HPMCOUNTER5_OFFSET,
                                               MINION_CSR_HPMCOUNTER6_OFFSET,
                                               MINION_CSR_HPMCOUNTER7_OFFSET,
                                               MINION_CSR_HPMCOUNTER8_OFFSET,
                                               MINION_CSR_HPMCOUNTER9_OFFSET,
                                               MINION_CSR_HPMCOUNTER10_OFFSET,
                                               MINION_CSR_HPMCOUNTER11_OFFSET,
                                               MINION_CSR_HPMCOUNTER12_OFFSET,
                                               MINION_CSR_HPMCOUNTER13_OFFSET,
                                               MINION_CSR_HPMCOUNTER14_OFFSET,
                                               MINION_CSR_HPMCOUNTER15_OFFSET,
                                               MINION_CSR_HPMCOUNTER16_OFFSET,
                                               MINION_CSR_HPMCOUNTER17_OFFSET,
                                               MINION_CSR_HPMCOUNTER18_OFFSET,
                                               MINION_CSR_HPMCOUNTER19_OFFSET,
                                               MINION_CSR_HPMCOUNTER20_OFFSET,
                                               MINION_CSR_HPMCOUNTER21_OFFSET,
                                               MINION_CSR_HPMCOUNTER22_OFFSET,
                                               MINION_CSR_HPMCOUNTER23_OFFSET,
                                               MINION_CSR_HPMCOUNTER24_OFFSET,
                                               MINION_CSR_HPMCOUNTER25_OFFSET,
                                               MINION_CSR_HPMCOUNTER26_OFFSET,
                                               MINION_CSR_HPMCOUNTER27_OFFSET,
                                               MINION_CSR_HPMCOUNTER28_OFFSET,
                                               MINION_CSR_HPMCOUNTER29_OFFSET,
                                               MINION_CSR_HPMCOUNTER30_OFFSET,
                                               MINION_CSR_HPMCOUNTER31_OFFSET,
                                               */
                                               MINION_CSR_SIE_OFFSET, MINION_CSR_STVEC_OFFSET,
                                               MINION_CSR_SSCRATCH_OFFSET, MINION_CSR_SEPC_OFFSET };
