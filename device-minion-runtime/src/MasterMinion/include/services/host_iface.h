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
*       Host to Minion communications
*
***********************************************************************/
#ifndef HOST_IFACE_DEFS_H
#define HOST_IFACE_DEFS_H

#include "common_defs.h"
#include "config/mm_config.h"
#include "vq.h"

#define     HIFACE_INT_PRIORITY    1

typedef enum {
    SQ,
    CQ
} vq_type_t;

/*! \fn int8_t Host_Iface_SQs_Init(void)
    \brief Initialize Host Interface Submission Queues
    \param none
    \return status
*/
int8_t Host_Iface_SQs_Init(void);

/*! \fn vq_cb_t* Host_Iface_Get_VQ_Base_Addr(uint8_t sq_id)
    \brief Obtain pointer to virtuql queue assiciated
    with the submission queue ID
    \param vq_type      Virtual Queue Type
    \param vq_id        Virtual Queue ID
    \return vq_cb_t*    Pointer to the virtual queue control block
*/
vq_cb_t* Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type, uint8_t vq_id);

/*! \fn int8_t Host_Iface_CQs_Init(void)
    \brief Initialize Host Interface Completion Queues
    \param none
    \return status
*/
int8_t Host_Iface_CQs_Init(void);

/*! \fn uint32_t Host_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief Host interface to peek SQ command size
    \param [in] sq_id: Submission queue ID
    \returns [out] Command size
*/
uint32_t Host_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id);

/*! \fn int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void* cmd)
    \brief Host interface to peek SQ command
    \param [in] sq_id: Submission queue ID
    \param [in] cmd: Pointer to command buffer
    \returns [out] Status
*/
int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void* cmd);

/*! \fn uint32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff)
    \brief Host interface SQ pop API
    \param [in] sq_id: Submission queue ID
    \param [in] rx_buff: Pointer to command rx buffer
    \returns [out] Command size
*/
uint32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff);

/*! \fn uint32_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void* rx_buff)
    \brief Host interface CQ push API
    \param [in] sq_id: Completion queue ID
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size
    \returns [out] Status
*/
int8_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void* p_cmd, uint32_t cmd_size);

/*! \fn bool Host_Iface_Interrupt_Status(void)
    \brief Query host interface interrupt status to check if host iface
    processing is needed
    \returns [out] Boolean status
*/
bool Host_Iface_Interrupt_Status(void); 

/*! \fn void Host_Iface_Processing(void)
    \brief Host interface SQ commands processing
    \param None
*/
void Host_Iface_Processing(void);

/*! \fn int8_t Host_Iface_SQs_Deinit(void)
    \brief Host interface SQs deinitialization
    \returns [out] Status
*/
int8_t Host_Iface_SQs_Deinit(void);

/*! \fn int8_t Host_Iface_CQs_Deinit(void)
    \brief Host interface CQs deinitialization
    \returns [out] Status
*/
int8_t Host_Iface_CQs_Deinit(void);

#endif /* HOST_IFACE_DEFS_H */
