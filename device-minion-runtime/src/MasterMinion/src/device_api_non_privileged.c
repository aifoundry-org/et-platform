#include "device_api_non_privileged.h"

#include "build_configuration.h"
#include "device-mrt-trace.h"
#include "kernel.h"
#include "kernel_params.h"
#include "log.h"
#include "mailbox.h"
#include "message.h"
#include "syscall_internal.h"
#include "minion_fw_boot_config.h"

#include <esperanto/device-api/device_api.h>
#include <esperanto/device-api/device_api_rpc_types_non_privileged.h>
#include <esperanto/device-api/tracing_types_non_privileged.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

void prepare_device_api_reply(const struct command_header_t *const cmd,
                              struct response_header_t *const rsp)
{
    rsp->command_info = *cmd;
    rsp->device_timestamp_mtime = (uint64_t)syscall(SYSCALL_GET_MTIME_INT, 0, 0, 0);
}

log_level_t devapi_loglevel_to_fw(const enum LOG_LEVELS log_level)
{
    // FIXME we expect that the enums between devapi and fw match for now just cast
    return (log_level_e)log_level;
}

static inline void broadcast_message_to_all_workers(const message_t *message, uint64_t shire_mask)
{
    broadcast_message_send_master(shire_mask & 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF, message);
    if (shire_mask & (1ULL << MASTER_SHIRE)) {
        // Lower 16 Minions of Master Shire run Master FW, upper 16 Minions run Worker FW
        broadcast_message_send_master(1ULL << MASTER_SHIRE, 0xFFFFFFFF00000000, message);
    }
}

void handle_device_api_non_privileged_message_from_host(const mbox_message_id_t* message_id,
                                                        uint8_t* buffer)
{
    volatile minion_fw_boot_config_t *boot_config = (volatile minion_fw_boot_config_t *)FW_MINION_FW_BOOT_CONFIG;
    uint64_t booted_minion_shires = boot_config->minion_shires & ((1ULL << NUM_SHIRES) - 1);

    {
        struct command_header_t *const cmd = (void *)buffer;
        cmd->device_timestamp_mtime = (uint64_t)syscall(SYSCALL_GET_MTIME_INT, 0, 0, 0);
    }

    if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_CMD) {
        const struct reflect_test_cmd_t* const cmd = (const void* const) buffer;
        struct reflect_test_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR, "DeviceAPI Reflect Test send error %" PRIi64 "\r\n", result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_DEVICE_FW_VERSION_CMD) {
        const struct device_fw_version_cmd_t* const cmd = (const void* const) buffer;
        struct device_fw_version_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_DEVICE_FW_VERSION_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
        memcpy(&rsp.device_fw_commit, IMAGE_VERSION_INFO_SYMBOL.git_hash,
               sizeof(rsp.device_fw_commit));
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Device FW Version MBOX_send error %" PRIi64 "\r\n", result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_DEVICE_API_VERSION_CMD) {
        const struct device_api_version_cmd_t* const cmd = (const void* const) buffer;
        struct device_api_version_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_DEVICE_API_VERSION_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
        rsp.major = ESPERANTO_DEVICE_API_VERSION_MAJOR;
        rsp.minor = ESPERANTO_DEVICE_API_VERSION_MINOR;
        rsp.patch = ESPERANTO_DEVICE_API_VERSION_PATCH;
        rsp.api_hash = DEVICE_API_NON_PRIVILEGED_HASH;
        // FIXME SW-1319
        rsp.accept = true;
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI DeviceAPI Version MBOX_send error %" PRIi64 "\r\n", result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_ABORT_CMD) {
        log_write(LOG_LEVEL_INFO, "received kernel abort message fom host\r\n");

#ifdef DEBUG_PRINT_HOST_MESSAGE
        print_host_message(buffer, length);
#endif

        const struct kernel_abort_cmd_t *const cmd = (const void *const)buffer;
        struct kernel_abort_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_ABORT_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        abort_kernel(cmd->kernel_id);

        rsp.status = true;
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR, "DeviceAPI Kernel Abort error %" PRIi64 "\r\n", result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_LAUNCH_CMD) {
        log_write(LOG_LEVEL_INFO, "received kernel launch message fom host\r\n");

#ifdef DEBUG_PRINT_HOST_MESSAGE
        print_host_message(buffer, length);
#endif

        const struct kernel_launch_cmd_t *const cmd = (const void *const)buffer;
        launch_kernel(cmd);
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_STATE_CMD) {
        const struct kernel_state_cmd_t* const cmd = (const void* const) buffer;
        struct kernel_state_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_STATE_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        rsp.status = get_kernel_state(cmd->kernel_id);
        log_write(LOG_LEVEL_INFO, "Kernel: %" PRIi64 " status: %" PRIi64 "\r\n", cmd->kernel_id,
                  rsp.status);

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR, "DeviceAPI Kernel Status MBOX_send error %" PRIi64 "\r\n",
                      result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_SET_MASTER_LOG_LEVEL_CMD) {
        const struct set_master_log_level_cmd_t* const cmd = (const void* const) buffer;
        struct set_master_log_level_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_SET_MASTER_LOG_LEVEL_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
        log_set_level(devapi_loglevel_to_fw(cmd->log_level));
        rsp.status = true;
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI DeviceAPI Set Master Log Level MBOX_send error %" PRIi64 "\r\n",
                      result);
        }
    } else if (*message_id ==  MBOX_DEVAPI_NON_PRIVILEGED_MID_SET_WORKER_LOG_LEVEL_CMD) {
        const struct set_worker_log_level_cmd_t* const cmd = (const void* const) buffer;
        struct set_worker_log_level_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_SET_WORKER_LOG_LEVEL_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_SET_LOG_LEVEL;
        message.data[0] = devapi_loglevel_to_fw(cmd->log_level);
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI DeviceAPI Set Master Log Level MBOX_send error %" PRIi64 "\r\n",
                      result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_DISCOVER_TRACE_BUFFER_CMD) {
        const struct discover_trace_buffer_cmd_t *const cmd =
            (const void *const)buffer;
        struct discover_trace_buffer_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_DISCOVER_TRACE_BUFFER_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        rsp.trace_base = DEVICE_MRT_TRACE_BASE;
        rsp.trace_buffer_size = trace_ctrl->buffer_size;
        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Discover trace buffer MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }

    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_GROUP_KNOB_CMD) {
        const struct configure_trace_group_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_group_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_GROUP_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        if (cmd->enable) {
            trace_ctrl->group_knobs[cmd->group_id / (sizeof(uint64_t) * 8UL)] |=
                (1ULL << cmd->group_id);
        } else {
            trace_ctrl->group_knobs[cmd->group_id / (sizeof(uint64_t) * 8UL)] &=
                ~(1ULL << cmd->group_id);
        }

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Configure Group knob MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_EVENT_KNOB_CMD) {
        const struct configure_trace_event_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_event_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;
        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_EVENT_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        if (cmd->enable) {
            trace_ctrl->event_knobs[cmd->event_id / (sizeof(uint64_t) * 8UL)] |=
                (1ULL << cmd->event_id);
        } else {
            trace_ctrl->event_knobs[cmd->event_id / (sizeof(uint64_t) * 8UL)] &=
                ~(1ULL << cmd->event_id);
        }

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Configure Event knob MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_BUFFER_SIZE_KNOB_CMD) {
        const struct configure_trace_buffer_size_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_buffer_size_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_BUFFER_SIZE_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        trace_ctrl->buffer_size = cmd->buffer_size;

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(
                LOG_LEVEL_ERROR,
                "DeviceAPI Configure Buffer size knob MBOX_send error " PRIi64
                "\r\n",
                result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_STATE_KNOB_CMD) {
        const struct configure_trace_state_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_state_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_STATE_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        trace_ctrl->trace_en = cmd->enable & 0x01;

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(
                LOG_LEVEL_ERROR,
                "DeviceAPI Configure Trace state knob MBOX_send error " PRIi64
                "\r\n",
                result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_UART_LOGGING_KNOB_CMD) {
        const struct configure_trace_uart_logging_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_uart_logging_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_UART_LOGGING_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        trace_ctrl->uart_en = cmd->enable & 0x01;

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(
                LOG_LEVEL_ERROR,
                "DeviceAPI Configure Trace UART logging knob MBOX_send error " PRIi64
                "\r\n",
                result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_LOG_LEVEL_KNOB_CMD) {
        const struct configure_trace_log_level_knob_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_trace_log_level_knob_rsp_t rsp;
        struct trace_control_t *trace_ctrl =
            (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_LOG_LEVEL_KNOB_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        trace_ctrl->log_level = cmd->log_level;

        // Evict control region changes
        TRACE_update_control();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_UPDATE_CONTROL;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(
                LOG_LEVEL_ERROR,
                "DeviceAPI Configure Trace log level knob MBOX_send error " PRIi64
                "\r\n",
                result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_RESET_TRACE_BUFFERS_CMD) {
        const struct reset_trace_buffers_cmd_t *const cmd =
            (const void *const)buffer;
        struct reset_trace_buffers_rsp_t rsp;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_RESET_TRACE_BUFFERS_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        // reset trace buffer for next run
        TRACE_init_buffer();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_BUFFER_RESET;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Reset Trace Buffers MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }
    } else if (*message_id ==
               MBOX_DEVAPI_NON_PRIVILEGED_MID_PREPARE_TRACE_BUFFERS_CMD) {
        const struct prepare_trace_buffers_cmd_t *const cmd =
            (const void *const)buffer;
        struct prepare_trace_buffers_rsp_t rsp;

        rsp.response_info.message_id =
            MBOX_DEVAPI_NON_PRIVILEGED_MID_PREPARE_TRACE_BUFFERS_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        // reset trace buffer for next run
        TRACE_evict_buffer();

        // send message to workers
        message_t message;
        message.id = MESSAGE_ID_TRACE_BUFFER_EVICT;
        broadcast_message_to_all_workers(&message, booted_minion_shires);

        rsp.status = true;

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "DeviceAPI Prepare Trace buffers MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }
    } else if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_PMCS_CMD) {
        const struct configure_pmcs_cmd_t *const cmd =
            (const void *const)buffer;
        struct configure_pmcs_rsp_t rsp;

        // Send message to workers to configure PMCs
        message_t message;
        message.id = MESSAGE_ID_PMC_CONFIGURE;
        message.data[0] = cmd->conf_buffer_addr;

        // Do not send anything to master shire yet
        broadcast_message_to_all_workers(&message, booted_minion_shires & 0xFFFFFFFFULL);
        rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_PMCS_RSP;
        rsp.status = true;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);

        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0) {
            log_write(LOG_LEVEL_ERROR,
                      "Configure PMCs MBOX_send error " PRIi64
                      "\r\n",
                      result);
        }
    } else {
        log_write(LOG_LEVEL_ERROR, "Invalid DeviceAPI message ID: %" PRIu64 "\r\n", *message_id);
    }
}
