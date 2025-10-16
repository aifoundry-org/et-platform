/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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

/**
 * @enum PLL_ID_t
 * @brief Enum defining PLL IDs
 */
typedef enum PLL_ID_e
{
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
typedef enum SP_PLL_STATE_e
{
    SP_PLL_STATE_INVALID,
    SP_PLL_STATE_OFF,
    SP_PLL_STATE_50_PER_CENT,
    SP_PLL_STATE_75_PER_CENT,
    SP_PLL_STATE_100_PER_CENT
} SP_PLL_STATE_t;

/**
 * @enum HPDPLL_LDO_UPDATE_t
 * @brief Enum defining HPDPLL LDO update type
 */
typedef enum HPDPLL_LDO_UPDATE_e
{
    HPDPLL_LDO_NO_UPDATE,
    HPDPLL_LDO_ENABLE,
    HPDPLL_LDO_DISABLE,
    HPDPLL_LDO_BYPASS,
    HPDPLL_LDO_KICK
} HPDPLL_LDO_UPDATE_t;

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

/*! \fn int configure_sp_pll_1(uint8_t mode)
    \brief This function configures service processor PLL 1
    \param None
    \return The function call status, pass/fail.
*/
int configure_sp_pll_1(const uint8_t mode);

/*! \fn int configure_sp_pll_2(uint8_t mode, HPDPLL_LDO_UPDATE_t ldo_update)
    \brief This function configures service processor PLL 2
    \param None 
    \return The function call status, pass/fail.
*/
int configure_sp_pll_2(const uint8_t mode, HPDPLL_LDO_UPDATE_t ldo_update);

/*! \fn int configure_sp_pll_4(uint8_t mode, HPDPLL_LDO_UPDATE_t ldo_update)
    \brief This function configures service processor PLL 4
    \param None 
    \return The function call status, pass/fail.
*/
int configure_sp_pll_4(const uint8_t mode, HPDPLL_LDO_UPDATE_t ldo_update);

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

/*! \fn int pll_init(void)
    \brief This function initializes PLL sub system
    \param none
    \return The function call status, pass/fail.
*/
int pll_init(void);

/*! \fn void pll_lock_loss_isr(void)
    \brief This is interrupt handler for CRU interrupt
    \param None 
    \return None
*/
void pll_lock_loss_isr(void);

/*! \fn void enable_spio_pll_lock_loss_interrupt(void)
    \brief Setup and enable SPIO PLLs lock loss interrupt
    \param None 
    \return None
*/
void enable_spio_pll_lock_loss_interrupt(void);

/*! \fn void print_spio_lock_loss_counters(void)
    \brief Prints lock counters handled by ISR
    \param None 
    \return None
*/
void print_spio_lock_loss_counters(void);

/*! \fn void spio_pll_clear_lock_monitor(PLL_ID_t pll)
    \brief This function clears lock monitor of SPIO PLL
    \param pll PLL ID
    \return none
*/
void spio_pll_clear_lock_monitor(PLL_ID_t pll);

/*! \fn uint32_t spio_pll_get_lock_monitor(PLL_ID_t pll)
    \brief This function reads lock monitor of SPIO PLL
    \param pll PLL ID
    \return PLL lock monitor value
*/
uint32_t spio_pll_get_lock_monitor(PLL_ID_t pll);

/*! \fn void print_spio_lock_loss_monitors(void)
    \brief This function prints lock monitors of SPIO PLLs
    \param none
    \return none
*/
void print_spio_lock_loss_monitors(void);

/*! \fn void clear_spio_lock_loss_monitors(void)
    \brief This function clears lock monitors of SPIO PLLs
    \param none
    \return none
*/
void clear_spio_lock_loss_monitors(void);

/*! \fn SP_PLL_STATE_t get_pll_requested_percent(void)
    \brief This function returns PLL state (100%, 75%, 50%, PLL OFF)
    \param None 
    \return PLL state
*/
SP_PLL_STATE_t get_pll_requested_percent(void);

#endif
