#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include "log.h"

/*
 * MM to CM messages
 */

#define KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH (1u << 0)
#define KERNEL_LAUNCH_FLAGS_EVICT_L3_AFTER_LAUNCH  (1u << 1)

typedef struct {
    cm_iface_message_header_t header;
    uint8_t kw_base_id;
    uint8_t kernel_id;
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
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_ack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_ack_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_nack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_nack_t);

typedef struct {
    cm_iface_message_header_t header;
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) cm_to_mm_message_kernel_launch_completed_t;

ASSERT_CACHE_LINE_CONSTRAINTS(cm_to_mm_message_kernel_launch_completed_t);

#endif
