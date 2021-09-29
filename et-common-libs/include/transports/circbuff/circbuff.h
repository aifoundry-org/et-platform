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
/*! \file circbuff.h
    \brief A C header that defines the Circular Buffer related data
    structures and defines.
*/
/***********************************************************************/
#ifndef CIRCBUFF_H
#define CIRCBUFF_H
#include "etsoc/common/common_defs.h"
#include "etsoc/isa/etsoc_memory.h"

/**
 * @brief Defines for Circular Buffer status codes.
 */
#define CIRCBUFF_OPERATION_SUCCESS     0
#define CIRCBUFF_ERROR_BAD_LENGTH     -1
#define CIRCBUFF_ERROR_BAD_HEAD_INDEX -2
#define CIRCBUFF_ERROR_BAD_TAIL_INDEX -3
#define CIRCBUFF_ERROR_FULL           -4
#define CIRCBUFF_ERROR_EMPTY          -5
#define CIRCBUFF_ERROR_END            -6 /* Indicates end of status codes for circbuff */

/**
 * @brief Reserved defines for Circular Buffer flags field.
 */
#define CIRCBUFF_FLAG_NO_READ          (1U << 31)
#define CIRCBUFF_FLAG_NO_WRITE         (1U << 30)

/*! \struct circ_buff_cb_t
    \brief The circular buffer control block which holds the
    information of a circular buffer.
*/
typedef struct __attribute__((__packed__)) circ_buff_cb {
    uint64_t head_offset;   /**< Offset of the circular buffer to write data to */
    uint64_t tail_offset;   /**< Offset of the circular buffer to read data from */
    uint64_t length;        /**< Total length (in bytes) of the circular buffer */
    uint64_t pad;           /**< Padding to make the struct 32-bytes aligned */
    uint8_t buffer_ptr[];   /**< Flexible array to access circular buffer memory
                            located just after circ_buff_cb_t */
} circ_buff_cb_t;

/*! \fn int8_t Circbuffer_Init(circ_buff_cb_t *circ_buff_cb_ptr, uint64_t buffer_length,
    uint32_t flags)
    \brief Initializes circular buffer instance.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] buffer_length: Length of the circular data buffer in bytes.
    \param [in] flags: Indicates memory access type
    \returns Status of the initialization process.
*/
int8_t Circbuffer_Init(circ_buff_cb_t *circ_buff_cb_ptr, uint64_t buffer_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Push(volatile circ_buff_cb_t *const circ_buff_cb_ptr,
    const void *const src_buffer, uint64_t src_length, uint32_t flags)
    \brief Pushes the data from source data buffer to the circular buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] src_buffer: Pointer to the source data buffer.
    \param [in] src_length: Total length (in bytes) of the data that needs to pushed.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is pushed or a negative error code in case of error.
*/
int8_t Circbuffer_Push(circ_buff_cb_t *const circ_buff_cb_ptr,
    const void *const src_buffer, uint64_t src_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Pop(volatile circ_buff_cb_t *const circ_buff_cb_ptr,
    void *const dest_buffer, uint64_t dest_length , uint32_t flags)
    \brief Pops the data from circular buffer to the destination buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] dest_buffer: Pointer to the destination data buffer.
    \param [in] dest_length: Total length (in bytes) of the data that needs to poped.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is poped or a negative error code in case of error.
*/
int8_t Circbuffer_Pop(circ_buff_cb_t *const circ_buff_cb_ptr,
    void *const dest_buffer, uint64_t dest_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Read(circ_buff_cb_t *const circ_buff_cb_ptr,
    void *const src_circ_buffer, void *const dest_buffer,
    uint64_t dest_length, uint32_t flags)
    \brief Reads the data from circular buffer to the destination buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] src_circ_buffer: Pointer to source circular buffer data pointer.
    \param [in] dest_buffer: Pointer to the destination data buffer.
    \param [in] dest_length: Total length (in bytes) of the data that needs to poped.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is read or a negative error code in case of error.
*/
int8_t Circbuffer_Read(circ_buff_cb_t *const circ_buff_cb_ptr,
    void *const src_circ_buffer, void *const dest_buffer,
    uint64_t dest_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Peek(volatile circ_buff_cb_t *const circbuffer_ptr,
    void *const dest_buffer, uint64_t peek_offset, uint64_t peek_length, uint32_t flags)
    \brief Peeks the circular buffer and returns the required data.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] dest_buffer: Pointer to the destination data buffer.
    \param [in] peek_length: Total length (in bytes) of the data that needs to be peeked.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is read or a negative error code in case of error.
*/
int8_t Circbuffer_Peek(circ_buff_cb_t *const circbuffer_ptr,
    void *const dest_buffer, uint64_t peek_offset, uint64_t peek_length, uint32_t flags);

/*! \fn static inline uint64_t Circbuffer_Get_Avail_Space(volatile circ_buff_cb_t *const circ_buff_ptr,
    uint32_t flags)
    \brief Returns the number of available bytes in the circular buffer.
    \param [in] circ_buff_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
    \returns Free space in bytes.
*/
static inline uint64_t Circbuffer_Get_Avail_Space(const circ_buff_cb_t *circ_buff_ptr,
    uint32_t flags)
{
    /* Read from memory if no read flag is not set */
    if (flags != CIRCBUFF_FLAG_NO_READ)
    {
        circ_buff_cb_t circ_buff __attribute__((aligned(8)));

        /* Read the circular buffer CB from memory */
        ETSOC_Memory_Read(circ_buff_ptr, &circ_buff, sizeof(circ_buff), flags);

        return (uint64_t)((circ_buff.head_offset >= circ_buff.tail_offset) ?
                        (circ_buff.length - 1) - (circ_buff.head_offset - circ_buff.tail_offset) :
                        circ_buff.tail_offset - circ_buff.head_offset - 1);
    }
    else
    {
        return (uint64_t)((circ_buff_ptr->head_offset >= circ_buff_ptr->tail_offset) ?
                (circ_buff_ptr->length - 1) - (circ_buff_ptr->head_offset - circ_buff_ptr->tail_offset) :
                circ_buff_ptr->tail_offset - circ_buff_ptr->head_offset - 1);
    }
}

/*! \fn static inline uint64_t Circbuffer_Get_Used_Space(volatile circ_buff_cb_t *const circ_buff_ptr,
    uint32_t flags)
    \brief Returns the number of used bytes in the circular buffer.
    \param [in] circ_buff_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
    \returns Used space in bytes.
*/
static inline uint64_t Circbuffer_Get_Used_Space(const circ_buff_cb_t *circ_buff_ptr,
    uint32_t flags)
{
    /* Read from memory if no read flag is not set */
    if (flags != CIRCBUFF_FLAG_NO_READ)
    {
        circ_buff_cb_t circ_buff __attribute__((aligned(8)));

        /* Read the circular buffer CB from memory */
        ETSOC_Memory_Read(circ_buff_ptr, &circ_buff, sizeof(circ_buff), flags);

        return (uint64_t)((circ_buff.head_offset >= circ_buff.tail_offset) ?
                circ_buff.head_offset - circ_buff.tail_offset :
                (circ_buff.length + circ_buff.head_offset - circ_buff.tail_offset));
    }
    else
    {
        return (uint64_t)((circ_buff_ptr->head_offset >= circ_buff_ptr->tail_offset) ?
                circ_buff_ptr->head_offset - circ_buff_ptr->tail_offset :
                (circ_buff_ptr->length + circ_buff_ptr->head_offset - circ_buff_ptr->tail_offset));
    }
}

/*! \fn static inline void Circbuffer_Get_Head_Tail(circ_buff_cb_t *const src_circ_buff_cb_ptr,
    circ_buff_cb_t *restrict dest_circ_buff_cb_ptr, uint32_t flags)
    \brief Returns the head and tail offsets in destination circular buffer CB.
    \param [in] src_circ_buff_cb_ptr: Pointer to source circular buffer control block.
    \param [in] dest_circ_buff_cb_ptr: Pointer to destination circular buffer control block.
    \param [in] flags: Indicates memory access type
*/
static inline void Circbuffer_Get_Head_Tail(const circ_buff_cb_t *src_circ_buff_cb_ptr,
    circ_buff_cb_t *restrict dest_circ_buff_cb_ptr, uint32_t flags)
{
    /* Read the circular buffer CB from memory */
    ETSOC_Memory_Read(src_circ_buff_cb_ptr, dest_circ_buff_cb_ptr,
        sizeof(((circ_buff_cb_t*)0)->head_offset) + sizeof(((circ_buff_cb_t*)0)->tail_offset), flags);
}

/*! \fn static inline uint64_t Circbuffer_Get_Tail(const circ_buff_cb_t *circ_buff_cb_ptr,
    uint32_t flags)
    \brief Gets the tail offset value in circular buffer CB.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
*/
static inline uint64_t Circbuffer_Get_Tail(const circ_buff_cb_t *circ_buff_cb_ptr, uint32_t flags)
{
    uint64_t tail;

    /* Read the circular buffer CB from memory */
    ETSOC_Memory_Read(&circ_buff_cb_ptr->tail_offset, &tail, sizeof(tail), flags);

    return tail;
}

/*! \fn static inline void Circbuffer_Set_Tail(circ_buff_cb_t *restrict dest_circ_buff_cb_ptr,
    uint64_t tail_val, uint32_t flags)
    \brief Sets the tail offset value in provided destination circular buffer CB.
    \param [in] dest_circ_buff_cb_ptr: Pointer to destination circular buffer control block.
    \param [in] tail_val: Value of tail offset to write.
    \param [in] flags: Indicates memory access type
*/
static inline void Circbuffer_Set_Tail(circ_buff_cb_t *restrict dest_circ_buff_cb_ptr,
    uint64_t tail_val, uint32_t flags)
{
    /* Write the circular buffer CB from memory */
    ETSOC_Memory_Write((void*)&tail_val, (void*)&dest_circ_buff_cb_ptr->tail_offset,
        sizeof(tail_val), flags);
}

/*! \fn static inline uint64_t Circbuffer_Get_Head(const circ_buff_cb_t *circ_buff_cb_ptr,
    uint32_t flags)
    \brief Gets the head offset value in circular buffer CB.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
*/
static inline uint64_t Circbuffer_Get_Head(const circ_buff_cb_t *circ_buff_cb_ptr, uint32_t flags)
{
    uint64_t head;

    /* Read the circular buffer CB from memory */
    ETSOC_Memory_Read(&circ_buff_cb_ptr->head_offset, &head, sizeof(head), flags);

    return head;
}

/*! \fn static inline void Circbuffer_Set_Head(circ_buff_cb_t *restrict dest_circ_buff_cb_ptr,
    uint64_t head_val, uint32_t flags)
    \brief Sets the head offset value in provided destination circular buffer CB.
    \param [in] dest_circ_buff_cb_ptr: Pointer to destination circular buffer control block.
    \param [in] head_val: Value of head offset to write.
    \param [in] flags: Indicates memory access type
*/
static inline void Circbuffer_Set_Head(circ_buff_cb_t *restrict dest_circ_buff_cb_ptr,
    uint64_t head_val, uint32_t flags)
{
    /* Write the circular buffer CB from memory */
    ETSOC_Memory_Write(&head_val, &dest_circ_buff_cb_ptr->head_offset,
        sizeof(head_val), flags);
}

#endif /* CIRCBUFF_H */
