/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef VQUEUE_COMMON_H
#define VQUEUE_COMMON_H

#include "circbuffer_common.h"

#include <stdint.h>

#define VQUEUE_MAGIC              0xBEEF
#define VQUEUE_BUFFER_HEADER_SIZE sizeof(struct vqueue_buf_header)

// Error codes
#define VQ_ERROR_POP       (CIRCBUFFER_ERROR_END - 1)
#define VQ_ERROR_PUSH      (CIRCBUFFER_ERROR_END - 2)
#define VQ_ERROR_VQ_EMPTY  (CIRCBUFFER_ERROR_END - 3)
#define VQ_ERROR_VQ_FULL   (CIRCBUFFER_ERROR_END - 4)
#define VQ_ERROR_SQ_FULL   (CIRCBUFFER_ERROR_END - 5)
#define VQ_ERROR_CQ_FULL   (CIRCBUFFER_ERROR_END - 6)
#define VQ_ERROR_NOT_READY (CIRCBUFFER_ERROR_END - 7)

// Data structures
/// \brief Enum for the type of Virtual Queue.
typedef enum {
    SQ = 0,
    CQ,
} vq_e;

/// \brief Status of each Virtual Queue.
typedef enum {
    VQ_STATUS_NOT_READY = 0U,
    VQ_STATUS_READY = 1U,
    VQ_STATUS_WAITING = 2U,
    VQ_STATUS_ERROR = 3U
} vq_status_e;

// TODO: SW-5261: Remove this structure once SysEMU moves to reading device interface regs. Its just kept to pass the build.
/// \brief Virtual Queue descriptor structure which holds the information
/// required by the Host to initialize the Virtual Queues. This structure
/// is shared with the Host.
struct vqueue_desc {
    uint8_t device_ready;
    uint8_t host_ready;
    uint8_t queue_count;
    uint16_t queue_element_count;
    uint16_t queue_element_size;
    uint64_t queue_addr;
} __attribute__((__packed__));

/// \brief The header which is placed in the start of each buffer of a virtual
/// queue. This strucutre holds the necessary info required for reading a valid buffer.
struct vqueue_buf_header {
    uint16_t length;
    uint16_t magic;
};

/// \brief This is a shared Virtual Queue info struture with Host and is placed in shared
/// memory. It holds the information of each virtual queue and is a combination of both
/// submission queue and completion queue. The both host and device operate on this shared structure.
struct vqueue_info {
    uint8_t master_status;
    uint8_t slave_status;
    struct circ_buf_header sq_header;
    struct circ_buf_header cq_header;
} __attribute__((__packed__));

#endif
