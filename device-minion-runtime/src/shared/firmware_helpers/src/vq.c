
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
/*! \file vq.c
    \brief A C module that implements the Virtual Queue Driver

    Public interfaces:
        VQ_Init
        VQ_Push
        VQ_Pop
        VQ_Pop_Optimized
        VQ_Data_Avail
        VQ_Deinit
*/
/***********************************************************************/
#include "vq.h"

/* Uncomment following line to enable VG debug messages. */
/* #define VQ_DEBUG_LOG */
#ifdef VQ_DEBUG_LOG
#include "../../../MasterMinion/include/services/log.h"
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
    int8_t status = -1;

    /* Use local atomic stores for Master Minion */
#if defined(MASTER_MINION)
    uint64_t temp = 0;

    /* Initialize the virtual queue control block */
    atomic_store_local_64((uint64_t*)&vq_cb->circbuff_cb,
        (uint64_t)vq_base);

    temp = (((uint64_t)flags << 32) |
        ((uint32_t)peek_length << 16) |
        peek_offset);
    atomic_store_local_64((uint64_t*)(void*)&vq_cb->cmd_size_peek_offset,
        temp);

    /* Initialize the circular buffer in shared memory */
    status = Circbuffer_Init((circ_buff_cb_t*)vq_base,
        (uint32_t)(vq_size - sizeof(circ_buff_cb_t)),
        atomic_load_local_32(&vq_cb->flags));

#elif defined(SERVICE_PROCESSOR_BL2)
    /* Initialize the virtual queue control block */
    vq_cb->circbuff_cb = (circ_buff_cb_t*) vq_base;
    vq_cb->cmd_size_peek_offset = peek_offset;
    vq_cb->cmd_size_peek_length = peek_length;
    vq_cb->flags = flags;

    /* Initialize the SQ circular buffer */
    status = Circbuffer_Init(vq_cb->circbuff_cb,
        (uint32_t)(vq_size - sizeof(circ_buff_cb_t)), vq_cb->flags);

#else
    /* Unused variables */
    (void)vq_cb;
    (void)vq_base;
    (void)vq_size;
    (void)peek_offset;
    (void)peek_length;
    (void)flags;
#endif

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
    int8_t status = -1;

    #ifdef VQ_DEBUG_LOG
    Log_Write(LOG_LEVEL_DEBUG, "%s%p%s%p%s%d%s",
        "VQ_Push:dst_addr:", vq_cb->circbuff_cb->buffer_ptr, ":src_addr:",
        data, ":data_size:", data_size, "\r\n");
    #endif

    /* Use local atomic load/stores for Master Minion */
#if defined(MASTER_MINION)
    status = Circbuffer_Push((circ_buff_cb_t*)(uintptr_t)
        atomic_load_local_64((uint64_t*)(void*)&vq_cb->circbuff_cb), data,
        data_size, atomic_load_local_32(&vq_cb->flags));

#elif defined(SERVICE_PROCESSOR_BL2)
    status = Circbuffer_Push(vq_cb->circbuff_cb, data,
        data_size, vq_cb->flags);

#else
    /* Unused variables */
    (void)vq_cb;
    (void)data;
    (void)data_size;
#endif

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
*       int32_t    Negative value - error
*                  zero - No Data
*                  Positive value - Number of bytes popped
*
***********************************************************************/
int32_t VQ_Pop(vq_cb_t* vq_cb, void* rx_buff)
{
    int32_t return_val;
    cmd_size_t command_size;

    /* Use local atomic load/stores for Master Minion */
#if defined(MASTER_MINION)
    uint64_t temp_addr;
    uint64_t temp_vals;

    /* Load the required variables */
    temp_addr =
        atomic_load_local_64((uint64_t*)(void*)&vq_cb->circbuff_cb);
    temp_vals =
        atomic_load_local_64((uint64_t*)(void*)&vq_cb->cmd_size_peek_offset);

    /* Peek the command size to pop from SQ */
    return_val = Circbuffer_Peek((circ_buff_cb_t*)(uintptr_t)temp_addr,
        (void *)&command_size, (uint16_t)(temp_vals & 0xFFFF),
        (uint16_t)((temp_vals >> 16) & 0xFFFF),
        (uint32_t)(temp_vals >> 32));

#elif defined(SERVICE_PROCESSOR_BL2)
    /* Peek the command size to pop from SQ */
    return_val = Circbuffer_Peek(vq_cb->circbuff_cb, (void *)&command_size,
        vq_cb->cmd_size_peek_offset, vq_cb->cmd_size_peek_length,
        vq_cb->flags);

#else
    /* Unused variables */
    (void)vq_cb;
    (void)rx_buff;
#endif

    if (return_val == STATUS_SUCCESS)
    {
        #ifdef VQ_DEBUG_LOG
        Log_Write(LOG_LEVEL_DEBUG, "%s%p%s%p%s%d%s",
            "VQ_Pop:src_addr:", vq_cb->circbuff_cb, ":dst_addr:",
            rx_buff, ":data_size:", command_size, "\r\n");
        #endif
        if (command_size > 0)
        {
            /* Use local atomic load/stores for Master Minion */
#if defined(MASTER_MINION)
            /* Pop the command from circular buffer */
            return_val = Circbuffer_Pop((circ_buff_cb_t*)(uintptr_t)temp_addr,
                rx_buff, command_size, (uint32_t)(temp_vals >> 32));

#elif defined(SERVICE_PROCESSOR_BL2)
            /* Pop the command from circular buffer */
            return_val = Circbuffer_Pop(vq_cb->circbuff_cb, rx_buff,
                command_size, vq_cb->flags);
#endif
            if (return_val == STATUS_SUCCESS)
            {
                return_val = command_size;
            }
        }
        else
        {
            return_val = VQ_ERROR_INVLD_CMD_SIZE;
        }
    }
    else if (return_val == CIRCBUFF_ERROR_EMPTY)
    {
        /* No more data */
        return_val = 0;
    }

    return return_val;
}

/************************************************************************
*
*   FUNCTION
*
*       VQ_Pop_Optimized
*
*   DESCRIPTION
*
*       This function is used to pop a command from virtual queue. Note that
*       this is an optimized version of VQ pop which operates on cached VQ
*       pointers.
*
*   INPUTS
*
*       vq_cb          Pointer to virtual queue control block
*       vq_used_space  Number of bytes available to pop
*       shared_mem_ptr Pointer to shared circular buffer pointer
*       rx_buff        Pointer to rx buffer to copy popped data
*
*   OUTPUTS
*
*       int32_t        Negative value - error
*                      Positive value - Number of bytes popped
*
***********************************************************************/
int32_t VQ_Pop_Optimized(vq_cb_t* vq_cb, uint64_t vq_used_space,
    void *const shared_mem_ptr,  void* rx_buff)
{
    int32_t return_val;
    uint16_t cmd_size = 0;
    uint32_t payload_size;

    /* Pop the header from circular buffer */
    return_val = Circbuffer_Read(vq_cb->circbuff_cb, shared_mem_ptr,
        rx_buff, DEVICE_CMD_HEADER_SIZE, vq_cb->flags);

    if (return_val == STATUS_SUCCESS)
    {
        /* Get the size of the command header + payload */
        cmd_size = DEVICE_GET_CMD_SIZE(rx_buff);
    }

    /* If payload is available.
       Command size should be at least equal to command header + payload. */
    if(cmd_size > DEVICE_CMD_HEADER_SIZE)
    {
        payload_size = (cmd_size - DEVICE_CMD_HEADER_SIZE);

        /* Verify the payload size */
        if (payload_size <= vq_used_space)
        {
            /* Pop the command payload from circular buffer */
            return_val = Circbuffer_Read(vq_cb->circbuff_cb, shared_mem_ptr,
                ((uint8_t*)rx_buff) + DEVICE_CMD_HEADER_SIZE, payload_size, vq_cb->flags);

            /* Populate the popped size */
            if (return_val == STATUS_SUCCESS)
            {
                return_val = cmd_size;
            }
        }
        else
        {
            /* Bad length of payload in command */
            return_val = VQ_ERROR_BAD_PAYLOAD_LENGTH;
        }
    }
    else if(cmd_size == DEVICE_CMD_HEADER_SIZE)
    {
        /* Populate the popped size */
        return_val = cmd_size;
    }
    else
    {
        return_val = VQ_ERROR_INVLD_CMD_SIZE;
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
    int8_t status = -1;

    /* Use local atomic load/stores for Master Minion */
#if defined(MASTER_MINION)
    status = Circbuffer_Peek((circ_buff_cb_t*)(uintptr_t)
        atomic_load_local_64((uint64_t*)(void*)&vq_cb->circbuff_cb),
        peek_buff, peek_offset, peek_length,
        atomic_load_local_32(&vq_cb->flags));

#elif defined(SERVICE_PROCESSOR_BL2)
    status = Circbuffer_Peek(vq_cb->circbuff_cb, peek_buff,
                peek_offset, peek_length, vq_cb->flags);

#else
    /* Unused variables */
    (void)vq_cb;
    (void)peek_buff;
    (void)peek_offset;
    (void)peek_length;
#endif

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
    /* Use local atomic load/stores for Master Minion */
#if defined(MASTER_MINION)
    return (Circbuffer_Get_Used_Space((circ_buff_cb_t*)(uintptr_t)
            atomic_load_local_64((uint64_t*)(void*)&vq_cb->circbuff_cb),
            atomic_load_local_32(&vq_cb->flags)) > 0);

#elif defined(SERVICE_PROCESSOR_BL2)
    return (Circbuffer_Get_Used_Space(vq_cb->circbuff_cb,
            vq_cb->flags) > 0);

#else
    /* Unused variables */
    (void)vq_cb;

    return false;
#endif
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
