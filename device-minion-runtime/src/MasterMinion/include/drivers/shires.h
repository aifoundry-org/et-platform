/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for public interfaces to manage
*       shires
*
***********************************************************************/
#ifndef SHIRES_DEFS_H
#define SHIRES_DEFS_H

#include "common_defs.h"
#include "workers/kw.h"
#include "shire.h"

/*! \struct shire_status_t
    \brief Shire status data structure.
*/
typedef struct {
    shire_state_t shire_state;
    kernel_id_t kernel_id;
} shire_status1_t;

/*! \fn void Shire_Set_Active(uint64_t mask)
    \brief Get active/functional shires
    \param None
*/
void Shire_Set_Active(uint64_t mask);

/*! \fn void Shire_Get_Active(void)
    \brief Set active/functional shires
    \param None
*/
uint64_t Shire_Get_Active(void);

/*! \fn void Shire_Update_State(uint64_t shire, shire_state_t shire_state)
    \brief Set active/functional shires
    \param Shire to update
    \param Shire state to update to
*/
void Shire_Update_State(uint64_t shire, shire_state_t shire_state);

/*! \fn void Shire_Check_All_Are_Booted(void)
    \brief Check if all shires are booted
    \param [in] shire mask
*/
bool Shire_Check_All_Are_Booted(uint64_t shire_mask);

/*! \fn void Shire_Check_All_Are_Ready(uint64_t shire_mask)
    \brief Check if all shires are in ready state
    \param [in] shire mask
*/
bool Shire_Check_All_Are_Ready(uint64_t shire_mask);

#endif /* SHIRES_DEFS_H */