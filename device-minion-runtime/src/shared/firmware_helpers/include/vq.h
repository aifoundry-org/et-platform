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
#include "device-common/atomic.h"

/**
 * @brief Macros for Virtual Queues related error codes.
 */
#define VQ_ERROR_INVLD_CMD_SIZE       (CIRCBUFF_ERROR_END - 1)
#define VQ_ERROR_BAD_PAYLOAD_LENGTH   (CIRCBUFF_ERROR_END - 2)
#define VQ_ERROR_BAD_TARGET           (CIRCBUFF_ERROR_END - 3)

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

/*! \fn int32_t VQ_Pop_Optimized(vq_cb_t* vq_cb, uint32_t vq_used_space,
    void *const shared_mem_ptr, void* rx_buff)
    \brief Pops a command from a virtual queue.
    \param vq_cb Pointer to virtual queue control block.
    \param vq_used_space Number of bytes used in VQ
    \param shared_mem_ptr Pointer to the VQ shared memory buffer used as
    circular buffer
    \param rx_buff Pointer to rx command buffer.
    \return The size of the command in bytes or negative error code.
*/
int32_t VQ_Pop_Optimized(vq_cb_t* vq_cb, uint64_t vq_used_space,
    void *const shared_mem_ptr, void* rx_buff);

/*! \fn int32_t VQ_Prefetch_Buffer(vq_cb_t* vq_cb, uint32_t vq_used_space,
    void *const shared_mem_ptr, void* rx_buff)
    \brief Prefetches the data from a virtual queue.
    \param vq_cb Pointer to virtual queue control block.
    \param vq_used_space Number of bytes used in VQ
    \param shared_mem_ptr Pointer to the VQ shared memory buffer used as
    circular buffer
    \param rx_buff Pointer to rx command buffer.
    \return Success status or negative error code.
*/
int8_t VQ_Prefetch_Buffer(vq_cb_t* vq_cb, uint64_t vq_used_space,
    void *const shared_mem_ptr, void* rx_buff);

/*! \fn int32_t VQ_Process_Command(void* cmds_buff, uint64_t buffer_size, uint32_t buffer_idx)
    \brief This function is used to process a popped command from prefetched VQ buffer.
    \param cmds_buff Pointer to rx command buffer.
    \param buffer_size Number of bytes used in VQ
    \param buffer_idx Index of the commands buffer from where the data is to be read
    \return The size of the command in bytes or negative error code.
*/
int32_t VQ_Process_Command(void* cmds_buff, uint64_t buffer_size, uint32_t buffer_idx);

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

/*! \fn static inline uint64_t VQ_Get_Tail_Offset(const vq_cb_t* vq_cb)
    \brief Get the tail offset of the VQ
    \param vq_cb Pointer to virtual queue control block.
    \return Value of tail offset
*/
static inline uint64_t VQ_Get_Tail_Offset(const vq_cb_t* vq_cb)
{
    return (vq_cb->circbuff_cb->tail_offset);
}

/*! \fn static inline uint64_t VQ_Get_Head_Offset(const vq_cb_t* vq_cb)
    \brief Get the head offset of the VQ
    \param vq_cb Pointer to virtual queue control block.
    \return Value of head offset
*/
static inline uint64_t VQ_Get_Head_Offset(const vq_cb_t* vq_cb)
{
    return (vq_cb->circbuff_cb->head_offset);
}

/*! \fn static inline void VQ_Set_Tail_Offset(vq_cb_t* dest_vq_cb, uint64_t tail_val)
    \brief Set the tail offset of the VQ
    \param vq_cb Pointer to destination virtual queue control block.
*/
static inline void VQ_Set_Tail_Offset(vq_cb_t* dest_vq_cb, uint64_t tail_val)
{
#if defined(MASTER_MINION)
    Circbuffer_Set_Tail((circ_buff_cb_t*)(uintptr_t)
        atomic_load_local_64((uint64_t*)(void*)&dest_vq_cb->circbuff_cb), tail_val,
        atomic_load_local_32(&dest_vq_cb->flags));
#else
    Circbuffer_Set_Tail(dest_vq_cb->circbuff_cb, tail_val, dest_vq_cb->flags);
#endif
}

/*! \fn static inline void VQ_Get_Head_And_Tail(vq_cb_t* src_vq_cb, vq_cb_t* dest_vq_cb)
    \brief Get the head and tail values from the source VQ CB to destination VQ CB
    \param src_vq_cb Pointer to source virtual queue control block.
    \param dest_vq_cb Pointer to destination virtual queue control block.
*/
static inline void VQ_Get_Head_And_Tail(vq_cb_t* src_vq_cb, vq_cb_t* dest_vq_cb)
{
    Circbuffer_Get_Head_Tail(
#if defined(MASTER_MINION)
        (circ_buff_cb_t*)(uintptr_t)atomic_load_local_64((uint64_t*)&src_vq_cb->circbuff_cb),
#else
        src_vq_cb->circbuff_cb,
#endif
        dest_vq_cb->circbuff_cb, dest_vq_cb->flags);
}

/*! \fn static inline uint64_t VQ_Get_Used_Space(vq_cb_t* vq_cb, uint32_t flags)
    \brief Get the used space (in bytes) in the VQ
    \param vq_cb Pointer to virtual queue control block.
    \param flags Extra flags passed to determine memory operations
    \return Number of bytes available in VQ
*/
static inline uint64_t VQ_Get_Used_Space(const vq_cb_t* vq_cb, uint32_t flags)
{
    return Circbuffer_Get_Used_Space(vq_cb->circbuff_cb, flags);
}

#endif /* __VQ_H__ */
