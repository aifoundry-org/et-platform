#ifndef MAILBOX_COMMON_H
#define MAILBOX_COMMON_H

#include "ringbuffer_common.h"

#include <assert.h>
#include <stdint.h>

#define MBOX_HEADER_LENGTH    sizeof(mbox_header_t)
#define MBOX_MAGIC            0xBEEF
#define MBOX_MAX_LENGTH       UINT16_MAX
#define MBOX_BUFFER_ALIGNMENT 8
#define MBOX_MASTER           1
#define MBOX_SLAVE            0

// Error Codes
#define SP_MM_HANDSHAKE_POLL_SUCCESS  0
#define SP_MM_HANDSHAKE_POLL_TIMEOUT  -1

static_assert(MBOX_MAX_LENGTH >= RINGBUFFER_LENGTH,
              "mbox_header_t length field must be large enough to index ringbuffer");

typedef enum {
    MBOX_STATUS_NOT_READY = 0U,
    MBOX_STATUS_READY = 1U,
    MBOX_STATUS_WAITING = 2U,
    MBOX_STATUS_ERROR = 3U
} mbox_status_e;

typedef struct mbox_s {
    uint32_t master_status;
    uint32_t slave_status;
    ringbuffer_t tx_ring_buffer;
    ringbuffer_t rx_ring_buffer;
} mbox_t;

static_assert(sizeof(mbox_t) <= 4096, "mbox_t must be <= 4KB");

typedef struct {
    uint16_t length;
    uint16_t magic;
} mbox_header_t;

#endif
