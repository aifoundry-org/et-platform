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
/***********************************************************************/
/*! \file sp_iface.h
    \brief A C header that defines the Service Processor public
    interfaces.
*/
/***********************************************************************/
#ifndef SP_IFACE_DEFS_H
#define SP_IFACE_DEFS_H

#include "common_defs.h"
#include "sp_mm_iface.h" /* header from shared/helper lib */
#include "sp_mm_comms_spec.h"

/*! \def TIMEOUT_SP_IFACE_RESPONSE(x)
    \brief Timeout value (per 100ms) for SP response wait
*/
#define TIMEOUT_SP_IFACE_RESPONSE(x)       (x * 2U)

/*! \def SP_IFACE_MM_HEARTBEAT_INTERVAL(x)
    \brief Periodic Interval value after which a heartbeat is sent to SP
*/
#define SP_IFACE_MM_HEARTBEAT_INTERVAL(x)   (x * 1U)

/*! \def SP_IFACE_INVALID_SHIRE_MASK
    \brief SP iface error code - Invalid shire mask
*/
#define SP_IFACE_INVALID_SHIRE_MASK       -1

/*! \def SP_IFACE_INVALID_BOOT_FREQ
    \brief SP iface error code - Invalid boot frequency
*/
#define SP_IFACE_INVALID_BOOT_FREQ        -2

/*! \def SP_IFACE_TIMER_REGISTER_FAILED
    \brief SP iface error code - Timer resgistration failure
*/
#define SP_IFACE_TIMER_REGISTER_FAILED    -3

/*! \def SP_IFACE_SP_RSP_TIMEDOUT
    \brief SP iface error code - SP response timeout occurred
*/
#define SP_IFACE_SP_RSP_TIMEDOUT          -4

/*! \def SP_IFACE_INVALID_FW_VERSION
    \brief SP iface error code - Invalid fw version
*/
#define SP_IFACE_INVALID_FW_VERSION       -5

/*! \def SP_IFACE_SP2MM_CMD_POP_FAILED
    \brief SP iface error code - SP2MM cmd pop failure
*/
#define SP_IFACE_SP2MM_CMD_POP_FAILED      -6

/*! \fn int8_t SP_Iface_Init(void)
    \brief Initialize Mm interface to Service Processor (SP)
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Init(void);

/*! \fn int8_t SP_Iface_Push_Cmd_To_MM2SP_SQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Master Minion (MM) to Service Processor (SP)
    Submission Queue(SQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define SP_Iface_Push_Cmd_To_MM2SP_SQ(p_cmd, cmd_size)   \
    SP_MM_Iface_Push(SP_SQ, p_cmd, cmd_size)

/*! \fn int8_t SP_Iface_Pop_Cmd_From_MM2SP_CQ(void* rx_buff)
    \brief Pop response from to Master Minion (MM) to Service Processor (SP)
    Completion Queue(CQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
#define SP_Iface_Pop_Rsp_From_MM2SP_CQ(rx_buff)   \
    SP_MM_Iface_Pop(SP_CQ, rx_buff)

/*! \fn int8_t SP_Iface_Push_Rsp_To_SP2MM_CQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Service Processor (SP) to Master Minion (MM)
    Completion Queue(CQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define SP_Iface_Push_Rsp_To_SP2MM_CQ(p_cmd, cmd_size)   \
    SP_MM_Iface_Push(MM_CQ, p_cmd, cmd_size)

/*! \fn int8_t SP_Iface_Pop_Cmd_From_SP2MM_SQ(void* rx_buff)
    \brief Pop response from to Service Processor (SP) to Master Minion (MM)
    Submission Queue(SQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
#define SP_Iface_Pop_Cmd_From_SP2MM_SQ(rx_buff)   \
    SP_MM_Iface_Pop(MM_SQ, rx_buff)

/*! \fn void SP_Iface_Processing(void)
    \brief An API to process messages from SP on receiving
    1. MM2SP_CQ post notification
    2. SP2MM SQ post notification
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Processing(void);

/*! \fn int8_t SP_Iface_Get_Shire_Mask(uint64_t *shire_mask)
    \brief A blocking API to obtain active compute shires mask from service processor
    \param shire_mask Pointer to shire mask variable, as return arg.
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Get_Shire_Mask(uint64_t *shire_mask);

/*! \fn int8_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq)
    \brief A blocking API to obtain compute minin boot frequency from service processor
    \param boot_freq Pointer to boot frequency variable, as return arg.
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq);

/*! \fn int8_t SP_Iface_Get_Fw_Version(enum mm2sp_fw_type_e fw_type, uint16_t *major, uint16_t *minor,
    uint16_t *patch)
    \brief A blocking API to obtain FW version from service processor
    \param fw_type Type of the FW
    \param major Pointer to variable where major version is stored
    \param minor Pointer to variable where minor version is stored
    \param revision Pointer to variable where revision version is stored
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Get_Fw_Version(mm2sp_fw_type_e fw_type, uint8_t *major, uint8_t *minor,
    uint8_t *revision);

/*! \fn int8_t SP_Iface_Report_Error(mm2sp_error_type_e error_type, int16_t error_code)
    \brief A non-blocking API to report Master Minion error codes to Service Processor
    \param error_type Error type
    \param error_code Error Code
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Report_Error(mm2sp_error_type_e error_type, int16_t error_code);

/*! \fn int8_t SP_Iface_Setup_MM_HeartBeat(void)
    \brief This function initializes the MM->SP heartbeat with a periodic timer.
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Setup_MM_HeartBeat(void);

#endif
