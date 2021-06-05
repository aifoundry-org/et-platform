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
/*! \file minion_configuration.h
    \brief A C header that defines the minion configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __MINION_CONFIGURATION_H__
#define __MINION_CONFIGURATION_H__

#include <stdint.h>
#include "bl2_sp_pll.h"
#include "bl2_reset.h"
#include "sp_otp.h"
#include "bl2_pmic_controller.h"
#include "bl2_firmware_loader.h"
#include "bl2_certificates.h"
#include "config/sp_bl2_return_code.h"
#include "error.h"


/*! \fn int Enable_Minion_Neighborhoods(uint64_t shire_mask)
    \brief This function enables minion neighborhoods PLL and DLLs for a given
           set of shires
    \param shire_mask shire to be configured
    \return The function call status, pass/fail.
*/
int Enable_Minion_Neighborhoods(uint64_t shire_mask);

/*! \fn int Enable_Master_Shire_Threads(uint8_t mm_id)
    \brief This function enables mastershire threads
    \param mm_id master minion shire id 
    \return The function call status, pass/fail.
*/
int Enable_Master_Shire_Threads(uint8_t mm_id);

/*! \fn int Enable_Compute_Minion(uint64_t minion_shires_mask)
    \brief This function enables compute minion threads
    \param  minion_shires_mask Shire Mask to enable
    \return The function call status, pass/fail.
*/
int Enable_Compute_Minion(uint64_t minion_shires_mask);

/*! \fn uint64_t Get_Active_Compute_Minion_Mask(void)
    \brief This function gets the active compute shire mask
           by reading the value from SP OTP 
    \param  minion_shires_mask Shire Mask to enable
    \return The function call status, pass/fail.
*/
uint64_t Get_Active_Compute_Minion_Mask(void);


/*! \fn int Load_Autheticate_Minion_Firmware(void)
    \brief This function loads and authenticates the 
           Minions firmware 
    \param  minion_shires_mask Shire Mask to enable
    \return The function call status, pass/fail.
*/
int Load_Autheticate_Minion_Firmware(void);

#endif
