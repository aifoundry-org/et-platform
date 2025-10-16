/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
#include "etsoc_hal/inc/minion_csr.h"
#include "hwinc/hal_device.h"
#include "hwinc/etsoc_neigh_esr.h"
#include "hwinc/sp_misc.h"
#include "hwinc/etsoc_shire_other_esr.h"

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

/*! \fn void Minion_Memory_Read(uint64_t hart_id, uint64_t address)
    \brief Read value at the address of the Hart specified by hart_id
    \param hart_id Hart ID for which the memory has to be read
    \param address Address to be read
    \returns uint64_t
*/
uint64_t Minion_Memory_Read(uint64_t hart_id, uint64_t address);

/*! \fn void Minion_Local_Atomic_Read(uint64_t hart_id, uint64_t address)
    \brief Read value of local variable's memory address atomically
    \param hart_id Hart ID for which the memory has to be read
    \param address Address to be read
    \returns uint64_t
*/
uint64_t Minion_Local_Atomic_Read(uint64_t hart_id, uint64_t address);

/*! \fn void Minion_Global_Atomic_Read(uint64_t hart_id, uint64_t address)
    \brief Read value of global variable's memory address atomically
    \param hart_id Hart ID for which the memory has to be read
    \param address Address to be read
    \returns uint64_t
*/
uint64_t Minion_Global_Atomic_Read(uint64_t hart_id, uint64_t address);

#endif /* MINION_STATE_INSPECTION_H */
