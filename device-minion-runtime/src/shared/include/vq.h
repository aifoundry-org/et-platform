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
/*! \file vq.h
    \brief Header/Interface to access Virtual Queue services. This
    abstraction is simply a wrapper for circular buffer data structure.
    It associates a shared memory address to the circular buffer data
    structure.
*/
/***********************************************************************/
#ifndef __VQ_H__
#define __VQ_H__

#include "common_defs.h"
#include "circbuff.h"

/**
 * @brief Macros for Virtual Queues related error codes.
 */
#define VQ_ERROR_INVLD_CMD_SIZE       (CIRCBUFF_ERROR_END - 1)

/*! \def VQ_CIRCBUFF_BASE_ADDR(base, idx, size)
    \brief Macro to return circbuff's base address.
*/
#define VQ_CIRCBUFF_BASE_ADDR(base,idx,size)   (base + (idx * size))

/*! \struct vq_cb_t
    \brief Virtual queues control block.
*/
typedef struct vq_cb_
{
    circ_buff_cb_t *circbuff_cb; /**< Pointer to circular buffer control block */
    uint16_t cmd_size_peek_offset; /**< Offset to peek */
    uint16_t cmd_size_peek_length; /**< Length starting from offset to peek */
    uint32_t flags; /**< Memory type attr to access circular buffer
                    associated with the virtual queue */
} vq_cb_t;

/*! \struct iface_cb_t;
    \brief Generic interface control block
*/
typedef struct iface_cb_ {
    uint32_t vqueue_base; /* This is a 32 bit offset from 64 dram base */
    uint32_t vqueue_size;
    vq_cb_t vqueue;
} iface_cb_t;

/*! \fn int8_t VQ_Init(vq_cb_t* vq_cb, uint64_t vq_base, uint32_t vq_size,
    uint16_t cmd_size_peek_offset, uint16_t cmd_size_peek_length)
    \brief Initialize the virtual queue instance
    \param vq_cb Pointer to virtual queue control block
    \param vq_base Base address for submission queue buffers.
    \param vq_size Size of each submission queue in bytes.
    \param cmd_size_peek_offset Base offset to be used for peek.
    \param cmd_size_peek_length Length of command should be peeked.
    \param flags Memory type to drive access attributes.
    \return Status indicating success or negative error code
*/
int8_t VQ_Init(vq_cb_t* vq_cb, uint64_t vq_base, uint32_t vq_size,
    uint16_t cmd_size_peek_offset, uint16_t cmd_size_peek_length, uint32_t flags);

/*! \fn int8_t VQ_Push(vq_cb_t* vq_cb, void* data, uint32_t data_size)
    \brief Push the command to circular buffer associated with
    vq_cb_t.
    \param vq_cb Pointer to virtual queue control block.
    \param data Pointer to data buffer.
    \param data_size Size of the data tp push in bytes.
    \return Status indicating success or negative error code.
*/
int8_t VQ_Push(vq_cb_t* vq_cb, void* data, uint32_t data_size);

/*! \fn int32_t VQ_Pop(vq_cb_t* vq_cb, void* rx_buff)
    \brief Pops a command from a virtual queue.
    \param vq_cb Pointer to virtual queue control block.
    \param rx_buff Pointer to rx command buffer.
    Caller shall use MM_CMD_MAX_SIZE to allocate rx_buff
    \return The size of the command in bytes, zero for no data
    or negative error code.
*/
int32_t VQ_Pop(vq_cb_t* vq_cb, void* rx_buff);

/*! \fn int8_t VQ_Peek(vq_cb_t* vq_cb, void* peek_buff,
        uint16_t peek_offset, uint16_t peek_length)
    \brief Peek into a segment in the virtual queue
    \param peek_buff Pointer to peek buffer.
    \param peek_length Length of bytes to peek.
    \return Status indicating sucess or negative error
*/
int8_t VQ_Peek(vq_cb_t* vq_cb, void* peek_buff, uint16_t peek_offset,
        uint16_t peek_length);

/*! \fn bool VQ_Data_Avail(vq_cb_t* vq_cb)
    \brief Check if data available in VQ
    \param vq_cb Pointer to virtual queue control block.
    \return Boolean indicating data available to process
*/
bool VQ_Data_Avail(vq_cb_t* vq_cb);

/*! \fn int8_t VQ_Deinit(void)
    \brief Deinitializes the virtual queues.
    \returns Status indicating success or negative error code
*/
int8_t VQ_Deinit(void);

#endif /* __VQ_H__ */
