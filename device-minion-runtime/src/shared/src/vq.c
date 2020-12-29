
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
*       This file implements the Virtual Queue services.
*
*   FUNCTIONS
*
*       VQ_Init
*       VQ_Push
*       VQ_Pop
*       VQ_Deinit
*
***********************************************************************/
#include "vq.h"

#undef DEBUG_LOG

#ifdef DEBUG_LOG
#include "../../MasterMinion/include/services/log1.h"
#endif

/************************************************************************
*
*   FUNCTION
*
*       VQ_Init
*  
*   DESCRIPTION
*
*       This function configures the virtual queue. The base of the 
*       circular buffer control block is assigned to the base of the
*       virtual queue.
*
*   INPUTS
*
*       vq_base      Virtual queue base address
*       vq_size      Virtual queue size
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
int8_t VQ_Init(vq_cb_t *vq_cb, uint64_t vq_base, uint32_t vq_size, 
        uint16_t peek_offset, uint16_t peek_length, uint32_t flags)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the virtual queue control block */
    vq_cb->circbuff_cb = (circ_buff_cb_t*) vq_base;
    vq_cb->cmd_size_peek_offset = peek_offset;
    vq_cb->cmd_size_peek_length = peek_length;
    vq_cb->flags = flags;

    /* Initialize the SQ circular buffer */
    status = Circbuffer_Init(vq_cb->circbuff_cb, 
                 (uint32_t)(vq_size - sizeof(circ_buff_cb_t)));

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       VQ_Push
*  
*   DESCRIPTION
*
*       This function is used to push a command to the virtual queue.
*
*   INPUTS
*
*       vq_cb      Pointer to virtual queue control block
*       data       Pointer to data to be pushed
*       data_size  Size of data to be pushed
*
*   OUTPUTS
*
*       uint8_t    status of virtual queue push operation
*
***********************************************************************/
int8_t VQ_Push(vq_cb_t* vq_cb, void* data, uint32_t data_size)
{
    int8_t status;

    #ifdef DEBUG_LOG
    Log_Write(LOG_LEVEL_DEBUG, "%s%p%s%p%s%d%s",
        "VQ_Push:dst_addr:", vq_cb->circbuff_cb->buffer_ptr, ":src_addr:", 
        data, ":data_size:", data_size, "\r\n");
    #endif

    status = Circbuffer_Push(vq_cb->circbuff_cb, data, (uint16_t) data_size,
                                vq_cb->flags);

    return status;
}


/************************************************************************
*
*   FUNCTION
*
*       VQ_Pop
*  
*   DESCRIPTION
*
*       This function is used to pop a command from virtual queue.
*
*   INPUTS
*
*       vq_cb      Pointer to virtual queue control block
*       rx_buff    Pointer to rx buffer to copy popped data.
*
*   OUTPUTS
*
*       uint32_t   Number of bytes popped or zero
*
***********************************************************************/
uint32_t VQ_Pop(vq_cb_t* vq_cb, void* rx_buff)
{
    int8_t status;
    uint16_t return_val = 0;
    cmd_size_t command_size;

    /* Peek the command size to pop from SQ */
    status = Circbuffer_Peek(vq_cb->circbuff_cb, (void *)&command_size, 
        vq_cb->cmd_size_peek_offset, vq_cb->cmd_size_peek_length,
        vq_cb->flags);

    if (status == STATUS_SUCCESS) 
    {
        #ifdef DEBUG_LOG
        Log_Write(LOG_LEVEL_DEBUG, "%s%p%s%p%s%d%s",
            "VQ_Pop:src_addr:", vq_cb->circbuff_cb, ":dst_addr:", 
            rx_buff, ":data_size:", command_size, "\r\n");
        #endif

        /* Pop the command from circular buffer */
        status = Circbuffer_Pop(vq_cb->circbuff_cb, rx_buff, command_size,
                    vq_cb->flags);

        if (status == STATUS_SUCCESS) 
        {
            return_val = command_size;
        }
    } 

    return return_val;
}

/************************************************************************
*
*   FUNCTION
*
*       VQ_Peek
*  
*   DESCRIPTION
*
*       This interface allows for peeking into a VQ.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t VQ_Peek(vq_cb_t* vq_cb, void* peek_buff, uint16_t peek_offset, 
    uint16_t peek_length)
{
    int8_t status = 0;

    status = Circbuffer_Peek(vq_cb->circbuff_cb, peek_buff, 
                peek_offset, peek_length, vq_cb->flags);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       VQ_Data_Avail
*  
*   DESCRIPTION
*
*       Is data available to process in the VQ.
*
*   INPUTS
*
*       vq_cb   VQ control block
*
*   OUTPUTS
*
*       bool      Boolean indicating presence of data to process.
*
***********************************************************************/
bool VQ_Data_Avail(vq_cb_t* vq_cb)
{
    return (Circbuffer_Get_Used_Space(vq_cb->circbuff_cb, vq_cb->flags) > 0);
}

/************************************************************************
*
*   FUNCTION
*
*       VQ_Deinit
*  
*   DESCRIPTION
*
*       This function deinitializes the virtual queue.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t VQ_Deinit(void) 
{
    /* TODO: Perform deinit activities for the SQs */

    return 0;
}
