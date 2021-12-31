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
    CW_SHIRE_STATE_FREE = 0,
    CW_SHIRE_STATE_BUSY,
};

/*! \cond
    \brief Start of section which is ignored by Doxygen
*/
typedef uint8_t cw_shire_state_t;

/*! \endcond
    \brief End of section which is ignored by Doxygen
*/

/*! \def CW_INIT_TIMEOUT
    \brief Timeout value for Compute Workers initialization
*/
#define CW_INIT_TIMEOUT 5U

/*! \fn int32_t CW_Init(void)
    \brief Initialize Compute Workers, used by dispatcher
    to initialize compute shires
    \return none
*/
int32_t CW_Init(void);

/*! \fn void CW_Process_CM_SMode_Messages(void)
    \brief CW helper to process messages from CM firmware
    running in S Mode. Used by dispatcher to handle
    exceptions that occur during CM firmware in S Mode.
    \return none
*/
void CW_Process_CM_SMode_Messages(void);

/*! \fn void CW_Update_Shire_State
            (uint64_t shire_mask, cw_shire_state_t shire_state)
    \brief Update shire state associated with compute workers
    \param shire_mask Shire Group to update
    \param shire_state State to update to
    \return Status success or error
*/
void CW_Update_Shire_State(uint64_t shire_mask, cw_shire_state_t shire_state);

/*! \fn int32_t CW_Check_Shires_Available_And_Free(uint64_t shire_mask)
    \brief Check if shire associated with compute workers are free
    \param shire_mask Shire mask to check
    \return Status success or error
*/
int32_t CW_Check_Shires_Available_And_Free(uint64_t shire_mask);

/*! \fn uint64_t CW_Get_Physically_Enabled_Shires(void)
    \brief Get mask for physically available shires of the chip
    obtained from OTP (eFuses).
    \return Physically available shire mask
*/
uint64_t CW_Get_Physically_Enabled_Shires(void);

/*! \fn uint64_t CW_Get_Booted_Shires(void)
    \brief Get the booted available shires.
    \return Booted available shire mask
*/
uint64_t CW_Get_Booted_Shires(void);

/*! \fn int32_t CW_CM_Configure_And_Wait_For_Boot(uint64_t available_shires)
    \brief Configure and reset CW minions for warmboot
    \param shire_mask Shires mask to perform reset and wait for boot
    \return None
*/
int32_t CW_CM_Configure_And_Wait_For_Boot(uint64_t shire_mask);

#endif /* CW_DEFS_H */
