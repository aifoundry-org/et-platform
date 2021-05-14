/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************/
/*! \file bl2_reset.h
    \brief A C header that defines the reset interfaces for minions, memshire 
           and PCIe.
*/
/***********************************************************************/

#ifndef __BL2_RESET_H__
#define __BL2_RESET_H__

/*! \fn int release_memshire_from_reset(void)
    \brief This function releases minionshore from reset state
    \param None 
    \return Status indicating success or negative error
*/
int release_memshire_from_reset(void);

/*! \fn int release_minions_from_cold_reset(void)
    \brief This function releases minionshore from cold reset state
    \param None 
    \return Status indicating success or negative error
*/
int release_minions_from_cold_reset(void);

/*! \fn int release_minions_from_warm_reset(void)
    \brief This function releases minionshore from warm reset state
    \param None 
    \return Status indicating success or negative error
*/
int release_minions_from_warm_reset(void);

/*! \fn void release_etsoc_reset(void)
    \brief This function resets etsoc core
    \param None 
    \return none
*/
void release_etsoc_reset(void);

/*! \fn void pcie_reset_flr(void)
    \brief This function resets PCIe
    \param None 
    \return none
*/
void pcie_reset_flr(void);

/*! \fn void pcie_reset_warm(void)
    \brief This function resets PCIe 
    \param None 
    \return none
*/
void pcie_reset_warm(void);

#endif
