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
#include "sp_dev_intf_reg.h"

// SP SQ has only 1 Queue
#define SP_VQUEUE_INDEX           1U
#define SQ_TYPE                   0
// Place SP VQ in upper 2K of the PC-SP Mbox
#define DEVICE_SP_VQUEUE_BASE     (0x003000300 + 0x800UL)
#define DEVICE_SP_VQUEUE_MEM_SIZE  0x400ULL 
#define VQUEUE_ELEMENT_COUNT       CIRCBUFFER_COUNT
#define VQUEUE_ELEMENT_SIZE        CIRCBUFFER_SIZE
#define VQUEUE_CONTROL_REGION_SIZE \
    256ULL // Contains all VQs information structures, i.e, struct vqueue_info
#define VQUEUE_DATA_BASE (DEVICE_SP_VQUEUE_BASE + VQUEUE_CONTROL_REGION_SIZE)

// Returns virtual queue control => (struct vqueue_info)
#define DEVICE_VQUEUE_BASE \
    ((void *)(DEVICE_SP_VQUEUE_BASE + sizeof(struct vqueue_info)))

// Returns submission queue address associated with the given virtual queue
#define DEVICE_SQUEUE_BASE(size) \
    ((void *)(VQUEUE_DATA_BASE + (2 * size)))

// Returns completion queue address associated with the given virtual queue
#define DEVICE_CQUEUE_BASE(size) \
    ((void *)(VQUEUE_DATA_BASE + (2 * size) + size))

#ifndef __ASSEMBLER__
// Ensure the SQs + CQs size are witihn limits
static_assert(VQUEUE_DATA_BASE + (2 * (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE)) <
             (DEVICE_SP_VQUEUE_BASE + DEVICE_SP_VQUEUE_MEM_SIZE),
              "SP Virtual Queues size not within limits.");
#endif

// Data structures
typedef struct {
    uint16_t producer_lock;
    uint16_t consumer_lock;
    uint32_t notify_int;
} vqueue_info_intern_t;

// Function prototypes
/// \brief Initializes all the Virtual Queue related information needed by the device. This also enables
/// the PCIe interrupts from Host.
void VQUEUE_init(void);

/// \brief Pops the data in the given buffer from the given virtual queue upto the specified length
/// and increments the tail pointer.
/// \param[in] vq: Type of the virtual queue to which we need to interact.
/// \param[in] vq_index: Index of the virtual queue (starting from zero).
/// \param[in] buffer_ptr: Pointer to the destination data buffer.
/// \param[in] length: Total length (in bytes) of the data that needs to poped.
/// \returns Length of the data written in destination buffer or a negative error code in case of error.
int64_t VQUEUE_pop(vq_e vq, uint32_t vq_index, void *const buffer_ptr, size_t buffer_size);

#endif
