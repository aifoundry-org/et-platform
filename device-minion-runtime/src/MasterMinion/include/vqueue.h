/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef VQUEUE_H
#define VQUEUE_H

#include <stddef.h>
#include "vqueue_common.h"
#include "circbuffer.h"
#include "mm_dev_intf_reg.h"

// SQ0 for high priority. In case of only one VQ available, all commands go through SQ0
#define VQUEUE_SQ_HP_ID          0
#define VQUEUE_MM_COUNT_MAX      4 // compile-time config
#define VQUEUE_ELEMENT_COUNT     CIRCBUFFER_COUNT
#define VQUEUE_ELEMENT_SIZE      CIRCBUFFER_SIZE
#define VQUEUE_ELEMENT_ALIGNMENT 64U // Aligned to cache line size (applicable to DRAM)
#define VQUEUE_CONTROL_REGION_SIZE \
    256ULL // Contains all VQs information structures, i.e, struct vqueue_info
#define VQUEUE_DATA_BASE (DEVICE_MM_VQUEUE_BASE + VQUEUE_CONTROL_REGION_SIZE)

// Returns virtual queue control => (struct vqueue_info * MM_VQ_COUNT)
#define DEVICE_VQUEUE_BASE(queue_id) \
    ((void *)(DEVICE_MM_VQUEUE_BASE + sizeof(struct vqueue_info) * queue_id))

// Returns submission queue address associated with the given virtual queue
#define DEVICE_SQUEUE_BASE(queue_id, size) \
    ((void *)(VQUEUE_DATA_BASE + (2 * CACHE_LINE_ALIGN(size) * queue_id)))

// Returns completion queue address associated with the given virtual queue
#define DEVICE_CQUEUE_BASE(queue_id, size) \
    ((void *)(VQUEUE_DATA_BASE + (2 * CACHE_LINE_ALIGN(size) * queue_id) + CACHE_LINE_ALIGN(size)))

#ifndef __ASSEMBLER__
// Ensure the SQs + CQs size are witihn limits
static_assert(VQUEUE_DATA_BASE + (2 * MM_VQ_COUNT * (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE)) <
                  (DEVICE_MM_VQUEUE_BASE + DEVICE_MM_VQUEUE_MEM_SIZE),
              "MM Virtual Queues size not within limits.");
#endif

// Function prototypes
/// \brief Initializes all the Virtual Queue related information needed by the device. This also enables
/// the PCIe interrupts from Host.
void VQUEUE_init(void);

/// \brief Pushes the data from the given buffer to the destination virtual queue upto the specified length
/// and increments the head pointer.
/// \param[in] vq: Type of the virtual queue to which we need to interact.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \param[in] buffer_ptr: Pointer to the source data buffer.
/// \param[in] length: Total length (in bytes) of the data that needs to pushed.
/// \returns Length of the data written in virtual queue or a negative error code in case of error.
int64_t VQUEUE_push(vq_e vq, uint32_t vq_index, const void *const buffer_ptr, uint32_t length);

/// \brief Pops the data in the given buffer from the given virtual queue upto the specified length
/// and increments the tail pointer.
/// \param[in] vq: Type of the virtual queue to which we need to interact.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \param[in] buffer_ptr: Pointer to the destination data buffer.
/// \param[in] length: Total length (in bytes) of the data that needs to poped.
/// \returns Length of the data written in destination buffer or a negative error code in case of error.
int64_t VQUEUE_pop(vq_e vq, uint32_t vq_index, void *const buffer_ptr, size_t buffer_size);

/// \brief Updates the status of a Virtual Queue based on the status of Host.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
void VQUEUE_update_status(uint32_t vq_index);

/// \brief Used to determine if a Virtual Queue is empty.
/// \param[in] vq: Type of the virtual queue to which we need to interact.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \returns True if virtual queue is empty and false if not.
bool VQUEUE_empty(vq_e vq, uint32_t vq_index);

/// \brief Used to determine if a Virtual Queue is full.
/// \param[in] vq: Type of the virtual queue to which we need to interact.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \returns True if virtual queue is full and false if not.
bool VQUEUE_full(vq_e vq, uint32_t vq_index);

/// \brief Used to check if a VQ is ready or not
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \returns True if virtual queue is ready and false if not.
bool VQUEUE_ready(uint32_t vq_index);

#endif
