#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MESSAGE_ID_NONE 0U
#define MESSAGE_ID_EXCEPTION 0xBEEF

#define MASTER_SHIRE 32U

typedef struct
{
    uint64_t id;
    uint64_t data[7];
} __attribute__ ((aligned (64))) message_t; // always aligned to a cache line

void message_init_master(void);
void message_init_worker(uint64_t shire, uint64_t hart);

uint64_t get_message_flags(uint64_t shire);
uint64_t get_message_id(uint64_t shire, uint64_t hart);

int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart, const message_t* const message);
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart, const message_t* const message);

void message_receive_master(uint64_t source_shire, uint64_t source_hart, message_t* const message);
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, message_t* const message);

#endif
