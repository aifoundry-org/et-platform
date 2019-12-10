#ifndef MAILBOX_ID_H
#define MAILBOX_ID_H

typedef uint64_t mbox_message_id_t;

//Warning: Never change or renumber these IDs. They are an API between host
//and SoC.
typedef enum
{
    MBOX_MESSAGE_ID_NONE                      = 0,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH             = 1,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE    = 2,
    MBOX_MESSAGE_ID_KERNEL_RESULT             = 3,
    MBOX_MESSAGE_ID_REFLECT_TEST              = 8,
    MBOX_MESSAGE_ID_DMA_RUN_TO_DONE           = 9,
    MBOX_MESSAGE_ID_DMA_DONE                  = 10,
    MBOX_MESSAGE_ID_FW_LAST                   = 999, ///< Last message-id of FW mesasges before
                                                     ///< the DeviceAPI messages start
} mbox_message_id_e;

typedef uint64_t mbox_response_id_t;

typedef enum {
    MBOX_KERNEL_LAUNCH_RESPONSE_OK = 0,
    MBOX_KERNEL_LAUNCH_RESPONSE_ERROR_SHIRES_NOT_READY,
    MBOX_KERNEL_LAUNCH_RESPONSE_ERROR,
    MBOX_KERNEL_LAUNCH_RESPONSE_RESULT_OK,
    MBOX_KERNEL_LAUNCH_RESPONSE_RESULT_ERROR,
} mbox_kernel_launch_reponse_e;

typedef enum {
    MBOX_KERNEL_RESULT_OK = 0,
    MBOX_KERNEL_RESULT_ERROR
} mbox_kernel_result_e;

#endif
