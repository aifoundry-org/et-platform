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
#ifndef __BL2_CACHE_CONTROL__
#define __BL2_CACHE_CONTROL__

#include <stdint.h>
#include "dm_event_def.h"
#include "bl_error_code.h"

/*!
 * @struct struct sram_event_control_block
 * @brief SRAM driver error mgmt control block
 */
struct sram_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \def SC_BANK_BROADCAST
    \brief Shire cache bank broadcast ID
*/
#define SC_BANK_BROADCAST 0xFu

/*! \def SC_BANK_SIZE
    \brief Shire cache bank size (1MB)
*/
#define SC_BANK_SIZE 0x400u

int32_t sram_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t sram_error_control_deinit(void)
    \brief This function cleans up the sram error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_error_control_deinit(void);

/*! \fn int32_t sram_enable_uce_interrupt(void)
    \brief This function enables sram uncorrectable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_enable_uce_interrupt(void);

/*! \fn int32_t sram_disable_ce_interrupt(void)
    \brief This function dis-ables sram corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_disable_ce_interrupt(void);

/*! \fn int32_t sram_disable_uce_interrupt(void)
    \brief This function dis-ables sram un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_disable_uce_interrupt(void);

/*! \fn int32_t sram_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the sram correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t sram_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t sram_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t sram_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t sram_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t sram_get_uce_count(uint32_t *uce_count);

void sram_error_threshold_isr(void);  //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

/*! \fn int cache_scp_l2_l3_size_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size,
*                                    uint64_t shire_mask)
    \brief This function sets cache configuration.
*          Configuration is per cache bank (1MB), and all banks are configured the same.
*          Following constraint must be followed:
*             - (scp_size + l2_size + l3_size) <= 0x400
*             - scp_size, l2_size and l3_size must be multiple of 2
*             - if l2_size !=0 then l2_size >= 0x40
    \param scp_size scratchpad size
    \param l2_size l2 cache size
    \param l3_size l3 cache size
    \param shire_mask shire_mask
    \return Status indicating success or negative error
*/
int cache_scp_l2_l3_size_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size,
                                    uint64_t shire_mask);

#endif
