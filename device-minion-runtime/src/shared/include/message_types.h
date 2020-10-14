#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include "log.h"

typedef struct {
    message_header_t header;
    uint64_t hart_id;
    uint64_t mcause;
    uint64_t mepc;
    uint64_t mtval;
    uint64_t mstatus;
} __attribute__((packed, aligned(64))) message_exception_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_exception_t);

typedef struct {
    message_header_t header;
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) message_kernel_launch_ack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_kernel_launch_ack_t);

typedef struct {
    message_header_t header;
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) message_kernel_launch_nack_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_kernel_launch_nack_t);

typedef struct {
    message_header_t header;
    uint64_t kernel_id;
} __attribute__((packed, aligned(64))) message_kernel_launch_completed_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_kernel_launch_completed_t);

typedef struct {
    message_header_t header;
    log_level_t log_level;
} __attribute__((packed, aligned(64))) message_set_log_level_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_set_log_level_t);

typedef struct {
    message_header_t header;
    uint64_t conf_buffer_addr;
} __attribute__((packed, aligned(64))) message_pmc_configure_t;

ASSERT_CACHE_LINE_CONSTRAINTS(message_pmc_configure_t);

#endif
