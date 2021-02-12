#include "device_api_privileged.h"

#include "device_minion_runtime_build_configuration.h"
#include "pcie_dma.h"
#include "log.h"
#include "mailbox.h"
#include "syscall_internal.h"


#include <esperanto/device-api/device_api.h>
#include <esperanto/device-api/device_api_rpc_types_privileged.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

void handle_device_api_privileged_message_from_host(const mbox_message_id_t* message_id,
                                                    uint8_t* buffer)
{
    {
        struct command_header_t* const cmd = (void*) buffer;
        cmd->device_timestamp_mtime = (uint64_t)syscall(SYSCALL_GET_MTIME_INT, 0, 0, 0);
    }

    if (*message_id == MBOX_DEVAPI_PRIVILEGED_MID_REFLECT_CMD) {
        const struct reflect_cmd_t* const cmd = (const void* const) buffer;
        struct reflect_rsp_t rsp;
        rsp.response_info.message_id = MBOX_DEVAPI_PRIVILEGED_MID_REFLECT_RSP;
        prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
        int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
        if (result != 0)
        {
            log_write(LOG_LEVEL_ERROR, "DeviceAPI Privileged Reflect Test send error " PRIi64 "\r\n", result);
        }
    }
    if (*message_id == MBOX_DEVAPI_PRIVILEGED_MID_DMA_RUN_TO_DONE_CMD)
    {
        int rc;

        //Starts the DMA engine, and blocks for the DMA to complete.
        const struct dma_run_to_done_cmd_t *const message = (const void* const)buffer;

        struct dma_run_to_done_rsp_t done_message;
        done_message.response_info.message_id = MBOX_DEVAPI_PRIVILEGED_MID_DMA_RUN_TO_DONE_RSP;
        prepare_device_api_reply(&message->command_info, &done_message.response_info);

        done_message.chan = message->chan;
        done_message.status = 0;

        if(message->chan <= ET_DMA_CHAN_ID_READ_3) {
            rc = dma_configure_read(message->chan);
        }
        else if (message->chan >= ET_DMA_CHAN_ID_WRITE_0 && message->chan <= ET_DMA_CHAN_ID_WRITE_3) {
            rc = dma_configure_write(message->chan);
        }
        else {
            rc = -1;
        }

        if (rc != 0) {
            log_write(LOG_LEVEL_ERROR, "Failed to configure DMA chan %d (errno %d)\r\n", message->chan, rc);
            done_message.status = (et_dma_state_e) rc;
            MBOX_send(MBOX_PCIE, &done_message, sizeof(done_message));
            return;
        }

        dma_start(message->chan);
        uint32_t dma_status;
        bool dma_done;
        do{
           if(message->chan <= ET_DMA_CHAN_ID_READ_3) {
               dma_status = dma_get_read_int_status();
               dma_done = dma_check_read_done(message->chan, dma_status);

           } else {
               dma_status = dma_get_write_int_status();
               dma_done = dma_check_write_done((message->chan - DMA_CHAN_ID_WRITE_0), dma_status);
           }
        } while(!dma_done);

        dma_clear_done(message->chan);

        //TODO: does not deal with error conditions that could abort DMA transfer at all.

        MBOX_send(MBOX_PCIE, &done_message, sizeof(done_message));

        //TODO: notify glow kernel HARTs that data is done being transferred
    }

}
