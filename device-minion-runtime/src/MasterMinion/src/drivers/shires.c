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
*       This file implements the Dispatcher. The function of the 
*       dispatcher is to field all interrupts and 
*
*   FUNCTIONS
*
*       Dispatcher_Launch
*
***********************************************************************/
#include "drivers/shires.h"
#include "services/log1.h"

/*! \var shire_status1_t Shire_Status[]
    \brief Global variable that maintains the shire status
    \warning Not thread safe!
*/
static shire_status1_t Gbl_Shire_Status[NUM_SHIRES] = { 0 };

/*! \var uint64_t Gbl_Functional_Shires
    \brief Global active/functional shire state
    \warning Not thread safe!
*/
static uint64_t Gbl_Functional_Shires = 0;

/*! \var uint64_t Gbl_Booted_Shires
    \brief Global booted shire state
    \warning Not thread safe!
*/
static uint64_t Gbl_Booted_Shires = 0;

/************************************************************************
*
*   FUNCTION
*
*       Shire_Get_Active
*  
*   DESCRIPTION
*
*       Get functional/active shires
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    Get functional/active shires
*
***********************************************************************/
uint64_t Shire_Get_Active(void)
{
    return Gbl_Functional_Shires;
}

/************************************************************************
*
*   FUNCTION
*
*       Shire_Set_Active
*  
*   DESCRIPTION
*
*       Set functional/active shires
*
*   INPUTS
*
*       uint64_t   Set functional shires 
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Shire_Set_Active(uint64_t mask)
{
    Gbl_Functional_Shires = mask;
}

/************************************************************************
*
*   FUNCTION
*
*       Shire_Update_State
*  
*   DESCRIPTION
*
*       Set active shires
*
*   INPUTS
*
*       uint64_t        Shire mask
*       shire_state_t   Shire state
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Shire_Update_State(uint64_t shire, shire_state_t shire_state)
{
    const shire_state_t current_state = 
        Gbl_Shire_Status[shire].shire_state;

    /*  TODO FIXME this is hokey, clean up state handling. */
    if (shire_state == SHIRE_STATE_COMPLETE) 
    {
        Gbl_Shire_Status[shire].shire_state = SHIRE_STATE_READY;
    } 
    else 
    {
        if (current_state != SHIRE_STATE_ERROR) 
        {
            Gbl_Shire_Status[shire].shire_state = shire_state;
        
            /* Update mask of booted shires 
            (coming from SHIRE_STATE_UNKNOWN) */
            Gbl_Booted_Shires |= 1ULL << shire;
        } 
        else 
        {
            /* The only legal transition from ERROR state is to READY state */
            if (shire_state == SHIRE_STATE_READY) 
            {
                Gbl_Shire_Status[shire].shire_state = shire_state;
            } 
            else 
            {
                Log_Write(LOG_LEVEL_ERROR, 
                    "Error illegal shire %d state transition from error\r\n",
                    shire);
            }
        }
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       Shire_Check_All_Are_Booted
*  
*   DESCRIPTION
*
*       Check all shires specified by mask are booted
*
*   INPUTS
*
*       uint64_t   Set Active 
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
bool Shire_Check_All_Are_Booted(uint64_t shire_mask)
{
    return ((Gbl_Booted_Shires & shire_mask) == shire_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       Shire_Check_All_Are_Ready
*  
*   DESCRIPTION
*
*       Check all shires specified by mask are booted
*
*   INPUTS
*
*       uint64_t   Set Active 
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
bool Shire_Check_All_Are_Ready(uint64_t shire_mask)
{
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) 
    {
        if (shire_mask & (1ULL << shire)) 
        {
            if (Gbl_Shire_Status[shire].shire_state != SHIRE_STATE_READY) 
            {
                return false;
            }
        }
    }

    return true;   
}