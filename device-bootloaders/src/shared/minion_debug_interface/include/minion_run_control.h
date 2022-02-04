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
    \brief A C header that defines the Minion Run Control public
    interfaces
*/
/***********************************************************************/
#ifndef MINION_RUN_CONTROL_H
#define MINION_RUN_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "minion_state_inspection.h"
#include "debug_accessor.h"
#include <esperanto/device-apis/management-api/device_mgmt_api_spec.h>
#include <esperanto/device-apis/management-api/device_mgmt_api_rpc_types.h>

/* Hart Selection */
/*! \fn void Select_Harts(uint8_t shire_mask, uint8_t thread_mask)
    \brief Select the Harts specified by shire_mask/thread_mask
    \param shire_id Active shire ID
    \param thread_mask Hart Thread mask
    \returns None
*/
void Select_Harts(uint8_t shire_id, uint8_t thread_mask);

/*! \fn void Unselect_Harts(uint8_t shire_mask, uint8_t thread_mask)
    \brief Unselect the Hart specified by shire_mask/thread_mask
    \param shire_id  Active shire ID
    \param thread_mask  Hart Thread mask
    \returns None
*/
void Unselect_Harts(uint8_t shire_id, uint8_t thread_mask);

/* Hart Control */
/*! \fn bool Halt_Harts(void)
    \brief This function is used to halt the running Harts
    \param None 
    \returns None
*/
bool Halt_Harts(void);

/*! \fn bool Resume_Harts(void)
    \brief This function is used to resume the halted Harts
    \param None 
    \returns bool
*/
bool Resume_Harts(void);

/*! \fn bool Check_Running(void)
    \brief This function returns True if selected harts are running. It DOESN'T implement the workaround for
           Errata 1.18 so minion 0 on each neigh needs to be selected and enabled when this function is called
    \param None 
    \returns bool
*/
bool Check_Running(void);

/*! \fn bool Check_Halted(void)
    \brief This function returns True if selected harts are halted
    \param None 
    \returns bool
*/
bool Check_Halted(void);

/* Breakpoint Control */
/*! \fn  void Set_PC_Breakpoint(uint64_t hart_id, uint64_t pc, priv_mask_t mode)
    \brief  Sets breakpoint on a PC. Other breapoints (on data or on PC) in the hart
            are overriten
    \param hart_id Hart ID for which breakpoint has to be set
    \param pc PC where the Hart will stop
    \param mode Hart Mode (MMODE, SMODE, UMODE)  
    \returns None
*/
void Set_PC_Breakpoint(uint64_t hart_id, uint64_t pc, priv_mask_e mode);

/*! \fn  void Unset_PC_Breakpoint(uint64_t hart_id)
    \brief Removes breakpoint on a PC. 
    \param hart_id Hart ID for which breakpoint has to be unset
    \returns None
*/
void Unset_PC_Breakpoint(uint64_t hart_id);

/*! \fn  void Set_PC(uint64_t hart_id, uint64_t pc)
    \brief Sets the PC of Hart
    \param hart_id Hart ID for which PC has to be set
    \param pc PC value to be set
    \returns None
*/
void Set_PC(uint64_t hart_id, uint64_t pc);

#endif /* MINION_RUN_CONTROL_H */
