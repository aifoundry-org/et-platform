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
/*! \file minion_run_control.h
    \brief A C header that defines the Minion State Inspection public
    interfaces
*/
/***********************************************************************/
#ifndef MINION_STATE_INSPECTION_H
#define MINION_STATE_INSPECTION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "esr.h"
#include "etsoc_hal/inc/hal_device.h"
#include "etsoc_hal/inc/etsoc_neigh_esr.h"
#include "etsoc_hal/inc/spio_misc_esr.h"
#include "etsoc_hal/inc/etsoc_shire_other_esr.h"
#include "etsoc_hal/inc/minion_csr.h"
#include "debug_accessor.h"
#include "debug_instruction_sequence.h"
#include <et-trace/layout.h>

/*! \fn uint64_t *Read_All_GPR(uint64_t hart_id)
    \brief Read all the GPRs of the Hart specified by hart_id
    \param hart_id Hart ID for which the GPRs have to be read 
    \returns uint64_t* Pointer to GPRs in dev_context_registers_t
*/
uint64_t *Read_All_GPR(uint64_t hart_id);

/*! \fn uint64_t Read_GPR(uint64_t hart_id, uint32_t reg)
    \brief Read GPR for the Hart specified by the hart_id
    \param hart_id Hart ID for which the GPR has to be read
    \param reg  GPR Index
    \returns uint64_t
*/
uint64_t Read_GPR(uint64_t hart_id, uint32_t reg);

/*! \fn void Write_GPR(uint64_t hart_id, uint32_t reg, uint64_t data)
    \brief Write GPR for the Hart specified by the hart_id
    \param hart_id Hart ID for which the GPR has to be written
    \param reg  GPR Index
    \param data value to be written to GPR
    \returns None
*/
void Write_GPR(uint64_t hart_id, uint32_t reg, uint64_t data);

/*! \fn uint64_t Read_CSR(uint64_t hart_id, uint32_t csr)
    \brief Read the CSR of the Hart specified by hart_id
    \param hart_id  Hart ID for which the CSR has to be read
    \param csr CSR Name
    \returns uint64_t
*/
uint64_t Read_CSR(uint64_t hart_id, uint32_t csr);

/*! \fn void Write_CSR(uint64_t hart_id, uint32_t csr, uint64_t data)
    \brief Write to CSR of the Hart specified by hart_id
    \param hart_id Hart ID for which the CSR has to be written
    \param csr CSR Name
    \param data Value to be written into CSR
    \returns None
*/
void Write_CSR(uint64_t hart_id, uint32_t csr, uint64_t data);


/*! \fn void VPU_RF_Init(uint64_t hart_id)
    \brief Initialize the VPU Register file of Hart specified by hart_id
    \param hart_id Hart ID for which has to be written
    \returns None
*/
void VPU_RF_Init(uint64_t hart_id);

#endif /* MINION_STATE_INSPECTION_H */
