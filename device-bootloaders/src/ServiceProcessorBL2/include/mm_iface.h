/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for public interfaces that provide
*       SP to Minion communications
*
***********************************************************************/
#ifndef MM_IFACE_H
#define MM_IFACE_H

#include <esperanto/device-apis/management-api/device_mgmt_api_spec.h> /* TODO: eliminate this */
#include <esperanto/device-apis/management-api/device_mgmt_api_rpc_types.h>
#include "etsoc/common/common_defs.h"
#include "transports/sp_mm_iface/sp_mm_iface.h"
#include "minion_configuration.h"
#include "FreeRTOS.h"

/*! \def SP_MM_CQ_MAX_ELEMENT_SIZE
    \brief Macro for specifying the maximum element size of SP-MM SQ.
*/
#define SP_MM_CQ_MAX_ELEMENT_SIZE 64U

/*! \fn int8_t MM_Iface_Init(void)
    \brief Initialize the SP to Master Minion interface.
    \param none
    \return Status indicating success or negative error
*/
int8_t MM_Iface_Init(void);

/*! \fn int8_t MM_Iface_Send_Echo_Cmd(void)
    \brief Send the Echo command to Master Minion. Note a timeout failure is possible.
    \param none
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Send_Echo_Cmd(void);

/*! \fn int32_t MM_Iface_MM_Command_Shell(const void* cmd, uint32_t cmd_size,
    char* rsp, uint32_t *rsp_size, uint32_t timeout_ms, uint8_t num_of_rsp)
    \brief Receive the TF command shell command, send
    the encapsulatedMM command to MM FW, wait and receive the response from
    MM FW, wrap the MM response in TF response shell, and return the response.
    \param cmd Pointer to MM device-api command
    \param cmd_size Size of MM device-api command
    \param rsp Pointer to receive the command's response
    \param rsp_size Size of reponse received
    \param timeout_ms Timeout of command in ms
    \param num_of_rsp Number of responses to expect
    \return Status indicating success or negative error
*/
int32_t MM_Iface_MM_Command_Shell(const void *cmd, uint32_t cmd_size, char *rsp, uint32_t *rsp_size,
                                  uint32_t timeout_ms, uint8_t num_of_rsp);

/*! \fn int32_t MM_Iface_Get_DRAM_BW(uint32_t *read_bw, uint32_t *write_bw)
    \brief Send the Get DRAM BW command to Master Minion. Note a timeout failure is possible.
    \param read_bw response containing read BW
    \param write_bw response containing write BW
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Get_DRAM_BW(uint32_t *read_bw, uint32_t *write_bw);

/*! \fn int32_t MM_Iface_Get_MM_Stats(struct compute_resources_sample *stats)
    \brief Send the Get MM Stats command to Master Minion. Note a timeout failure is possible.
    \param stats response containing mm stats
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Get_MM_Stats(struct compute_resources_sample *stats);

/*! \fn int32_t MM_Iface_MM_Stats_Run_Control(sp2mm_stats_control_e control)
    \brief Send the MM Stats run control command to Master Minion. Note a timeout failure is possible.
    \param control Control operation to be performed on MM stats.
    \return Status indicating success or negative error
*/
int32_t MM_Iface_MM_Stats_Run_Control(sp2mm_stats_control_e control);

/*! \fn int32_t MM_Iface_Send_Abort_All_Cmd(void)
    \brief Send the Get Abort command to Master Minion Firmware.
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Send_Abort_All_Cmd(void);

/*! \fn int32_t MM_Iface_Wait_For_CM_Boot_Cmd(uint64_t shire_mask)
    \brief Send a warm reset to CM and wait for them to boot.
    \param shire_mask Minion shire mask to perform reset on
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Wait_For_CM_Boot_Cmd(uint64_t shire_mask);

/*! \fn int8_t MM_Iface_Push_Cmd_To_SP2MM_SQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Service Processor (SP) to Master Minion (MM)
    Submission Queue(SQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
int8_t MM_Iface_Push_Cmd_To_SP2MM_SQ(const void *p_cmd, uint32_t cmd_size);

/*! \fn int8_t MM_Iface_Pop_Rsp_From_SP2MM_CQ(void* rx_buff)
    \brief Pop response from Service Processor (SP) to Master Minion (MM)
    Completion Queue(CQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Pop_Rsp_From_SP2MM_CQ(void *rx_buff);

/*! \fn int32_t MM_Iface_Pop_Cmd_From_MM2SP_SQ(void* rx_buff)
    \brief Pop command from Master Minion (MM) to Service Processor (SP)
    Submission Queue(SQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Pop_Cmd_From_MM2SP_SQ(void *rx_buff);

/*! \fn int8_t MM_Iface_Push_Rsp_To_MM2SP_CQ(const void* p_rsp, uint32_t rsp_size)
    \brief Push response from Master Minion (MM) to Service Processor (SP)
    Completion Queue(CQ)
    \param p_rsp Pointer to response buffer
    \param rsp_size Size of response
    \return Status indicating success or negative error
*/
int8_t MM_Iface_Push_Rsp_To_MM2SP_CQ(const void *p_rsp, uint32_t rsp_size);

/*! \fn int32_t MM_Iface_Send_Update_Freq_Cmd(uint16_t freq)
    \brief PThis sends Frequency update command to Master Minion Firmware.
    \param freq Frequency value to set
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Send_Update_Freq_Cmd(uint16_t freq);
#endif
