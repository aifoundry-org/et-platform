#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_state.h"
#include "kernel_sync.h"

#include <esperanto/device-api/device_api.h>
#include <esperanto/device-api/device_api_message_types.h>
#include <esperanto/device-api/device_api_rpc_types_non_privileged.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    KERNEL_ID_0 = 0,
    KERNEL_ID_1,
    KERNEL_ID_2,
    KERNEL_ID_3,
    KERNEL_ID_NONE
} kernel_id_t;

void kernel_init(void);
void __attribute__((noreturn)) kernel_sync_thread(uint64_t kernel_id);
void update_kernel_state(kernel_id_t kernel_id, kernel_state_t kernel_state);
void launch_kernel(const struct kernel_launch_cmd_t *const cmd);
dev_api_kernel_abort_response_result_e abort_kernel(kernel_id_t kernel_id);
kernel_state_t get_kernel_state(kernel_id_t kernel_id);

#endif
