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
/*! \file circbuff.c
    \brief A C module that implements the Circular Buffer data structure

    Public interfaces:
        Circbuffer_Init
        Circbuffer_Push
        Circbuffer_Pop
        Circbuffer_Peek
*/
/***********************************************************************/
#include "circbuff.h"

/************************************************************************
*
*   FUNCTION
*
*       Circbuffer_Init
*  
*   DESCRIPTION
*
*       This function initializes circular buffer instance and resets the 
*       head, tail and data buffer.
*
*   INPUTS
*
*       circ_buff_cb_ptr  Pointer to circular buffer control block.
*       buffer_length     Length of the circular data buffer in bytes.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t Circbuffer_Init(circ_buff_cb_t *circ_buff_cb_ptr, 
        uint32_t buffer_length)
{
    /* Reset the head and tail offsets */
    circ_buff_cb_ptr->head_offset = 0;
    circ_buff_cb_ptr->tail_offset = 0;

    /* Set the buffer length */
    circ_buff_cb_ptr->length = buffer_length;

    /* Reset the circular buffer memory */
    for (uint32_t i = 0; i < buffer_length; i++) 
    {
        circ_buff_cb_ptr->buffer_ptr[i] = 0;
    }

    return CIRCBUFF_OPERATION_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Circbuffer_Push
*  
*   DESCRIPTION
*
*       This function writes the given number of bytes from source data  
*       buffer to destination circular buffer and increments the head 
*       offset. The length of the data to be written must not be greater 
*       than the length of circular buffer.
*
*   INPUTS
*
*       circ_buff_cb_ptr  Pointer to circular buffer control block.
*       src_buffer        Pointer to source data buffer.
*       src_length        Length of the source data buffer in bytes.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t Circbuffer_Push(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr, 
    const void *restrict const src_buffer, uint32_t src_length, uint32_t flags)
{
    uint32_t head_offset = circ_buff_cb_ptr->head_offset;
    const uint32_t cbuffer_length = circ_buff_cb_ptr->length;
    const uint8_t *const src_data_ptr = src_buffer;

    (void)flags;

    if (src_length > cbuffer_length) 
    {
        return CIRCBUFF_ERROR_BAD_LENGTH;
    }

    if (Circbuffer_Get_Avail_Space(circ_buff_cb_ptr, flags) == 0) 
    {
        return CIRCBUFF_ERROR_FULL;
    }

    if (head_offset >= cbuffer_length) 
    {
        return CIRCBUFF_ERROR_BAD_HEAD_INDEX;
    } 
    else 
    {
        for (uint32_t i = 0; i < src_length; i++) 
        {
            /* TODO: Use flags filed specified memory type to access circ buff */
            circ_buff_cb_ptr->buffer_ptr[head_offset] = src_data_ptr[i];
            head_offset = (uint32_t)((head_offset + 1) % cbuffer_length);
        }

        circ_buff_cb_ptr->head_offset = head_offset;

        return CIRCBUFF_OPERATION_SUCCESS;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Circbuffer_Pop
*  
*   DESCRIPTION
*
*       This function reads the given number of bytes from circular buffer 
*       to the given destination data buffer and increments the tail 
*       offset. The length of the data to read must not be greater than the 
*       available data in circular buffer.
*
*   INPUTS
*
*       circ_buff_cb_ptr  Pointer to circular buffer control block.
*       dest_buffer       Pointer to destination data buffer.
*       dest_length       Length of the destination data buffer in bytes.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t Circbuffer_Pop(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr, 
        void *restrict const dest_buffer, uint32_t dest_length, uint32_t flags)
{
    uint32_t tail_offset = circ_buff_cb_ptr->tail_offset;
    const uint32_t cbuffer_length = circ_buff_cb_ptr->length;
    uint8_t *const dest_data_ptr = dest_buffer;
    const uint32_t used_space = Circbuffer_Get_Used_Space(circ_buff_cb_ptr, flags);

    (void)flags;

    if (used_space == 0) 
    {
        return CIRCBUFF_ERROR_EMPTY;
    }

    if (dest_length > used_space) 
    {
        return CIRCBUFF_ERROR_BAD_LENGTH;
    }

    if (tail_offset >= cbuffer_length) 
    {
        return CIRCBUFF_ERROR_BAD_TAIL_INDEX;
    } 
    else 
    {
        for (uint32_t i = 0; i < dest_length; i++) 
        {
            /* TODO: Use flags filed specified memory type to access circ buff */
            dest_data_ptr[i] = circ_buff_cb_ptr->buffer_ptr[tail_offset];
            tail_offset = (uint32_t)((tail_offset + 1U) % cbuffer_length);
        }

        circ_buff_cb_ptr->tail_offset = tail_offset;

        return CIRCBUFF_OPERATION_SUCCESS;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Circbuffer_Peek
*  
*   DESCRIPTION
*
*       This function peeks into the circular buffer and reads the given 
*       number of bytes from circular buffer to the given destination data
*       buffer. It does not increment the tail offset. The length of the 
*       data to peek must not be greater than the available data in 
*       circular buffer.
*
*   INPUTS
*
*       circ_buff_cb_ptr  Pointer to circular buffer control block.
*       dest_buffer       Pointer to destination data buffer.
*       peek_length       Length of data to peek and store in dest_buffer.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t Circbuffer_Peek(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr, 
    void *restrict const dest_buffer, uint16_t peek_offset, uint16_t peek_length, 
    uint32_t flags)
{
    uint32_t tail_offset = circ_buff_cb_ptr->tail_offset;
    const uint32_t cbuffer_length = circ_buff_cb_ptr->length;
    uint8_t *const dest_data_ptr = dest_buffer;
    const uint32_t used_space = Circbuffer_Get_Used_Space(circ_buff_cb_ptr, flags);

    (void)flags;

    if (used_space == 0) 
    {
        return CIRCBUFF_ERROR_EMPTY;
    }

    if (peek_length > used_space) 
    {
        return CIRCBUFF_ERROR_BAD_LENGTH;
    }

    if (tail_offset >= cbuffer_length) 
    {
        return CIRCBUFF_ERROR_BAD_TAIL_INDEX;
    } 
    else 
    {
        for (uint16_t i = 0; i < peek_length; i++) 
        {
            /* TODO: Use flags filed specified memory type to access circ buff */
            dest_data_ptr[i] = circ_buff_cb_ptr->buffer_ptr[tail_offset + peek_offset];
            tail_offset = (uint32_t)((tail_offset + 1U) % cbuffer_length);
        }

        return CIRCBUFF_OPERATION_SUCCESS;
    }
}
