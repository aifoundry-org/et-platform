#ifndef MAILBOX_H
#define MAILBOX_H

#include "mailbox_common.h"
#include "ringbuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MBOX_MAX_MESSAGE_LENGTH RINGBUFFER_MAX_LENGTH

typedef enum {
    MBOX_SP = 0,
    MBOX_PCIE,
} mbox_e;

#define MBOX_COUNT 2

void MBOX_init(void);
int64_t MBOX_send(mbox_e mbox, const void *const buffer_ptr, uint32_t length);
int64_t MBOX_receive(mbox_e mbox, void *const buffer_ptr, size_t buffer_size);
void MBOX_update_status(mbox_e mbox);
bool MBOX_ready(mbox_e mbox);
bool MBOX_empty(mbox_e mbox);

#endif
