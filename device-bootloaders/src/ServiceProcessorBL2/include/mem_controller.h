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
/*! \file mem_controller.h
    \brief A C header that defines the DDR memory subsystem's
    public interfaces. These interfaces provide services using which
    DDR can be initialized and report error events.
*/
/***********************************************************************/

#ifndef __MEM_CONTROLLER_H__
#define __MEM_CONTROLLER_H__

#include <stdint.h>

#include "hwinc/ms_reg_def.h"
#include "hwinc/ddrc_reg_def.h"
#include "dm_event_def.h"
#include "bl2_pmic_controller.h"
#include "bl2_reset.h"
#include "bl_error_code.h"

/*! \def NUMBER_OF_MEMSHIRE
*/
#define NUMBER_OF_MEMSHIRE     8

/**
 * @enum ddr_frequency_t
 * @brief Frequency enum defines for DDR memory
 */
typedef enum {
    DDR_FREQUENCY_800MHZ,
    DDR_FREQUENCY_933MHZ,
    DDR_FREQUENCY_1066MHZ
} ddr_frequency_t;

/**
 * @enum ddr_capacity_t
 * @brief Capacity enum defines for DDR memory
 */
typedef enum {
    DDR_CAPACITY_4GB,
    DDR_CAPACITY_8GB,
    DDR_CAPACITY_16GB,
    DDR_CAPACITY_32GB,
} ddr_capacity_t;

/**
 * @struct DDR_MODE
 * @brief Structure for passing into DDR driver
 */
typedef struct {
    ddr_frequency_t  frequency;
    ddr_capacity_t   capacity;
    bool             ecc;
    bool             training;
    bool             sim_only;
} DDR_MODE;

/*!
 * @struct struct ddr_event_control_block
 * @brief DDR driver error mgmt control block
 */
struct ddr_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \fn int configure_memshire_plls(DDR_MODE *ddr_mode)
    \brief This function initializes every memshire present in system. It internally
           calls ddr_init with memshire id
    \param ddr_mode point to a DDR_MODE structure, which contains the parameters for DDR init
    \return Zero indicating success or non-zero for error
*/
int configure_memshire_plls(const DDR_MODE *ddr_mode);

/*! \fn int ddr_config(DDR_MODE *ddr_mode)
    \brief This function initializes every memshire present in system. It internally
           calls ddr_init with memshire id
    \param ddr_mode point to a DDR_MODE structure, which contains the parameters for DDR init
    \return Zero indicating success or non-zero for error
*/
int ddr_config(DDR_MODE *ddr_mode);

/*! \fn int32_t ddr_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the ddr error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/
int32_t ddr_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t ddr_error_control_deinit(void)
    \brief This function cleans up the ddr error control subsystem.
    \param none
    \return Status indicating success or negative error
*/
int32_t ddr_error_control_deinit(void);

/*! \fn int32_t ddr_enable_uce_interrupt(void)
    \brief This function enables ddr uncorrectable error interrupts.
    \param none
    \return Status indicating success or negative error
*/
int32_t ddr_enable_uce_interrupt(void);

/*! \fn int32_t ddr_disable_ce_interrupt(void)
    \brief This function dis-ables ddr corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/
int32_t ddr_disable_ce_interrupt(void);

/*! \fn int32_t ddr_disable_uce_interrupt(void)
    \brief This function dis-ables ddr un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/
int32_t ddr_disable_uce_interrupt(void);

/*! \fn int32_t ddr_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the ddr correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/
int32_t ddr_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t ddr_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/
int32_t ddr_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t ddr_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/
int32_t ddr_get_uce_count(uint32_t *uce_count);

/*! \fn void ddr_error_threshold_isr(void)
    \brief This is error threshold isr
    \param None
*/

void ddr_error_threshold_isr(void);  //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

/*! \fn int ddr_get_memory_details(char *mem_detail)
    \brief This function get Memory vendor and part details
    \param mem_detail pointer to variable to hold Memory detail value
    \return Status indicating success or negative error
*/
int ddr_get_memory_details(char *mem_detail);

/*! \fn int ddr_get_memory_type(char *mem_type)
    \brief This function get Memory type details
    \param mem_type pointer to variable to hold memory type value
    \return Status indicating success or negative error
*/
int ddr_get_memory_type(char *mem_type);

/*! \fn int32_t configure_memshire(void)
    \brief This function configures the MemShire and DDR Controllers
    \param NA
    \return Status indicating success or negative error
*/
int32_t configure_memshire(void);

/*! \fn uint32_t get_memshire_frequency(void)
    \brief This function returns Memshire frequency
    \param NA
    \return memshire frequency
*/
uint32_t get_memshire_frequency(void);

/*! \fn uint32_t get_ddr_frequency(void)
    \brief This function returns DDR frequency
    \param NA
    \return DDR frequency
*/
uint32_t get_ddr_frequency(void);

#endif

