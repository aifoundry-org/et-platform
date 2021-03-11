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

#include <common_defs.h>

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
    uint32_t head_offset;   /**< Offset of the circular buffer to write data to */
    uint32_t tail_offset;   /**< Offset of the circular buffer to read data from */
    uint32_t length;        /**< Total length (in bytes) of the circular buffer */
    uint32_t pad;           /**< Padding to make 64-bit aligned */
    uint8_t buffer_ptr[];   /**< Flexible array to access circular buffer memory
                            located just after circ_buff_cb_t */
} circ_buff_cb_t;

/*! \fn int8_t Circbuffer_Init(circ_buff_cb_t *circ_buff_cb_ptr, uint32_t buffer_length,
        uint32_t flags)
    \brief Initializes circular buffer instance.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] buffer_length: Length of the circular data buffer in bytes.
    \param [in] flags: Indicates memory access type
    \returns Status of the initialization process.
*/
int8_t Circbuffer_Init(circ_buff_cb_t *circ_buff_cb_ptr, uint32_t buffer_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Push(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr,
        const void *restrict const src_buffer, uint32_t src_length, uint32_t flags)
    \brief Pushes the data from source data buffer to the circular buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] src_buffer: Pointer to the source data buffer.
    \param [in] src_length: Total length (in bytes) of the data that needs to pushed.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is pushed or a negative error code in case of error.
*/
int8_t Circbuffer_Push(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
            const void *restrict const src_buffer, uint32_t src_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Pop(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr,
        void *restrict const dest_buffer, uint32_t dest_length , uint32_t flags)
    \brief Pops the data from circular buffer to the destination buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] dest_buffer: Pointer to the destination data buffer.
    \param [in] dest_length: Total length (in bytes) of the data that needs to poped.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is poped or a negative error code in case of error.
*/
int8_t Circbuffer_Pop(circ_buff_cb_t *restrict const circ_buff_cb_ptr,
            void *restrict const dest_buffer, uint32_t dest_length, uint32_t flags);

/*! \fn int8_t Circbuffer_Peek(volatile circ_buff_cb_t *restrict const circbuffer_ptr,
        void *restrict const dest_buffer, uint32_t peek_offset, uint32_t peek_length,
        uint32_t flags)
    \brief Peeks the circular buffer and returns the required data.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] dest_buffer: Pointer to the destination data buffer.
    \param [in] peek_length: Total length (in bytes) of the data that needs to be peeked.
    \param [in] flags: Indicates memory access type
    \returns Success status if the data is read or a negative error code in case of error.
*/
int8_t Circbuffer_Peek(circ_buff_cb_t *restrict const circbuffer_ptr,
            void *restrict const dest_buffer, uint32_t peek_offset, uint32_t peek_length,
            uint32_t flags);

/*! \fn uint32_t Circbuffer_Get_Avail_Space(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr,
        uint32_t flags)
    \brief Returns the number of available bytes in the circular buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
    \returns Free space in bytes.
*/
uint32_t Circbuffer_Get_Avail_Space(circ_buff_cb_t *restrict const circ_buff_cb_ptr, uint32_t flags);

/*! \fn uint32_t Circbuffer_Get_Used_Space(volatile circ_buff_cb_t *restrict const circ_buff_cb_ptr,
        uint32_t flags)
    \brief Returns the number of used bytes in the circular buffer.
    \param [in] circ_buff_cb_ptr: Pointer to circular buffer control block.
    \param [in] flags: Indicates memory access type
    \returns Used space in bytes.
*/
uint32_t Circbuffer_Get_Used_Space(circ_buff_cb_t *restrict const circ_buff_cb_ptr, uint32_t flags);

#endif /* CIRCBUFF_H */
