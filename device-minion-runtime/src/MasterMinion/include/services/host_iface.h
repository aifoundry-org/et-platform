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
/*! \file host_iface.h
    \brief A C header that defines the Host Interface component's
    public interfaces. These interfaces provide services using which
    the master minion runtime can communicate with host over the virtual
    queue interface. The virtual queue interface comprises of submission 
    queues using which host submits commands to the device and completion 
    using which device responds to host commands, and sends asynchronous
    events to host.
*/
/***********************************************************************/
#ifndef HOST_IFACE_DEFS_H
#define HOST_IFACE_DEFS_H

#include "common_defs.h"
#include "config/mm_config.h"
#include "vq.h"

/*! \def HIFACE_INT_PRIORITY
    \brief Macro that provides the Host Interface interrupt priority.
*/
#define     HIFACE_INT_PRIORITY    1

/**
 * @brief Enum of supported virtual queue types
 */
typedef enum {
    SQ,
    CQ
} vq_type_t;

/*! \fn int8_t Host_Iface_SQs_Init(void)
    \brief Initialize Host Interface Submission Queues
    \return Status indicating sucess or a negative error code
*/
int8_t Host_Iface_SQs_Init(void);

/*! \fn vq_cb_t* Host_Iface_Get_VQ_Base_Addr(uint8_t sq_id)
    \brief Obtain pointer to virtual queue assiciated
    with the queue type and identifier
    \param vq_type Virtual Queue Type
    \param vq_id Virtual Queue ID
    \return vq_cb_t* Pointer to the virtual queue control block
*/
vq_cb_t* Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type, uint8_t vq_id);

/*! \fn int8_t Host_Iface_CQs_Init(void)
    \brief Initialize Host Interface Completion Queues
    \return Status indicating success or a negative error code
*/
int8_t Host_Iface_CQs_Init(void);

/*! \fn uint32_t Host_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief Interface to peek into submission queue identified
    by the sq_id
    \param sq_id Submission queue ID
    \returns Size of command submitted from host
*/
uint32_t Host_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id);

/*! \fn int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void* cmd)
    \brief Interface to peek into submission queue identiefied by
    the submission queue identifier and obtain the first valid command
    in the cmd pointer provided by the caller
    \param sq_id Submission queue ID
    \param cmd Pointer to command buffer
    \returns Status indicating success or negative error
*/
int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void* cmd);

/*! \fn uint32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff)
    \brief Interface to pop a command from submission queue
    identified by submission queue identifier. The popped command
    is copied into the rx_buff provided by the caller
    \param sq_id Submission queue ID
    \param rx_buff Pointer to command rx buffer
    \return Command size
*/
uint32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff);

/*! \fn int8_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void* p_cmd, uint32_t cmd_size)
    \brief Interface to push a command to the completion queue identified by
    the completion queue identifier
    \param cq_id Completion queue ID
    \param p_cmd Pointer to command rx buffer
    \param cmd_size Command size
    \return Status indicating success or negative error
*/
int8_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void* p_cmd, uint32_t cmd_size);

/*! \fn bool Host_Iface_Interrupt_Status(void)
    \brief Query host interface interrupt status to check if host iface
    processing is needed
    \return Boolean indicating host interface interrupt status
*/
bool Host_Iface_Interrupt_Status(void); 

/*! \fn void Host_Iface_Processing(void)
    \brief Interface to process a host command fetched 
    \return none
*/
void Host_Iface_Processing(void);

/*! \fn int8_t Host_Iface_SQs_Deinit(void)
    \brief Host interface SQs deinitialization
    \return Status indicating success or negative error
*/
int8_t Host_Iface_SQs_Deinit(void);

/*! \fn int8_t Host_Iface_CQs_Deinit(void)
    \brief Host interface CQs deinitialization
    \returns Status indicating success or negative error
*/
int8_t Host_Iface_CQs_Deinit(void);

#endif /* HOST_IFACE_DEFS_H */
