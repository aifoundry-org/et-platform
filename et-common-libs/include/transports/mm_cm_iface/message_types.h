/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file message_types.h
    \brief A C header that defines Messages between MM and CM.
*/
/***********************************************************************/

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Common definitions
 */

typedef uint8_t cm_iface_message_id_t;
typedef uint8_t cm_iface_message_number_t;

typedef struct {
    union {
        struct {
            cm_iface_message_number_t number;
            cm_iface_message_id_t id;
            uint16_t tag_id;
        };
        uint32_t raw_header;
    };
    uint8_t pad[4]; /* Padding to make struct 64-bit aligned */
} cm_iface_message_header_t;

#define MESSAGE_MAX_PAYLOAD_SIZE (64 - sizeof(cm_iface_message_header_t))

#define ASSERT_CACHE_LINE_CONSTRAINTS(type)                                  \
    static_assert(sizeof(type) == 64, "sizeof(" #type ") must be 64 bytes"); \
    static_assert(_Alignof(type) == 64, "_Alignof(" #type ") must be 64 bytes")

typedef struct {
    cm_iface_message_header_t header;
    uint8_t data[MESSAGE_MAX_PAYLOAD_SIZE];
} __attribute__((packed, aligned(64))) cm_iface_message_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_iface_message_t);

/*
 * Broadcast/Multicast message
 */

typedef struct {
    uint64_t shire_mask;       /* Bit mask of shires that need to ack back */
    uint32_t sender_thread_id; /* MM thread ID of the multicast sender */
} __attribute__((aligned(64))) broadcast_message_ctrl_t;

ASSERT_CACHE_LINE_CONSTRAINTS(broadcast_message_ctrl_t);

/*
 * MM to CM messages
 */

typedef enum {
    MM_TO_CM_MESSAGE_ID_NONE = 0,
    MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH,
    MM_TO_CM_MESSAGE_ID_KERNEL_ABORT,
    MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL,
    MM_TO_CM_MESSAGE_ID_TRACE_CONFIGURE,
    MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT,
    MM_TO_CM_MESSAGE_ID_PMC_CONFIGURE
} mm_to_cm_message_id_e;

#define KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH      (1u << 0)
#define KERNEL_LAUNCH_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE (1u << 1)

typedef struct {
    uint64_t code_start_address;
    uint64_t pointer_to_args;
    uint64_t shire_mask;
    uint64_t exception_buffer;
    uint8_t kw_base_id;
    uint8_t slot_index;
    uint8_t flags;
    uint8_t pad[5]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed)) mm_to_cm_message_kernel_params_t;

typedef struct {
    cm_iface_message_header_t header;
    mm_to_cm_message_kernel_params_t kernel;
} __attribute__((packed, aligned(64))) mm_to_cm_message_kernel_launch_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_kernel_launch_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t conf_buffer_addr;
} __attribute__((packed, aligned(64))) mm_to_cm_message_pmc_configure_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_pmc_configure_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t thread_mask; /**< Bit Mask of Thread. */
    uint32_t cm_control;  /**< Bit encoded CM Trace control. */
    uint8_t pad[4];       /**< Padding for alignment. */
} __attribute__((packed, aligned(64))) mm_to_cm_message_trace_rt_control_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_trace_rt_control_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t thread_mask; /**< Bit Mask of Thread. */
} __attribute__((packed, aligned(64))) mm_to_cm_message_trace_buffer_evict_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_trace_buffer_evict_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t shire_mask;  /**< Bit Mask of Shire to enable Trace Capture */
    uint64_t thread_mask; /**< Bit Mask of Thread within a Shire to enable Trace Capture */
    uint32_t event_mask; /**< This is a bit mask, each bit corresponds to a specific Event to trace */
    uint32_t
        filter_mask; /**< This is a bit mask representing a list of filters for a given event to trace */
    uint32_t
        threshold; /**< Trace buffer threshold, device will notify Host when buffer is filled up-to this threshold value. */
    uint8_t pad[4]; /**< Padding for alignment. */
} __attribute__((packed, aligned(64))) mm_to_cm_message_trace_rt_config_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_trace_rt_config_t);

/*
 * CM to MM messages
 */

typedef enum {
    CM_TO_MM_MESSAGE_ID_NONE = 0x80u,
    /* Kernel specific messages start from here */
    CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR,
    CM_TO_MM_MESSAGE_ID_KERNEL_ABORT_NACK,
    CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE,
    CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION,
    /* Compute FW specific messages start from here */
    CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY,
    CM_TO_MM_MESSAGE_ID_FW_EXCEPTION,
    CM_TO_MM_MESSAGE_ID_FW_ERROR
} cm_to_mm_message_id_e;

/* Status values for kernel completion message */
typedef enum {
    KERNEL_COMPLETE_STATUS_ERROR = -1,
    KERNEL_COMPLETE_STATUS_SUCCESS = 0
} kernel_complete_status_e;

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
    uint8_t pad[4]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed, aligned(64))) mm_to_cm_message_shire_ready_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_shire_ready_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t hart_id;
    uint64_t mcause;
    uint64_t mepc;
    uint64_t mtval;
    uint64_t mstatus;
    uint32_t shire_id;
    uint8_t pad[4]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed, aligned(64))) cm_to_mm_message_exception_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_exception_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t hart_id;
    int32_t error_code;
    uint8_t pad[4]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed, aligned(64))) cm_to_mm_message_fw_error_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_fw_error_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t hart_id;
    uint8_t slot_index;
    int8_t error_code;
    uint8_t pad[6]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_error_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_error_t);

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
    uint8_t slot_index;
    int8_t status;
    uint8_t pad[2]; /* Padding to make struct 64-bit aligned */
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_completed_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_completed_t);

#endif
