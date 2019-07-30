#ifndef MESSAGE_H
#define MESSAGE_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define MASTER_SHIRE 32U

typedef enum
{
    MESSAGE_ID_NONE = 0xA5A5A5A5U,
    MESSAGE_ID_SHIRE_READY,
    MESSAGE_ID_KERNEL_LAUNCH,
    MESSAGE_ID_KERNEL_LAUNCH_ACK,
    MESSAGE_ID_KERNEL_LAUNCH_NACK,
    MESSAGE_ID_KERNEL_ABORT,
    MESSAGE_ID_KERNEL_ABORT_NACK,
    MESSAGE_ID_KERNEL_COMPLETE,
    MESSAGE_ID_LOOPBACK,
    MESSAGE_ID_EXCEPTION,
    MESSAGE_ID_LOG_WRITE
} message_id_t;

typedef uint32_t message_number_t;

typedef struct
{
    message_number_t number;
    message_id_t id;
    uint64_t data[7];
} __attribute__ ((aligned (64))) message_t; // always aligned to a cache line

// Ensure message_t maps directly to a 64-byte cache line
static_assert(sizeof(message_t) == 64, "sizeof message_t must be 64 bytes");
static_assert(_Alignof(message_t) == 64, "_Alignof message_t must be 64 bytes");

void message_init_master(void);
void message_init_worker(uint64_t shire, uint64_t hart);

uint64_t get_message_flags(uint64_t shire);
message_id_t get_message_id(uint64_t shire, uint64_t hart);

inline bool message_available(uint64_t shire_id, uint64_t hart_id) { return (get_message_id(shire_id, hart_id) != MESSAGE_ID_NONE); }
bool broadcast_message_available(message_number_t previous_broadcast_message_number);

int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart, const message_t* const message);
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart, const message_t* const message);

int64_t broadcast_message_send_master(uint64_t dest_shire_mask, uint64_t dest_hart_mask, const message_t* const message);
message_number_t broadcast_message_receive_worker(message_t* const message);

void message_receive_master(uint64_t source_shire, uint64_t source_hart, message_t* const message);
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, message_t* const message);

#endif
