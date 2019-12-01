#ifndef MAILBOX_ID_H
#define MAILBOX_ID_H

typedef uint64_t mbox_message_id_t;

//Warning: Never change or renumber these IDs. They are an API between host
//and SoC.
typedef enum
{
    MBOX_MESSAGE_ID_NONE                      = 0,
    MBOX_MESSAGE_ID_REFLECT_TEST              = 8,
    MBOX_MESSAGE_ID_DMA_RUN_TO_DONE           = 9,
    MBOX_MESSAGE_ID_DMA_DONE                  = 10,
    MBOX_MESSAGE_ID_FW_LAST                   = 999, ///< Last message-id of FW mesasges before
                                                     ///< the DeviceAPI messages start
} mbox_message_id_e;

#endif
