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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for public interfaces that provide
*       SP to Minion communications
*
***********************************************************************/
#include "common_defs.h"
#include "vq.h"
#include "sp_vq_build_config.h"

/*! \fn int8_t SP_Iface_SQs_Init(void)
    \brief Initialize SP Interface SQs
    \param none
    \return status
*/
int8_t SP_Host_Iface_SQ_Init(void);

/*! \fn int8_t SP_Iface_CQs_Init(void)
    \brief Initialize SP Interface CQs
    \param none
    \return status
*/
int8_t SP_Host_Iface_CQ_Init(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief SP interface to peek SQ command size
    \returns [out] command size
*/
uint32_t SP_Host_Iface_SQ_Pop_Cmd(void* rx_buff);

/*! \fn uint32_t SP_Iface_CQ_Push_Cmd(uint8_t cq_id, void* rx_buff)
    \brief SP interface CQ push API
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size
    \returns [out] Status
*/
int8_t SP_Host_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size);
