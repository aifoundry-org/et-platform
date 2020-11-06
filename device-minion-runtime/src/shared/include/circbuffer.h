/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef CIRCBUFFER_H
#define CIRCBUFFER_H

#include "circbuffer_common.h"

/// \brief Initializes the circular buffer related elements present in struct circ_buf_header.
/// \param[in] circbuffer_ptr: Pointer to circular buffer header structure.
/// \param[in] buffer_ptr: Pointer to the data buffer for the current circular buffer.
/// \param[in] buffer_length: Length of the data buffer in bytes
void CIRCBUFFER_init(volatile struct circ_buf_header *const circbuffer_ptr, void *buffer_ptr,
                     uint64_t buffer_length);

/// \brief Reads the number of data bytes from source buffer to the destination buffer.
/// \param[in] src_buffer: Pointer to the source data buffer.
/// \param[in] src_buffer_offset: Offset of the source buffer from where the data should be read.
/// \param[in] src_buffer_length: Total length (in bytes) of the source data buffer.
/// \param[in] dest_buffer: Pointer to the destination data buffer.
/// \param[in] dest_buffer_length: Total length (in bytes) of the destination data buffer.
/// \returns Length of the data read in destination buffer or a negative error code in case of error.
int64_t CIRCBUFFER_read(void *restrict const src_buffer, uint64_t src_buffer_offset,
                        uint64_t src_buffer_length, void *restrict const dest_buffer,
                        uint64_t dest_buffer_length);

/// \brief Writes the number of data bytes from source buffer to the destination buffer.
/// \param[in] dest_buffer: Pointer to the destination data buffer.
/// \param[in] dest_buffer_offset: Offset of the destination buffer where the data should be written.
/// \param[in] dest_buffer_length: Total length (in bytes) of the destination data buffer.
/// \param[in] src_buffer: Pointer to the source data buffer.
/// \param[in] src_buffer_length: Total length (in bytes) of the source data buffer.
/// \returns Length of the data written in destination buffer or a negative error code in case of error.
int64_t CIRCBUFFER_write(void *restrict const dest_buffer, uint64_t dest_buffer_offset,
                         uint64_t dest_buffer_length, const void *restrict const src_buffer,
                         uint64_t src_buffer_length);

/// \brief Pushes the data from source data buffer to the destination circular buffer and increments the
/// head pointer of circular buffer.
/// \param[in] circbuffer_ptr: Pointer to the structure containing circular buffer header information.
/// \param[in] dest_buffer: Pointer to the destination data buffer.
/// \param[in] src_buffer: Pointer to the source data buffer.
/// \param[in] length: Total length (in bytes) of the data that needs to pushed.
/// \returns Length of the data written in destination buffer or a negative error code in case of error.
int64_t CIRCBUFFER_push(volatile struct circ_buf_header *restrict const circbuffer_ptr,
                        void *restrict const dest_buffer, const void *restrict const src_buffer,
                        uint32_t length);

/// \brief Pops the data from source data buffer to the destination circular buffer and increments the
/// tail pointer of circular buffer.
/// \param[in] circbuffer_ptr: Pointer to the structure containing circular buffer header information.
/// \param[in] dest_buffer: Pointer to the destination data buffer.
/// \param[in] src_buffer: Pointer to the source data buffer.
/// \param[in] length: Total length (in bytes) of the data that needs to poped.
/// \returns Length of the data written in destination buffer or a negative error code in case of error.
int64_t CIRCBUFFER_pop(volatile struct circ_buf_header *restrict const circbuffer_ptr,
                       const void *restrict const src_buffer, void *restrict const dest_buffer,
                       uint32_t length);

/// \brief Returns the number of available buffers in the given circular buffer.
/// \param[in] head_index: Value of head index.
/// \param[in] tail_index: Value of tail index.
/// \returns Number of available buffers.
inline uint32_t CIRCBUFFER_free(const uint32_t head_index, const uint32_t tail_index)
{
    return (uint32_t)(head_index >= tail_index) ?
               (CIRCBUFFER_COUNT - 1) - (head_index - tail_index) :
               tail_index - head_index - 1;
}

/// \brief Returns the number of used/occupied buffers in the given circular buffer.
/// \param[in] head_index: Value of head index.
/// \param[in] tail_index: Value of tail index.
/// \returns Number of used/occupied buffers.
inline uint32_t CIRCBUFFER_used(const uint32_t head_index, const uint32_t tail_index)
{
    return (uint32_t)(head_index >= tail_index) ? head_index - tail_index :
                                                  (CIRCBUFFER_COUNT + head_index - tail_index);
}

#endif
