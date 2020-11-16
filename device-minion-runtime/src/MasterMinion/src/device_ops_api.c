#include "device_ops_api.h"
#include "layout.h"
#include "vqueue.h"
#include "log.h"
#include "pcie_dma.h"

//TODO: For VQ WP2 we do not enable concurrency architecture within device runtime.
// For thi reason all command handling is implemented here and assigned to execute
// on the master minion
int64_t handle_device_ops_cmd(const uint32_t sq_idx)
{
    static uint8_t buffer[CIRCBUFFER_SIZE] __attribute__((aligned(8))) = { 0 };
    int64_t length;
    struct cmd_header_t *hdr;

    // Pop next available command from sq_idx
    length = VQUEUE_pop(SQ, sq_idx, buffer, sizeof(buffer));

    if (length > 0) {
        hdr = (void *)&buffer[0];

        switch (hdr->cmd_hdr.msg_id) {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD: {
            struct device_ops_api_compatibility_rsp_t rsp;

            log_write(LOG_LEVEL_INFO,
                      "DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD \r\n");

            // Construct and transmit command response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
            rsp.major = DEVICE_OPS_API_MAJOR;
            rsp.minor = DEVICE_OPS_API_MINOR;
            rsp.patch = DEVICE_OPS_API_PATCH;
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops API Compatibility response error: %" PRIi64 "\r\n",
                          res);
            } else {
                log_write(LOG_LEVEL_INFO,
                          "API_COMPATIBILITY_RSP major=% " PRIi8 "minor=% " PRIi8 "patch=% " PRIi8
                          "\r\n",
                          rsp.major, rsp.minor, rsp.patch);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD: {
            struct device_ops_device_fw_version_cmd_t *cmd = (void *)hdr;
            struct device_ops_fw_version_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD \r\n");

            // Construct and transmit command response with device firmware version
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
            if (cmd->firmware_type == DEV_OPS_FW_TYPE_MASTER_MINION_FW) {
                //TODO: implement proper logic to fetch and return firmware version
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MASTER_MINION_FW;
            } else if (cmd->firmware_type == DEV_OPS_FW_TYPE_MACHINE_MINION_FW) {
                //TODO: implement proper logic to fetch and return firmware version
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MACHINE_MINION_FW;
            } else if (cmd->firmware_type == DEV_OPS_FW_TYPE_WORKER_MINION_FW) {
                //TODO: implement proper logic to fetch and return firmware version
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_WORKER_MINION_FW;
            }
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops Device FW Version response error: %" PRIi64 "\r\n",
                          res);
            } else {
                log_write(LOG_LEVEL_INFO,
                          "FW_VERSION_RSP major=% " PRIi8 "minor=% " PRIi8 "patch=% " PRIi8 "\r\n",
                          rsp.major, rsp.minor, rsp.patch);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD: {
            struct device_ops_echo_cmd_t *cmd = (void *)hdr;
            struct device_ops_echo_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD \r\n");

            // Construct and transmit response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.echo_payload = cmd->echo_payload;
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops echo response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "ECHO_RSP echo_payload=% " PRIi32 "\r\n",
                          rsp.echo_payload);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD: {
            struct device_ops_kernel_launch_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD \r\n");

            // Spoof a response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
            rsp.cmd_wait_time = 0xdeadbeef;
            rsp.cmd_execution_time = 0xdeadbeef;
            rsp.status = DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_OK;
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops kernel launch response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "KERNEL_LAUNCH_RSP kernel_launch_status=% " PRIi32 "\r\n",
                          rsp.status);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD: {
            struct device_ops_kernel_abort_cmd_t *cmd = (void *)hdr;
            struct device_ops_kernel_abort_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD \r\n");

            // Spoof a response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
            rsp.kernel_id = cmd->kernel_id;
            rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_OK;
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops kernel abort response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "KERNEL_ABORT_RSP kernel_abort_status=% " PRIi32 "\r\n",
                          rsp.status);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD: {
            struct device_ops_kernel_state_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD \r\n");

            // Spoof a response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_RSP;
            rsp.status = DEV_OPS_API_KERNEL_STATE_COMPLETE;
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops kernel stats response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "KERNEL_STATE_RSP kernel_state_status=% " PRIi32 "\r\n",
                          rsp.status);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD: {
            struct device_ops_data_read_cmd_t *cmd = (void *)hdr;
            struct device_ops_data_read_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD \r\n");

            // Construct and transmit command response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;

            // Configure the device DMA read
            if (dma_trigger_transfer(DMA_DEVICE_TO_HOST, cmd->src_device_phy_addr,
                                     cmd->dst_host_phy_addr, cmd->size) == DMA_OPERATION_SUCCESS) {
                rsp.status = ETSOC_DMA_STATE_DONE;
            } else {
                rsp.status = ETSOC_DMA_STATE_ABORTED;
            }

            rsp.cmd_wait_time = 0xdeadbeef; // TODO: Just a dummy value for now
            rsp.cmd_execution_time = 0xdeadbeef; // TODO: Just a dummy value for now
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops Data Read response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "DATA_READ_RSP data_read_status=% " PRIi32 "\r\n",
                          rsp.status);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD: {
            struct device_ops_data_write_cmd_t *cmd = (void *)hdr;
            struct device_ops_data_write_rsp_t rsp;

            log_write(LOG_LEVEL_INFO, "DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD \r\n");

            // Construct and transmit command response
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;

            // Configure the device DMA write
            if (dma_trigger_transfer(DMA_HOST_TO_DEVICE, cmd->src_host_phy_addr,
                                     cmd->dst_device_phy_addr,
                                     cmd->size) == DMA_OPERATION_SUCCESS) {
                rsp.status = ETSOC_DMA_STATE_DONE;
            } else {
                rsp.status = ETSOC_DMA_STATE_ABORTED;
            }

            rsp.cmd_wait_time = 0xdeadbeef; // TODO: Just a dummy value for now
            rsp.cmd_execution_time = 0xdeadbeef; // TODO: Just a dummy value for now
            int64_t res = VQUEUE_push(CQ, sq_idx, &rsp, sizeof(rsp));
            if (res != 0) {
                log_write(LOG_LEVEL_ERROR,
                          "CQ push: Device-ops Data Write response error: %" PRIi64 "\r\n", res);
            } else {
                log_write(LOG_LEVEL_INFO, "DATA_WRITE_RSP data_read_status=% " PRIi32 "\r\n",
                          rsp.status);
            }

            break;
        }
        }
    }

    return length;
}