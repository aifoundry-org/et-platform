#ifndef HOST_MESSAGE_H
#define HOST_MESSAGE_H

#include "kernel_params.h"
#include "kernel_info.h"
#include "mailbox_id.h"

typedef struct {
    mbox_message_id_t message_id;
    kernel_params_t kernel_params;
    kernel_info_t kernel_info;
} host_message_t;

typedef struct {
    mbox_message_id_t message_id;
    uint64_t kernel_id;
    mbox_response_id_t response_id;
} devfw_response_t;

#endif // HOST_MESSAGE_H
