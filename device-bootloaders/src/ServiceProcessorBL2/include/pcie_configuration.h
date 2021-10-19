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
#ifndef __BL2_PCIE__
#define __BL2_PICE__

#include <stdint.h>
#include "dm_event_def.h"
#include "etsoc/isa/esr_defines.h"
#include "etsoc/drivers/pcie/pcie_int.h"
#include "etsoc/isa/io.h"
#include "bl2_sp_pll.h"
#include "bl2_pmic_controller.h"
#include "etsoc/drivers/pcie/pcie_int.h"

/*! \def PCIE_GEN_1
    \brief PCIE gen 1 bit rates(GT/s) definition.
*/
#define PCIE_GEN_1 2

/*! \def PCIE_GEN_2
    \brief PCIE gen 2 bit rates(GT/s) definition.
*/
#define PCIE_GEN_2 5

/*! \def PCIE_GEN_3
    \brief PCIE gen 3 bit rates(GT/s) definition.
*/
#define PCIE_GEN_3 8

/*! \def PCIE_GEN_4
    \brief PCIE gen 4 bit rates(GT/s) definition.
*/
#define PCIE_GEN_4 16

/*! \def PCIE_GEN_5
    \brief PCIE gen 5 bit rates(GT/s) definition.
*/
#define PCIE_GEN_5 32

/*! \fn int32_t pcie_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the PCIe error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/

int32_t pcie_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t pcie_error_control_deinit(void)
    \brief This function cleans up the PCIe error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_error_control_deinit(void);

/*! \fn int32_t pcie_enable_ce_interrupt(void)
    \brief This function enables pcie correctable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_enable_ce_interrupt(void);

/*! \fn int32_t pcie_enable_uce_interrupt(void)
    \brief This function enables pcie uncorrectable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_enable_uce_interrupt(void);

/*! \fn int32_t pcie_disable_ce_interrupt(void)
    \brief This function dis-ables pcie corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_disable_ce_interrupt(void);

/*! \fn int32_t pcie_disable_uce_interrupt(void)
    \brief This function dis-ables pcie un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_disable_uce_interrupt(void);

/*! \fn int32_t pcie_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the PCIe correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t pcie_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t pcie_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t pcie_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t pcie_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t pcie_get_uce_count(uint32_t *uce_count);

void pcie_error_threshold_isr(
    void); //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

/*! \fn int32_t setup_pcie_gen3_link_speed(void)
    \brief Interface to set up PCIE Gen3 Link speed
    \param None
    \returns Status indicating success or negative error
*/
int32_t setup_pcie_gen3_link_speed(void);

/*! \fn int32_t setup_pcie_gen4_link_speed(void)
    \brief Interface to set up PCIE Gen4 Link speed
    \param None
    \returns Status indicating success or negative error
*/
int32_t setup_pcie_gen4_link_speed(void);

/*! \fn int32_t setup_pcie_lane_width_x4(void)
    \brief Interface to set up PCIE lane width(x4)
    \param None
    \returns Status indicating success or negative error
*/
int32_t setup_pcie_lane_width_x4(void);

/*! \fn int32_t setup_pcie_lane_width_x8(void)
    \brief Interface to set up PCIE lane width(x4)
    \param None
    \returns Status indicating success or negative error
*/
int32_t setup_pcie_lane_width_x8(void);

/*! \fn int32_t pcie_retrain_phy(void)
    \brief Interface to start phy retraining
    \param None
    \returns Status indicating success or negative error
*/
int32_t pcie_retrain_phy(void);

/*! \fn int pcie_get_speed(char *pcie_speed)
    \brief Interface to get the PCIE speed
    \param *pcie_speed  Pointer to pcie speed variable
    \returns Status indicating success or negative error
*/
int pcie_get_speed(char *pcie_speed);

/*! \fn int PShire_Initialize(void)
    \brief Interface to initialize PShire
    \param None
    \returns Status indicating success or negative error
*/
int PShire_Initialize(void);

/*! \fn int PCIE_Controller_Initialize(void)
    \brief Interface to initialize PCIE controller
    \param None
    \returns Status indicating success or negative error
*/
int PCIE_Controller_Initialize(void);

/*! \fn int PCIE_Phy_Initialize(void)
    \brief Interface to initialize PCIE phy
    \param None
    \returns Status indicating success or negative error
*/
int PCIE_Phy_Initialize(void);

/*! \fn int PShire_Voltage_Update(uint8_t voltage)
    \brief Interface to set pshire voltage
    \param voltage
    \returns Status indicating success or negative error
*/
int PShire_Voltage_Update(uint8_t voltage);

/*! \fn int Pshire_PLL_Program(uint8_t frequency)
    \brief Interface to program PLL of PShire
    \param frequency
    \returns Status indicating success or negative error
*/
int Pshire_PLL_Program(uint8_t frequency);

/*! \fn int Pshire_NOC_update_routing_table(void)
    \brief Interface to update routing table
    \param None
    \returns Status indicating success or negative error
*/
int Pshire_NOC_update_routing_table(void);

/*! \fn int PCIe_Phy_Firmware_Update (const uint64_t* image)
    \brief Interface to update phy firmware
    \param image pointer to firmware image
    \returns Status indicating success or negative error
*/
int PCIe_Phy_Firmware_Update (const uint64_t* image);

/*! \fn int PCIE_Init_Status(void)
    \brief Interface to find pcie initialization status
    \param None
    \returns Status indicating success or negative error
*/
int PCIE_Init_Status(void);

#endif
