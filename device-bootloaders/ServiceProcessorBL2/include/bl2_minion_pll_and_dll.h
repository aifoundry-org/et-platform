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
/*! \file bl2_minion_pll_and_dll.h
    \brief A C header that defines the minion PLL configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __MINION_PLL_AND_DLL_H__
#define __MINION_PLL_AND_DLL_H__

#include <stdint.h>

/*! \fn int configure_minion_plls_and_dlls(uint64_t shire_mask)
    \brief This function configures minion PLL and DLLs for a given
           set of shires
    \param shire_mask shire to be configured
    \return The function call status, pass/fail.
*/
int configure_minion_plls_and_dlls(uint64_t shire_mask);

/*! \fn int enable_minion_neighborhoods(uint64_t shire_mask)
    \brief This function enables minion neighborhoods PLL and DLLs for a given
           set of shires
    \param shire_mask shire to be configured
    \return The function call status, pass/fail.
*/
int enable_minion_neighborhoods(uint64_t shire_mask);

/*! \fn int enable_master_shire_threads(uint8_t mm_id)
    \brief This function enables mastershire threads
    \param mm_id master minion shire id 
    \return The function call status, pass/fail.
*/
int enable_master_shire_threads(uint8_t mm_id);

#endif
