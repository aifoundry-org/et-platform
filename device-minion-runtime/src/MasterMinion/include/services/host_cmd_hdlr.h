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
/*! \file host_cmd_hdlr.h
    \brief A C header that defines the Host Command Handler component's
    public interfaces. This interface privides services to handle all
    commands received from the Host over PCIe.
*/
/***********************************************************************/
#ifndef HOST_CMD_HDLR_H
#define HOST_CMD_HDLR_H

/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>

/* common-api, device_ops_api */
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>

/*! \def HOST_CMD_STATUS_ABORTED
    \brief Host command handler - Command aborted
    NOTE: This is common for all Device API commands that's why it is kept max negative value.
    TODO:SW-9109: Make this a unique error code.
*/
#define HOST_CMD_STATUS_ABORTED -128

/*! \def HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE
    \brief Host command handler - Invalid firmware type
*/
#define HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE -2

/*! \def HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED
    \brief Host command handler - Firmware version query to SP (MM to SP Interface)
           is failed.
*/
#define HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED -3

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_MAJOR
    \brief Host command handler - Invalid Major firmware version
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_MAJOR -4

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_MINOR
    \brief Host command handler - Invalid Minor firmware version
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_MINOR -5

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_PATCH
    \brief Host command handler - Invalid patch for current firmware
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_PATCH -6

/*! \fn int8_t Host_Command_Handler(void* command_buffer, uint8_t sqw_idx,
        uint64_t start_cycles)
    \brief Interface to handle host side commands
    \param command_buffer pointer to command buffer
    \param sqw_idx index of submission queue worker
    \param start_cycles cycle count to measure wait latency
    \return status success or negative error code
*/
int8_t Host_Command_Handler(void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles);

/*! \fn int8_t Host_HP_Command_Handler(void* command_buffer, uint8_t sqw_hp_idx)
    \brief Interface to handle host side high priority commands
    \param command_buffer pointer to command buffer
    \param sqw_hp_idx index of hp submission queue worker
    \return status success or negative error code
*/
int8_t Host_HP_Command_Handler(void *command_buffer, uint8_t sqw_hp_idx);

#endif /* HOST_CMD_HDLR_H */
