/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_reset.h
    \brief A C header that defines the reset interfaces for minions, memshire 
           and PCIe.
*/
/***********************************************************************/

#ifndef __BL2_RESET_H__
#define __BL2_RESET_H__

/*! \fn void Maxion_Reset_Cold_Assert(void)
    \brief This function asserts Maxion Cold reset
    \param None
    \return none
*/
void Maxion_Reset_Cold_Assert(void);
/*! \fn void Maxion_Reset_Cold_Release(void)
    \brief This function releases Maxion Cold reset
    \param None
    \return none
*/
void Maxion_Reset_Cold_Release(void);

/*! \fn void Maxion_Reset_Warm_Uncore_Assert(void)
    \brief This function asserts Maxion Uncore Warm reset
    \param None
    \return none
*/
void Maxion_Reset_Warm_Uncore_Assert(void);

/*! \fn void Maxion_Reset_Warm_Uncore_Release(void)
    \brief This function releases Maxion Uncore Warm reset
    \param None
    \return none
*/
void Maxion_Reset_Warm_Uncore_Release(void);

/*! \fn void Maxion_Reset_Warm_Core_Release(void)
    \brief This function releases Maxion Core Warm reset
    \param None
    \return none
*/
void Maxion_Reset_Warm_Core_Release(void);

/*! \fn void Maxion_Reset_Warm_Core_Assert(void)
    \brief This function asserts Maxion Core Warm reset
    \param None
    \return none
*/
void Maxion_Reset_Warm_Core_Assert(void);

/*! \fn void Maxion_Reset_PLL_Uncore_Release
    \brief This function releases Maxion Uncore PLL from reset state
    \param None
    \return none
*/
void Maxion_Reset_PLL_Uncore_Release(void);

/*! \fn void Maxion_Reset_PLL_Core_Release
    \brief This function releases Maxion Core PLL from reset state
    \param None
    \return none

*/
void Maxion_Reset_PLL_Core_Release(void);

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

/*! \fn uint32_t get_hpdpll_strap_value(void)
    \brief This function reads HPDPLL starp value
    \param None 
    \return HPDPLL strap value
*/
uint8_t get_hpdpll_strap_value(void);

/*! \fn uint32_t get_lvdpll_strap_value(void)
    \brief This function reads LVDPLL starp value
    \param None 
    \return LVDPLL strap value
*/
uint8_t get_lvdpll_strap_value(void);

#endif
