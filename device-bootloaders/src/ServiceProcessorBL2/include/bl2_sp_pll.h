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
/*! \file bl2_sp_pll.h
    \brief A C header that defines the PLL configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __BL2_SP_PLL_H__
#define __BL2_SP_PLL_H__

#include <stdint.h>

/*! \def ERROR_SP_PLL_CONFIG_DATA_NOT_FOUND
    \brief PLL error define for data not found
*/
#define ERROR_SP_PLL_CONFIG_DATA_NOT_FOUND -1000

/*! \def ERROR_SP_PLL_PLL_LOCK_TIMEOUT
    \brief sp pll lock timeout
*/
#define ERROR_SP_PLL_PLL_LOCK_TIMEOUT      -1001

/*! \def ERROR_SP_PLL_INVALID_PLL_ID
    \brief invalid PLL ID define
*/
#define ERROR_SP_PLL_INVALID_PLL_ID        -1002

/**
 * @enum PLL_ID_t
 * @brief Enum defining PLL IDs
 */
typedef enum PLL_ID_e {
    PLL_ID_INVALID,
    PLL_ID_SP_PLL_0,
    PLL_ID_SP_PLL_1,
    PLL_ID_SP_PLL_2,
    PLL_ID_SP_PLL_3,
    PLL_ID_SP_PLL_4,
    PLL_ID_MAXION_CORE,
    PLL_ID_MAXION_UNCORE,
    PLL_ID_PSHIRE
} PLL_ID_t;

/**
 * @enum SP_PLL_STATE_t
 * @brief Enum defining PLL states
 */
typedef enum SP_PLL_STATE_e {
    SP_PLL_STATE_INVALID,
    SP_PLL_STATE_OFF,
    SP_PLL_STATE_50_PER_CENT,
    SP_PLL_STATE_75_PER_CENT,
    SP_PLL_STATE_100_PER_CENT
} SP_PLL_STATE_t;

/*! \fn uint32_t get_input_clock_index(void)
    \brief This function returns input clock index
    \param None
    \return clock index
*/
uint32_t get_input_clock_index(void);

/*! \fn int configure_sp_pll_0(uint8_t mode)
    \brief This function configures service processor PLL 0
    \param None 
    \return The function call status, pass/fail.
*/
int configure_sp_pll_0(const uint8_t mode);

/*! \fn int configure_sp_pll_2(uint8_t mode)
    \brief This function configures service processor PLL 2
    \param None 
    \return The function call status, pass/fail.
*/
int configure_sp_pll_2(const uint8_t mode);

/*! \fn int configure_sp_pll_4(uint8_t mode)
    \brief This function configures service processor PLL 4
    \param None 
    \return The function call status, pass/fail.
*/
int configure_sp_pll_4(const uint8_t mode);

/*! \fn int configure_pshire_pll(const uint8_t mode)
    \brief This function configures pshire PLL
    \param mode mode of PLL config 
    \return The function call status, pass/fail.
*/
int configure_pshire_pll(const uint8_t mode);

/*! \fn int configure_minion_plls(void)
    \brief This function configures minion PLLs
    \param None 
    \return The function call status, pass/fail.
*/
int configure_minion_plls(const uint8_t mode);

/*! \fn int configure_maxion_pll_core(const uint8_t mode)
    \brief This function configures Maxion core PLL
    \param mode mode of PLL config
    \return The function call status, pass/fail.
*/
int configure_maxion_pll_core(const uint8_t mode);

/*! \fn int configure_maxion_pll_uncore(const uint8_t mode)
    \brief This function configures Maxion uncore PLL
    \param mode mode of PLL config
    \return The function call status, pass/fail.
*/
int configure_maxion_pll_uncore(const uint8_t mode);

/*! \fn int get_pll_frequency(PLL_ID_t pll_id, uint32_t *frequency)
    \brief This function returns current configured PLL frequency of 
        specific PLL ID
    \param pll_id PLL ID of device
    \param frequency returned frequency value
    \return The function call status, pass/fail.
*/
int get_pll_frequency(PLL_ID_t pll_id, uint32_t *frequency);

/*! \fn int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency,
             uint32_t pcie_pll_0_frequency)
    \brief This function initializes PLL sub system
    \param sp_pll_0_frequency PLL0 default frequency 
    \param sp_pll_1_frequency PLL1 default frequency 
    \param pcie_pll_0_frequency PCIe PLL default frequency 
    \return The function call status, pass/fail.
*/
int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency,
             uint32_t pcie_pll_0_frequency);

#endif
