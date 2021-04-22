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
************************************************************************/

/***********************************************************************/
/*! \file cw.h
    \brief A C header that defines the CW Worker's public interfaces.
*/
/***********************************************************************/
#ifndef CW_DEFS_H
#define CW_DEFS_H

/*! \enum cw_shire_state_t
    \brief Enum that provides the status of a shire
*/
enum cw_shire_state_t {
    CW_SHIRE_STATE_UNKNOWN = 0,
    CW_SHIRE_STATE_READY,
    CW_SHIRE_STATE_RUNNING,
    CW_SHIRE_STATE_ERROR,
    CW_SHIRE_STATE_RESERVED
};

/*! \cond
    \brief Start of section which is ignored by Doxygen
*/
typedef uint8_t cw_shire_state_t;

/*! \endcond
    \brief End of section which is ignored by Doxygen
*/

/*! \def CW_ERROR_GENERAL
    \brief Compute Worker - General error
*/
#define CW_ERROR_GENERAL     -1

/*! \def CW_SHIRE_UNAVAILABLE
    \brief Compute Worker - Shires unavailable
*/
#define CW_SHIRE_UNAVAILABLE -2

/*! \def CW_SHIRES_NOT_READY
    \brief Compute Worker - Shires not ready
*/
#define CW_SHIRES_NOT_READY   -3

/*! \fn int8_t CW_Init(void)
    \brief Initialize Compute Workers, used by dispatcher
    to initialize compute shires
    \return none
*/
int8_t CW_Init(void);

/*! \fn void CW_Process_CM_SMode_Messages(void)
    \brief CW helper to process messages from CM firmware
    running in S Mode. Used by dispatcher to handle
    exceptions that occur during CM firmware in S Mode.
    \param none
    \return none
*/
void CW_Process_CM_SMode_Messages(void);


/*! \fn int8_t CW_Update_Shire_State
            (uint64_t shire, cw_shire_state_t shire_state)
    \brief Update shire state associated with compute workers
    \param shire Shire to update
    \param shire_state State to update to
    \return Status success or error
*/
int8_t CW_Update_Shire_State(uint64_t shire, cw_shire_state_t shire_state);

/*! \fn int8_t CW_Check_Shires_Available_And_Ready(uint64_t shire_mask)
    \brief Check if shire associated with compute workers is available
    and ready
    \param shire_mask Shire mask to check
    \return Status success or error
*/
int8_t CW_Check_Shires_Available_And_Ready(uint64_t shire_mask);

/*! \fn uint64_t CW_Get_Physically_Enabled_Shires(void)
    \brief Get mask for physically available shires of the chip
    obtained from OTP (eFuses).
    \return Physically available shire mask
*/
uint64_t CW_Get_Physically_Enabled_Shires(void);

#endif /* CW_DEFS_H */
