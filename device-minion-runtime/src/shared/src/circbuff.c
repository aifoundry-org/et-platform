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
        Circbuffer_Read
*/
/***********************************************************************/
#include "circbuff.h"

/*! \var void memory_read
    \brief An array containing function pointers to ETSOC memory read functions.
    \warning Not thread safe!
*/
void (*memory_read[MEM_TYPES_COUNT])
    (const void *src_ptr, void *dest_ptr, uint32_t length) __attribute__((aligned(64))) =
    { ETSOC_Memory_Read_Local_Atomic, ETSOC_Memory_Read_Global_Atomic,
      ETSOC_Memory_Read_Uncacheable, ETSOC_Memory_Read_Write_Cacheable };

/*! \var void memory_write
    \brief An array containing function pointers to ETSOC memory write functions.
    \warning Not thread safe!
*/
void (*memory_write[MEM_TYPES_COUNT])
    (const void *src_ptr, void *dest_ptr, uint32_t length) __attribute__((aligned(64))) =
    { ETSOC_Memory_Write_Local_Atomic, ETSOC_Memory_Write_Global_Atomic,
      ETSOC_Memory_Write_Uncacheable, ETSOC_Memory_Read_Write_Cacheable };

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
            uint32_t buffer_length, uint32_t flags)
{
    circ_buff_cb_t circ_buff;

    /* Reset the head and tail offsets */
    circ_buff.head_offset = 0;
    circ_buff.tail_offset = 0;

    /* Set the buffer length */
    circ_buff.length = buffer_length;

    /* Write the circular buffer CB to memory */
    (*memory_write[flags]) (&circ_buff, circ_buff_cb_ptr, sizeof(circ_buff));

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
int8_t Circbuffer_Push(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
            const void *restrict src_buffer, uint32_t src_length, uint32_t flags)
{
    int8_t status = CIRCBUFF_OPERATION_SUCCESS;
    const uint8_t *src_u8 = (const uint8_t *)src_buffer;
    circ_buff_cb_t circ_buff;

    /* Read the circular buffer CB from memory */
    (*memory_read[flags]) (circ_buff_cb_ptr, &circ_buff, sizeof(circ_buff));

    /* Verify the available space in circular buffer */
    if (Circbuffer_Get_Avail_Space(&circ_buff, CIRCBUFF_FLAG_NO_READ) >= src_length)
    {
        /* Verify the head offset */
        if (circ_buff.head_offset >= circ_buff.length)
        {
            status = CIRCBUFF_ERROR_BAD_HEAD_INDEX;
        }
    }
    else
    {
        status = CIRCBUFF_ERROR_FULL;
    }

    /* If previous operations are successful */
    if (status == CIRCBUFF_OPERATION_SUCCESS)
    {
        /* Check if buffer wrap is required */
        if (circ_buff.head_offset + src_length > circ_buff.length)
        {
            uint32_t bytes_till_end = circ_buff.length - circ_buff.head_offset;

            (*memory_write[flags]) (src_u8,
                (void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.head_offset], bytes_till_end);
            circ_buff.head_offset = 0;
            src_length -= bytes_till_end;
            src_u8 += bytes_till_end;
        }

        (*memory_write[flags]) (src_u8,
            (void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.head_offset], src_length);
        circ_buff.head_offset = (circ_buff.head_offset + src_length) % circ_buff.length;

        /* Update the head offset */
        (*memory_write[flags]) ((void*)&(circ_buff.head_offset),
            (void*)&circ_buff_cb_ptr->head_offset, sizeof(circ_buff.head_offset));
    }

    return status;
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
int8_t Circbuffer_Pop(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
            void *restrict const dest_buffer, uint32_t dest_length, uint32_t flags)
{
    int8_t status = CIRCBUFF_OPERATION_SUCCESS;
    uint8_t *dest_u8 = (uint8_t *)dest_buffer;
    circ_buff_cb_t circ_buff;
    uint32_t used_space;

    /* Read the circular buffer CB from memory */
    (*memory_read[flags]) (circ_buff_cb_ptr, &circ_buff, sizeof(circ_buff));

    /* Get the used space in circular buffer */
    used_space = Circbuffer_Get_Used_Space(&circ_buff, CIRCBUFF_FLAG_NO_READ);

    /* Verify if circular buffer has some data */
    if (used_space > 0)
    {
        /* Verify the length of data requested to read */
        if (dest_length > used_space)
        {
            status = CIRCBUFF_ERROR_BAD_LENGTH;
        }
    }
    else
    {
        status = CIRCBUFF_ERROR_EMPTY;
    }

    /* Verify the circular buffer tail offset */
    if ((status == CIRCBUFF_OPERATION_SUCCESS) &&
        (circ_buff.tail_offset >= circ_buff.length))
    {
        status = CIRCBUFF_ERROR_BAD_TAIL_INDEX;
    }

    /* If previous operations are successful */
    if (status == CIRCBUFF_OPERATION_SUCCESS)
    {
        /* Check if buffer wrap is required */
        if (circ_buff.tail_offset + dest_length > circ_buff.length)
        {
            uint32_t bytes_till_end = circ_buff.length - circ_buff.tail_offset;

            (*memory_read[flags]) ((void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.tail_offset],
                dest_u8, bytes_till_end);
            circ_buff.tail_offset = 0;
            dest_length -= bytes_till_end;
            dest_u8 += bytes_till_end;
        }

        (*memory_read[flags]) ((void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.tail_offset],
            dest_u8, dest_length);
        circ_buff.tail_offset = (circ_buff.tail_offset + dest_length) % circ_buff.length;

        /* Update the tail offset */
        (*memory_write[flags]) ((void*)&(circ_buff.tail_offset),
            (void*)&circ_buff_cb_ptr->tail_offset, sizeof(circ_buff.tail_offset));
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Circbuffer_Read
*
*   DESCRIPTION
*
*       This function reads the given number of bytes from circular buffer
*       to the given destination data buffer and increments the tail
*       offset.
*
*   INPUTS
*
*       circ_buff_cb_ptr  Pointer to circular buffer control block.
*       src_circ_buffer   Pointer to source circular buffer data pointer.
*       dest_buffer       Pointer to destination data buffer.
*       dest_length       Length of the destination data buffer in bytes.
*       flags             Additional flags to determine memory type
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t Circbuffer_Read(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
    void *restrict const src_circ_buffer, void *restrict const dest_buffer,
    uint32_t dest_length, uint32_t flags)
{
    int8_t status = CIRCBUFF_OPERATION_SUCCESS;
    uint8_t *src_u8 = (uint8_t *)src_circ_buffer;
    uint8_t *dest_u8 = (uint8_t *)dest_buffer;

    /* Verify the circular buffer tail offset */
    if (circ_buff_cb_ptr->tail_offset >= circ_buff_cb_ptr->length)
    {
        status = CIRCBUFF_ERROR_BAD_TAIL_INDEX;
    }

    /* If previous operations are successful */
    if (status == CIRCBUFF_OPERATION_SUCCESS)
    {
        /* Check if buffer wrap is required */
        if (circ_buff_cb_ptr->tail_offset + dest_length > circ_buff_cb_ptr->length)
        {
            uint32_t bytes_till_end = circ_buff_cb_ptr->length - circ_buff_cb_ptr->tail_offset;

            (*memory_read[flags]) (src_u8 + circ_buff_cb_ptr->tail_offset,
                dest_u8, bytes_till_end);
            circ_buff_cb_ptr->tail_offset = 0;
            dest_length -= bytes_till_end;
            dest_u8 += bytes_till_end;
        }

        (*memory_read[flags]) (src_u8 + circ_buff_cb_ptr->tail_offset,
            dest_u8, dest_length);

        /* Update tail offset value */
        circ_buff_cb_ptr->tail_offset =
            (circ_buff_cb_ptr->tail_offset + dest_length) % circ_buff_cb_ptr->length;
    }

    return status;
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
int8_t Circbuffer_Peek(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
        void *restrict const dest_buffer, uint32_t peek_offset,
        uint32_t peek_length, uint32_t flags)
{
    int8_t status = CIRCBUFF_OPERATION_SUCCESS;
    uint8_t *dest_u8 = (uint8_t *)dest_buffer;
    circ_buff_cb_t circ_buff;
    uint32_t used_space;

    /* Read the circular buffer CB from memory */
    (*memory_read[flags]) (circ_buff_cb_ptr, &circ_buff, sizeof(circ_buff));

    /* Get the used space in circular buffer */
    used_space = Circbuffer_Get_Used_Space(&circ_buff, CIRCBUFF_FLAG_NO_READ);

    /* Verify if circular buffer has some data */
    if (used_space > 0)
    {
        /* Verify the length and offset of data requested to peek */
        if ((peek_length + peek_offset) > used_space)
        {
            status = CIRCBUFF_ERROR_BAD_LENGTH;
        }
    }
    else
    {
        status = CIRCBUFF_ERROR_EMPTY;
    }

    /* Verify the circular buffer tail offset */
    if ((status == CIRCBUFF_OPERATION_SUCCESS) &&
        (circ_buff.tail_offset >= circ_buff.length))
    {
        status = CIRCBUFF_ERROR_BAD_TAIL_INDEX;
    }

    /* If previous operations are successful */
    if (status == CIRCBUFF_OPERATION_SUCCESS)
    {
        /* Check if buffer wrap is required */
        if (circ_buff.tail_offset + (peek_length + peek_offset) > circ_buff.length)
        {
            uint32_t bytes_till_end = circ_buff.length - (circ_buff.tail_offset + peek_offset);

            (*memory_read[flags]) (
                (void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.tail_offset + peek_offset],
                dest_u8, bytes_till_end);
            circ_buff.tail_offset = 0;
            peek_offset = 0;
            peek_length -= bytes_till_end;
            dest_u8 += bytes_till_end;
        }

        (*memory_read[flags]) (
            (void*)&circ_buff_cb_ptr->buffer_ptr[circ_buff.tail_offset + peek_offset],
            dest_u8, peek_length);
    }

    return status;
}
