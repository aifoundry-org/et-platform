#ifndef MAILBOX_H
#define MAILBOX_H

#include "mailbox_common.h"
#include "ringbuffer.h"

#include <stdint.h>

#define MBOX_MAX_MESSAGE_LENGTH RINGBUFFER_MAX_LENGTH

typedef enum {
    MBOX_MASTER_MINION = 0,
    MBOX_MAXION,
    MBOX_PCIE,
} mbox_e;

#define MBOX_COUNT 3

void MBOX_init_pcie(void);
void MBOX_init_mm(void);
void MBOX_init_max(void);

int64_t MBOX_send(mbox_e mbox, const void *const buffer_ptr, uint32_t length);

#endif
