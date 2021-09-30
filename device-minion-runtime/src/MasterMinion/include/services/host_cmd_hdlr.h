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
*/
#define HOST_CMD_STATUS_ABORTED   -1

/*! \fn int8_t Host_Command_Handler(void* command_buffer, uint8_t sqw_idx,
        uint64_t start_cycles)
    \brief Interface to handle host side commands
    \param command_buffer pointer to command buffer
    \param sqw_idx index of submission queue worker
    \param start_cycles cycle count to measure wait latency
    \return status success or negative error code
*/
int8_t Host_Command_Handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles);

/*! \fn int8_t Host_HP_Command_Handler(void* command_buffer, uint8_t sqw_hp_idx)
    \brief Interface to handle host side high priority commands
    \param command_buffer pointer to command buffer
    \param sqw_hp_idx index of hp submission queue worker
    \return status success or negative error code
*/
int8_t Host_HP_Command_Handler(void* command_buffer, uint8_t sqw_hp_idx);

#endif /* HOST_CMD_HDLR_H */
