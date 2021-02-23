#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "log_levels.h"

/*
 * Common definitions
 */

typedef uint8_t cm_iface_message_id_t;
typedef uint8_t cm_iface_message_number_t;

typedef struct {
    cm_iface_message_number_t number;
    cm_iface_message_id_t id;
} cm_iface_message_header_t;

#define MESSAGE_MAX_PAYLOAD_SIZE (64 - sizeof(cm_iface_message_header_t))

#define ASSERT_CACHE_LINE_CONSTRAINTS(type)                                      \
    static_assert(sizeof(type) == 64, "sizeof(" #type ") must be 64 bytes");     \
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
    uint32_t count;
} __attribute__((aligned(64))) broadcast_message_ctrl_t;

ASSERT_CACHE_LINE_CONSTRAINTS(broadcast_message_ctrl_t);

/*
 * MM to CM messages
 */

typedef enum {
    MM_TO_CM_MESSAGE_ID_NONE = 0,
    MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH,
    MM_TO_CM_MESSAGE_ID_KERNEL_ABORT,
    MM_TO_CM_MESSAGE_ID_SET_LOG_LEVEL,
    MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL,
    MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_RESET,
    MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT,
    MM_TO_CM_MESSAGE_ID_PMC_CONFIGURE
} mm_to_cm_message_id_e;

#define KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH (1u << 0)
#define KERNEL_LAUNCH_FLAGS_EVICT_L3_AFTER_LAUNCH  (1u << 1)

typedef struct {
    cm_iface_message_header_t header;
    uint8_t kw_base_id;
    uint8_t slot_index;
    uint8_t flags;
    uint64_t code_start_address;
    uint64_t pointer_to_args;
    uint64_t shire_mask;
} __attribute__((packed, aligned(64))) mm_to_cm_message_kernel_launch_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_kernel_launch_t);

typedef struct {
    cm_iface_message_header_t header;
    log_level_t log_level;
} __attribute__((packed, aligned(64))) mm_to_cm_message_set_log_level_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_set_log_level_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t conf_buffer_addr;
} __attribute__((packed, aligned(64))) mm_to_cm_message_pmc_configure_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_pmc_configure_t);

/*
 * CM to MM messages
 */

typedef enum {
    CM_TO_MM_MESSAGE_ID_NONE = 0x80u,
    /* Kernel specific messages start from here */
    CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ACK,
    CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_NACK,
    CM_TO_MM_MESSAGE_ID_KERNEL_ABORT_NACK,
    CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE,
    CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION,
    /* Compute FW specific messages start from here */
    CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY,
    CM_TO_MM_MESSAGE_ID_FW_EXCEPTION,
} cm_to_mm_message_id_e;

/* Status values for kernel completion message */
typedef enum {
    KERNEL_COMPLETE_STATUS_ERROR = -1,
    KERNEL_COMPLETE_STATUS_SUCCESS = 0
} kernel_complete_status_e;

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
} __attribute__((packed, aligned(64))) mm_to_cm_message_shire_ready_t;

ASSERT_CACHE_LINE_CONSTRAINTS(mm_to_cm_message_shire_ready_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t hart_id;
    uint64_t mcause;
    uint64_t mepc;
    uint64_t mtval;
    uint64_t mstatus;
} __attribute__((packed, aligned(64))) cm_to_mm_message_exception_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_exception_t);

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
    uint8_t slot_index;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_ack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_ack_t);

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
    uint8_t slot_index;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_nack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_nack_t);

typedef struct {
    cm_iface_message_header_t header;
    uint32_t shire_id;
    uint8_t slot_index;
    int8_t status;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_completed_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_completed_t);

#endif
