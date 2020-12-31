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
#include "config/mm_config.h"
#include "vq.h"

/*! \fn int8_t SP_Iface_SQs_Init(void)
    \brief Initialize SP Interface SQs
    \return Statuc indicating success or negative error
*/
int8_t SP_Iface_SQs_Init(void);

/*! \fn int8_t SP_Iface_CQs_Init(void)
    \brief Initialize SP Interface CQs
    \return Status indiicating success or negative error
*/
int8_t SP_Iface_CQs_Init(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief SP interface to peek SQ command size
    \return Command size
*/
uint16_t SP_Iface_Peek_SQ_Cmd_Size(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd(uint8_t sq_id, void* cmd)
    \brief SP interface to peek fetch SQ command, the command
    is copied to the caller provided cmd bufffer
    \param cmd Pointer to command buffer
    \return Command size
*/
uint16_t SP_Iface_Peek_SQ_Cmd(void* cmd);

/*! \fn uint32_t SP_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff)
    \brief Interface to pop a command from the SP SQ
    \param rx_buff Pointer to command rx buffer provided by caller
    \return Command size
*/
uint32_t SP_Iface_SQ_Pop_Cmd(void* rx_buff);

/*! \fn uint32_t SP_Iface_CQ_Push_Cmd(uint8_t cq_id, void* rx_buff)
    \brief Interface to push command to the SP CQ
    \param p_cmd Pointer to command rx buffer
    \param cmd_size Command size
    \return Status indicating success or negative error
*/
int8_t SP_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size);

/*! \fn bool SP_Iface_Interrupt_Status(void)
    \brief Interface to query for Service Processor interface interrupt status
    \return Boolean status indicating interrupt status
*/
bool SP_Iface_Interrupt_Status(void); 

/*! \fn void SP_Iface_Processing(void)
    \brief Interface to process availabel commands in the SP SQ
    \return none
*/
void SP_Iface_Processing(void);

/*! \fn int8_t SP_Iface_SQs_Deinit(void)
    \brief SP interface SQs deinitialization
    \returns Status indicating success or negative error
*/
int8_t SP_Iface_SQs_Deinit(void);

/*! \fn int8_t SP_Iface_CQs_Deinit(void)
    \brief SP interface CQs deinitialization
    \returns Status indicating success or negative error
*/
int8_t SP_Iface_CQs_Deinit(void);

#endif /* SP_IFACE_DEFS_H */