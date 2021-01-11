#ifndef MESSAGE_H
#define MESSAGE_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MESSAGE_ID_NONE = 0,
    MESSAGE_ID_SHIRE_READY,
    MESSAGE_ID_KERNEL_LAUNCH,
    MESSAGE_ID_KERNEL_LAUNCH_ACK,
    MESSAGE_ID_KERNEL_LAUNCH_NACK,
    MESSAGE_ID_KERNEL_ABORT,
    MESSAGE_ID_KERNEL_ABORT_NACK,
    MESSAGE_ID_KERNEL_COMPLETE,
    MESSAGE_ID_LOOPBACK,
    MESSAGE_ID_U_MODE_EXCEPTION,
    MESSAGE_ID_FW_EXCEPTION,
    MESSAGE_ID_LOG_WRITE,
    MESSAGE_ID_SET_LOG_LEVEL,
    MESSAGE_ID_TRACE_UPDATE_CONTROL,
    MESSAGE_ID_TRACE_BUFFER_RESET,
    MESSAGE_ID_TRACE_BUFFER_EVICT,
    MESSAGE_ID_PMC_CONFIGURE
} cm_iface_message_id_e;

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

#include "message_types.h"

typedef struct {
    uint32_t count;
} __attribute__((aligned(64))) broadcast_message_ctrl_t;

ASSERT_CACHE_LINE_CONSTRAINTS(broadcast_message_ctrl_t);

void message_init_master(void);
void message_init_worker(uint64_t shire, uint64_t hart);

uint64_t get_message_flags(uint64_t shire);
cm_iface_message_id_t get_message_id(uint64_t shire, uint64_t hart);

inline bool message_available(uint64_t shire_id, uint64_t hart_id)
{
    return (get_message_id(shire_id, hart_id) != MESSAGE_ID_NONE);
}
bool broadcast_message_available(cm_iface_message_number_t previous_broadcast_message_number);

int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart,
                            const cm_iface_message_t *const message);
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart,
                            const cm_iface_message_t *const message);

int64_t broadcast_message_send_master(uint64_t dest_shire_mask, const cm_iface_message_t *const message);
cm_iface_message_number_t broadcast_message_receive_worker(cm_iface_message_t *const message);

void message_receive_master(uint64_t source_shire, uint64_t source_hart, cm_iface_message_t *const message);
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, cm_iface_message_t *const message);

#endif
