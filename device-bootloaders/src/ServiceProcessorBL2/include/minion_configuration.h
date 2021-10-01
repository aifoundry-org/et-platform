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
#include "bl_error_code.h"
#include "transports/sp_mm_iface/sp_mm_comms_spec.h"
#include "mm_iface.h"
#include "dm.h"
#include "sp_host_iface.h"
#include "dm_event_def.h"

/*!
 * @enum minion_error_type
 * @brief Enum defining event/error type
 */
enum minion_error_type {
    CM_USER_KERNEL_ERROR = 0,
    CM_RUNTIME_ERROR,
    MM_DISPATCHER_ERROR,
    MM_SQW_ERROR,
    MM_DMAW_ERROR,
    MM_KW_ERROR,
    MM_RUNTIME_HANG_ERROR,
    MM_UNDEFINED_ERROR
};

/*! \def MINION_HANG_ERROR_THRESHOLD
    \brief Minion hang errors threshold
*/
#define MINION_HANG_ERROR_THRESHOLD   1

/*! \def MINION_EXCEPT_ERROR_THRESHOLD
    \brief Minion exception errors threshold
*/
#define MINION_EXCEPT_ERROR_THRESHOLD 1

/*! \def MM_HEARTBEAT_TIMEOUT_MSEC
    \brief MM  heartbeat timeout period
*/
#define MM_HEARTBEAT_TIMEOUT_MSEC    10000

/*! \fn int Minion_Shire_Update_Voltage( uint8_t voltage)
    \brief This function provide support to update the Minion
           Shire Power Rails
    \param voltage value of the Voltage to updated to
    \return The function call status, pass/fail.
*/
int Minion_Shire_Update_Voltage( uint8_t voltage);


/*! \fn Minion_Get_Voltage_Given_Freq(int32_t target_frequency)
    \brief This function returns a voltage operating value given
            a freq value
    \param Target Minion Frequency
    \return Target Minion Voltage
*/
int Minion_Get_Voltage_Given_Freq(int32_t target_frequency);

/*! \fn int Minion_Program_Step_Clock_PLL(uint8_t mode)
    \brief This function provide support to program the
           Minion Shire Step Clock which is coming from
           IO Shire HDPLL 4
    \param value of the freq(in mode) to updated to
    \return The function call status, pass/fail.
*/
int Minion_Program_Step_Clock_PLL(uint8_t mode);

/*! \fn uint8_t pll_freq_to_mode(int32_t freq)
    \brief This function returns the mode for a given
           frequency
    \param frequency
    \return corresponding mode
*/
uint8_t pll_freq_to_mode(int32_t freq);

/*! \fn int Minion_Enable_Shire_Cache_and_Neighborhoods(uint64_t shire_mask)
    \brief This function enables minion shire caches and neighborhoods
    \param shire_mask shire to be configured
    \return The function call status, pass/fail.
*/
int Minion_Enable_Shire_Cache_and_Neighborhoods(uint64_t shire_mask);

/*! \fn int Minion_Enable_Master_Shire_Threads(uint8_t mm_id)
    \brief This function enables mastershire threads
    \param mm_id master minion shire id
    \return The function call status, pass/fail.
*/
int Minion_Enable_Master_Shire_Threads(uint8_t mm_id);

/*! \fn int Minion_Reset_Threads(uint64_t minion_shires_mask)
    \brief This function resets the Minion Shire threads
    \param minion_shires_mask Minion Shire Mask
    \return The function call status, pass/fail.
*/
int Minion_Reset_Threads(uint64_t minion_shires_mask);

/*! \fn int Minion_Minion_Configure_Minion_Clock_Reset(uint64_t minion_shires_mask, uint8_t mode, uint8_t lvdpll_mode, bool use_step_clock)
    \brief This function configures the Minion PLLs to Step Clock, and bring them out of reset.
    \param  minion_shires_mask Shire Mask to enable
    \param  hdpll_mode Frequency mode to bring up Minions (Step Clock)
    \param  lvdpll_mode Frequency mode to enable the internal LVDPLL of each Shire
    \param  to enable Minion to use Step clock
    \return The function call status, pass/fail.
*/
int Minion_Configure_Minion_Clock_Reset(uint64_t minion_shires_mask, uint8_t hdpll_mode, uint8_t lvdpll_mode, bool use_step_clock);

/*! \fn uint64_t Minion_Get_Active_Compute_Minion_Mask(void)
    \brief This function gets the active compute shire mask
           by reading the value from SP OTP
    \param N/A
    \return Active CM shire mask.
*/
uint64_t Minion_Get_Active_Compute_Minion_Mask(void);


/*! \fn int Minion_Load_Authenticate_Firmware(void)
    \brief This function loads and authenticates the
           Minions firmware
    \param  N/A
    \return The function call status, pass/fail.
*/
int Minion_Load_Authenticate_Firmware(void);

/*! \fn int Minion_Shire_Update_PLL_Freq(uint32_t freq)
    \brief This function supports updating the Minion
           Shire PLL dynamically without stoppong the
           cores
    \param  freq value of the freq to updated to
    \return The function call status, pass/fail.
*/
int Minion_Shire_Update_PLL_Freq(uint16_t freq);

/*! \fn uint64_t Minion_Read_ESR(uint32_t address)
    \brief This function supports reading a Minion Shire
           ESR and returns value read
    \param  address ESR address offset
    \return value on offset of register address.
*/
uint64_t Minion_Read_ESR(uint32_t address);

/*! \fn int Minion_Write_ESR(uint32_t address, uint64_t data, uint64_t mmshire_mask)
    \brief This function supports writing to a Minion Shire
           ESR with specific data
    \param  address ESR address offset
    \param  data to be written to
    \param  mmshire_mask Shire Mask
    \return The function call status, pass/fail.
*/
int Minion_Write_ESR(uint32_t address, uint64_t data, uint64_t mmshire_mask);

/*! \fn int Minion_Kernel_Launch(uint64_t mmshire_mask, void *args)
    \brief This function supports launching a compute Kernel on specific
            Shires
    \param  Minion Shire mask to launch Compute kernel on
    \param  Arguments to the Compute Kernel
    \return The function call status, pass/fail.
*/
int Minion_Kernel_Launch(uint64_t mmshire_mask, void *args);

/*! \brief Sets the active Shire Mask global
    \param active_shire_mask Mask of active Shires
    \returns none
*/
void Minion_Set_Active_Shire_Mask(uint64_t active_shire_mask);

/*! \fn void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id)
    \brief Process the Minion State related host request
    \param tag_id Tag ID
    \param msg_id Unique enum representing specific command
    \returns none
*/
void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id);

/*! \fn uint64_t Minion_State_MM_Iface_Get_Active_Shire_Mask(void)
    \brief Return current active shires mask.
    \returns none
*/
uint64_t Minion_State_MM_Iface_Get_Active_Shire_Mask(void);

/*! \fn void Minion_State_MM_Error_Handler(int32_t msg_id)
    \brief Process the Minion State errors.
    \param msg_id Unique enum representing specific error.
    \returns none
*/
void Minion_State_MM_Error_Handler(int32_t msg_id);

/*! \fn void Minion_State_MM_Heartbeat_Handler(void)
    \brief Increment MM heartbeat.
    \param none
    \returns none
*/
void Minion_State_MM_Heartbeat_Handler(void);

/*! \fn uint64_t Minion_State_Get_MM_Heartbeat_Count(void)
    \brief Get MM heartbeat count.
    \param none
    \returns none
*/
uint64_t Minion_State_Get_MM_Heartbeat_Count(void);

/*! \fn int32_t Minion_State_Error_Control_Init(dm_event_isr_callback event_cb)
    \brief This function initializes the Minion error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/

int32_t Minion_State_Error_Control_Init(dm_event_isr_callback event_cb);

/*! \fn int32_t Minion_State_Error_Control_Deinit(void)
    \brief This function cleans up the minion error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t Minion_State_Error_Control_Deinit(void);

/*! \fn int32_t Minion_State_Set_Exception_Error_Threshold(uint32_t ce_threshold)
    \brief This function programs the minion exception error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t Minion_State_Set_Exception_Error_Threshold(uint32_t ce_threshold);

/*! \fn int32_t Minion_State_Set_Hang_Error_Threshold(uint32_t ce_threshold)
    \brief This function programs the minion hang error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t Minion_State_Set_Hang_Error_Threshold(uint32_t ce_threshold);

/*! \fn int32_t Minion_State_Get_Exception_Error_Count(uint32_t *err_count)
    \brief This function returns the minion exception error count
    \param err_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t Minion_State_Get_Exception_Error_Count(uint32_t *err_count);

/*! \fn int32_t Minion_State_Get_Hang_Error_Count(uint32_t *err_count)
    \brief This function returns the minion hang error count
    \param err_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t Minion_State_Get_Hang_Error_Count(uint32_t *err_count);

/*! \fn int8_t MM_Init_HeartBeat_Watchdog(void)
    \brief This function creates watchdog timer for MM  heartbeat
    \param NONE
    \return Status indicating success or negative error
*/
int8_t MM_Init_HeartBeat_Watchdog(void);

#endif
