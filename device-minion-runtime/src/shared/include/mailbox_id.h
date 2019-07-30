#ifndef MAILBOX_ID_H
#define MAILBOX_ID_H

typedef uint64_t mbox_message_id_t;

typedef enum
{
    MBOX_MESSAGE_ID_NONE,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH,
    MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
    MBOX_MESSAGE_ID_KERNEL_RESULT
} mbox_message_id_e;


typedef uint64_t mbox_response_id_t;

typedef enum {
    MBOX_KERNEL_LAUNCH_RESPONSE_OK,
    MBOX_KERNEL_LAUNCH_RESPONSE_ERROR_SHIRES_NOT_READY,
    MBOX_KERNEL_LAUNCH_RESPONSE_ERROR
} mbox_kernel_launch_reponse_e;

typedef enum {
    MBOX_KERNEL_RESULT_OK,
    MBOX_KERNEL_RESULT_ERROR
} mbox_kernel_result_e;

#endif
