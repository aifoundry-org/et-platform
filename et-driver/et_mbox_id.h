#ifndef ET_MBOX_ID_H
#define ET_MBOX_ID_H

#include <linux/types.h>

//typedef uint64_t mbox_message_id_t;

//Warning: Never change or renumber these IDs. They are an API between host
//and SoC.
enum mbox_message_id {
    MBOX_MESSAGE_ID_NONE                      = 0,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH             = 1,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE    = 2,
    MBOX_MESSAGE_ID_KERNEL_RESULT             = 3,
    MBOX_MESSAGE_ID_KERNEL_ABORT              = 4,
    MBOX_MESSAGE_ID_KERNEL_ABORT_RESPONSE     = 5,
    MBOX_MESSAGE_ID_GET_KERNEL_STATE          = 6,
    MBOX_MESSAGE_ID_GET_KERNEL_STATE_RESPONSE = 7,
    MBOX_MESSAGE_ID_REFLECT_TEST              = 8,
    MBOX_MESSAGE_ID_DMA_RUN_TO_DONE           = 9,
    MBOX_MESSAGE_ID_DMA_DONE                  = 10,
    MBOX_MESSAGE_ID_SET_MASTER_LOG_LEVEL      = 11,
    MBOX_MESSAGE_ID_SET_WORKER_LOG_LEVEL      = 12
};

#endif
