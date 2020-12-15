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
*       This file implements the Service Processor Interface Services.
*
*   FUNCTIONS
*
*       Worker_Iface_Init
*       Worker_Iface_Push
*       Worker_Iface_Pop
*       Worker_Iface_Processing
*       Worker_Iface_Deinit
*
***********************************************************************/
#include "services/worker_iface.h"
#include "services/log1.h"
#include "vq.h"

/*! \var iface_q_cb_t KW_FIFO
    \brief Global SQWs (multi-source) to KW FIFO (sink)
    \warning Not thread safe!
*/
static iface_cb_t KW_FIFO = {0};

/*! \var iface_q_cb_t DMAW_FIFO
    \brief Global SQWs (multi-source) to DMAW FIFO (sink)
    \warning Not thread safe!
*/
static iface_cb_t DMAW_FIFO = {0};

/*! \var iface_q_cb_t CQW_FIFO
    \brief Global Multiple source to CQW FIFO (sink)
    \warning Not thread safe!
*/
static iface_cb_t CQW_FIFO = {0};

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Init
*  
*   DESCRIPTION
*
*       Initialize Worker interface specified by caller
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
int8_t Worker_Iface_Init(uint8_t interface_type)
{
    int8_t status = 0;

    if (interface_type == TO_KW_FIFO)
    {
        /* TODO */

    }
    else if (interface_type == TO_DMAW_FIFO)
    {
        /* TODO */

    }
    else if (interface_type == TO_CQW_FIFO)
    {
        /* TODO */

    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Push_Cmd
*  
*   DESCRIPTION
*
*       Push command to worker interface
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*       p_cmd               Pointer to command that will be pushed to
*                           worker interface
*       cmd_size            Size of command to be pushed to worker interface
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
int8_t Worker_Iface_Push_Cmd(uint8_t interface_type, void* p_cmd, 
    uint32_t cmd_size)
{
    int8_t status;
    vq_cb_t *p_vq_cb;

    if (interface_type == TO_KW_FIFO)
    {
        p_vq_cb = &KW_FIFO.vqueue;
    }
    else if (interface_type == TO_DMAW_FIFO)
    {
        p_vq_cb = &DMAW_FIFO.vqueue;
    }
    else if (interface_type == TO_CQW_FIFO)
    {
        p_vq_cb = &CQW_FIFO.vqueue;
    }

    /* Pop the command from circular buffer */
    status = VQ_Push(p_vq_cb, p_cmd, cmd_size);

    /* Implement logic to post FCC event base on interface type */

    return status;
    
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Pop_Cmd
*  
*   DESCRIPTION
*
*       Pop command from worker interface
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*       rx_buffer           RX buffer to which popped command should be
*                           copied to
*
*   OUTPUTS
*
*       uint32_t            command_size
*
***********************************************************************/
uint32_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff)
{
    uint32_t retval;
    vq_cb_t *p_vq_cb;
    uint32_t command_size;

    if (interface_type == TO_KW_FIFO)
    {
        p_vq_cb = &KW_FIFO.vqueue;
    }
    else if (interface_type == TO_DMAW_FIFO)
    {
        p_vq_cb = &DMAW_FIFO.vqueue;
    }
    else if (interface_type == TO_CQW_FIFO)
    {
        p_vq_cb = &CQW_FIFO.vqueue;
    }

    /* Pop the command from circular buffer */
    command_size = VQ_Pop(p_vq_cb, rx_buff);

    if (command_size) 
    {
        retval = command_size;
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s",
        "ERROR: Circbuff Pop Failed \r\n");
    }

    return retval;
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Deinit
*  
*   DESCRIPTION
*
*       Deinitialize worker interface specied by caller
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*
*   OUTPUTS
*
*       int8_t              status, success or failure
*
***********************************************************************/
int8_t Worker_Iface_Deinit(uint8_t interface_type)
{
    (void) interface_type;
    /* TODO */

    return 0;
}