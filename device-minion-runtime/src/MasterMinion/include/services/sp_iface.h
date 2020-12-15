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
#include "config/mm_config.h"
#include "vq.h"

/*! \fn int8_t SP_Iface_SQs_Init(void)
    \brief Initialize SP Interface SQs
    \param none
    \return status
*/
int8_t SP_Iface_SQs_Init(void);

/*! \fn int8_t SP_Iface_CQs_Init(void)
    \brief Initialize SP Interface CQs
    \param none
    \return status
*/
int8_t SP_Iface_CQs_Init(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief SP interface to peek SQ command size
    \returns [out] command size
*/
uint16_t SP_Iface_Peek_SQ_Cmd_Size(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd(uint8_t sq_id, void* cmd)
    \brief SP interface to peek SQ command
    \param [in] cmd: Pointer to command buffer
    \returns [out] Command size
*/
uint16_t SP_Iface_Peek_SQ_Cmd(void* cmd);

/*! \fn uint32_t SP_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff)
    \brief SP interface SQ pop API
    \param [in] cmd: Pointer to command rx buffer
    \returns [out] Command size
*/
uint32_t SP_Iface_SQ_Pop_Cmd(void* rx_buff);

/*! \fn uint32_t SP_Iface_CQ_Push_Cmd(uint8_t cq_id, void* rx_buff)
    \brief SP interface CQ push API
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size
    \returns [out] Status
*/
int8_t SP_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size);

/*! \fn bool SP_Iface_Interrupt_Status(void)
    \brief Query SP interface interrupt status to check if SP iface
    processing is needed
    \returns [out] Boolean status
*/
bool SP_Iface_Interrupt_Status(void); 

/*! \fn void SP_Iface_Processing(void)
    \brief SP interface SQ commands processing
    \param None
*/
void SP_Iface_Processing(void);

/*! \fn int8_t SP_Iface_SQs_Deinit(void)
    \brief SP interface SQs deinitialization
    \returns [out] Status
*/
int8_t SP_Iface_SQs_Deinit(void);

/*! \fn int8_t SP_Iface_CQs_Deinit(void)
    \brief SP interface CQs deinitialization
    \returns [out] Status
*/
int8_t SP_Iface_CQs_Deinit(void);
