#include "kernel.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "fcc.h"
#include "hart.h"
#include "host_message.h"
#include "kernel_config.h"
#include "layout.h"
#include "log.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message.h"
#include "shire.h"
#include "syscall.h"

#include <esperanto/device-api/device_api.h>

//#define DEBUG_PRINT_KERNEL_INSTRUCTIONS

typedef struct
{
    kernel_state_t kernel_state;
    uint64_t shire_mask;
    uint64_t start_time;
    uint64_t end_time;
} kernel_status_t;

// Local state
static kernel_status_t kernel_status[MAX_SIMULTANEOUS_KERNELS];

// Shared state - Worker minion fetch kernel parameters from these
static kernel_config_t* const kernel_config = (kernel_config_t*)FW_MASTER_TO_WORKER_KERNEL_CONFIGS;

static void clear_kernel_config(kernel_id_t kernel_id);

void kernel_init(void)
{
    for (uint64_t kernel = 0; kernel < MAX_SIMULTANEOUS_KERNELS; kernel++)
    {
        clear_kernel_config(kernel);
    }
}

// Waits for all the shires associated with a kernel to report ready via a FCC,
// then synchronizes their release to run the kernel by sending a FCC
void __attribute__((noreturn)) kernel_sync_thread(uint64_t kernel_id)
{
    volatile const kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];

    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while (1)
    {
        // wait for a kernel launch sync request from master_thread
        WAIT_FCC(0);

        // read fresh kernel_config
        evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
        WAIT_CACHEOPS

        const uint64_t num_shires = kernel_config_ptr->num_shires;
        const uint64_t shire_mask = kernel_config_ptr->kernel_info.shire_mask;

        if ((num_shires > 0) && (shire_mask > 0))
        {
            // Broadcast launch FCC0 to all HARTs in all shires in shire_mask
            broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC0); // thread 0 FCC 0
            broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC2); // thread 1 FCC 0

            // Wait for a ready FCC1 from each shire
            for (uint64_t i = 0; i < num_shires; i++)
            {
                WAIT_FCC(1);
            }

            // Broadcast go FCC1 to all HARTs in all shires in shire_mask
            broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC1); // thread 0 FCC 1
            broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC3); // thread 1 FCC 1

            // Send message to master minion indicating the kernel is starting
            message_t sync_message = {.id = MESSAGE_ID_KERNEL_LAUNCH_ACK, .data = {0}};
            sync_message.data[0] = kernel_id;
            message_send_worker(get_shire_id(), get_hart_id(), &sync_message);

            // Wait for a done FCC1 from each shire
            for (uint64_t i = 0; i < num_shires; i++)
            {
                WAIT_FCC(1);
            }

            // Send message to master minion indicating the kernel is complete
            sync_message.id = MESSAGE_ID_KERNEL_COMPLETE;
            sync_message.data[0] = kernel_id;
            message_send_worker(get_shire_id(), get_hart_id(), &sync_message);
        }
        else
        {
            // Invalid config, send error message to the master minion
            message_t sync_message = {.id = MESSAGE_ID_KERNEL_LAUNCH_NACK, .data = {0}};
            sync_message.data[0] = kernel_id;
            message_send_worker(get_shire_id(), get_hart_id(), &sync_message);
        }
    }
}

void update_kernel_state(kernel_id_t kernel_id, kernel_state_t kernel_state)
{
    if (kernel_id >= KERNEL_ID_NONE)
    {
        return; // error
    }

    // Update kernel state per shire state change
    switch (kernel_state)
    {
        case KERNEL_STATE_UNUSED:
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_UNUSED;
        break;

        case KERNEL_STATE_LAUNCHED:
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_LAUNCHED;
        break;

        case KERNEL_STATE_RUNNING:
        {
            kernel_status[kernel_id].start_time = (uint64_t)syscall(SYSCALL_GET_MTIME, 0, 0, 0);

            kernel_status[kernel_id].kernel_state = KERNEL_STATE_RUNNING;

            // Mark all shires associated with this kernel as running
            for (uint64_t shire = 0; shire < 33; shire++)
            {
                if (kernel_status[kernel_id].shire_mask & (1ULL << shire))
                {
                    update_shire_state(shire, SHIRE_STATE_RUNNING);
                }
            }
        }
        break;

        case KERNEL_STATE_ABORTED:
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_ABORTED;
        break;

        case KERNEL_STATE_ERROR:
        {
            const devfw_response_t response = {
                .message_id = MBOX_MESSAGE_ID_KERNEL_RESULT,
                .kernel_id = kernel_id,
                .response_id = MBOX_KERNEL_RESULT_ERROR};

            MBOX_send(MBOX_PCIE, &response, sizeof(response));

            clear_kernel_config(kernel_id);
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_ERROR;
        }
        break;

        case KERNEL_STATE_COMPLETE:
        {
            kernel_status[kernel_id].end_time = (uint64_t)syscall(SYSCALL_GET_MTIME, 0, 0, 0);
            // TODO FIXME mtime is currently 40MHz and not the specified 10MHz for two reasons:
            // 1. RTLMIN-5392: PU RV Timer is dividing clk_100Mhz /25 instead of /10
            // 2. In ZeBu, clk_100 is forced to 1GHz for now - it will reduce to 100MHz eventually.
            // 1GHz / 25 = 40Mhz
            uint64_t elapsed_time_us = (kernel_status[kernel_id].end_time - kernel_status[kernel_id].start_time) / 40;

            log_write(LOG_LEVEL_INFO, "kernel %d complete, %" PRId64 "us\r\n", kernel_id, elapsed_time_us);

            const devfw_response_t response = {
                .message_id = MBOX_MESSAGE_ID_KERNEL_RESULT,
                .kernel_id = kernel_id,
                .response_id = MBOX_KERNEL_RESULT_OK};

            MBOX_send(MBOX_PCIE, &response, sizeof(response));

            // Mark all shires associated with this kernel as complete
            for (uint64_t shire = 0; shire < 33; shire++)
            {
                if (kernel_status[kernel_id].shire_mask & (1ULL << shire))
                {
                    update_shire_state(shire, SHIRE_STATE_COMPLETE);
                }
            }

            clear_kernel_config(kernel_id);
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_UNUSED;
        }
        break;

        case KERNEL_STATE_UNKNOWN:
        default:
        break;
    }
}

void launch_kernel(const kernel_params_t* const kernel_params_ptr, const kernel_info_t* const kernel_info_ptr)
{
    const kernel_id_t kernel_id = kernel_params_ptr->kernel_id;
    const uint64_t shire_mask = kernel_info_ptr->shire_mask;
    kernel_status_t* const kernel_status_ptr = &kernel_status[kernel_id];
    uint64_t num_shires = 0;
    bool allShiresReady = true;
    bool kernelReady = true;

    if (!all_shires_ready(shire_mask))
    {
        log_write(LOG_LEVEL_ERROR, "aborting kernel %d launch, not all shires ready\r\n", kernel_id);
        allShiresReady = false;
    }

#ifdef DEBUG_PRINT_KERNEL_INSTRUCTIONS
    const uint32_t* pc = (uint32_t*)kernel_info_ptr->compute_pc;

    for (int i = 0 ; i < 10; i++)
    {
        log_write(LOG_LEVEL_INFO, "PC: 0x%010" PRIxPTR " data: 0x%08" PRIx32 "\r\n", pc, *pc);
        pc += 1;
    }
#endif

    // Confirm this kernel is not active
    if (kernel_status_ptr->kernel_state != KERNEL_STATE_UNUSED)
    {
        log_write(LOG_LEVEL_ERROR, "aborting kernel %d launch, state not unused\r\n", kernel_id);
        kernelReady = false;
    }

    if (allShiresReady && kernelReady)
    {
        volatile kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];

        for (uint64_t shire = 0; shire < 33; shire++)
        {
            if (shire_mask & (1ULL << shire))
            {
                num_shires++;
            }
        }

        // Copy params and info into kernel config buffer
        kernel_config_ptr->kernel_params = *kernel_params_ptr;
        kernel_config_ptr->kernel_info   = *kernel_info_ptr;
        kernel_config_ptr->num_shires    = num_shires;

        // Fix up kernel_params_ptr for the copy in kernel_config
        kernel_config_ptr->kernel_info.kernel_params_ptr = &kernel_config[kernel_id].kernel_params;

        // Evict kernel config to point of coherency - sync threads and worker minion will read it
        FENCE
        evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
        WAIT_CACHEOPS

        // notify the appropriate sync thread to manage kernel launch
        notify_kernel_sync_thread(kernel_id, FCC_0);

        update_kernel_state(kernel_id, KERNEL_STATE_LAUNCHED);
        kernel_status_ptr->shire_mask = shire_mask;

        for (uint64_t shire = 0; shire < 33; shire++)
        {
            if (shire_mask & (1ULL << shire))
            {
                set_shire_kernel_id(shire, kernel_id);
            }
        }

        log_write(LOG_LEVEL_INFO, "launching kernel %d\r\n", kernel_id);
    }
    else
    {
        const devfw_response_t response = {
            .message_id = MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
            .kernel_id = kernel_id,
            .response_id = MBOX_KERNEL_LAUNCH_RESPONSE_ERROR_SHIRES_NOT_READY};

        MBOX_send(MBOX_PCIE, &response, sizeof(response));
    }
}

// Sends an abort message to all HARTs in all shires associated with the kernel_id if
// the kernel is running
dev_api_kernel_abort_response_result_e abort_kernel(kernel_id_t kernel_id)
{
    const kernel_state_t kernel_state = kernel_status[kernel_id].kernel_state;

    if ((kernel_state == KERNEL_STATE_LAUNCHED) || (kernel_state == KERNEL_STATE_RUNNING) || (kernel_state == KERNEL_STATE_ERROR))
    {
        message_t message = {.id = MESSAGE_ID_KERNEL_ABORT, .data = {0}};

        if (0 == broadcast_message_send_master(kernel_status[kernel_id].shire_mask, 0xFFFFFFFFFFFFFFFFU, &message))
        {
            log_write(LOG_LEVEL_INFO, "abort_kernel: aborted kernel %d\r\n", kernel_id);
            update_kernel_state(kernel_id, KERNEL_STATE_ABORTED);

            return DEV_API_KERNEL_ABORT_RESPONSE_RESULT_OK;
        }
    }

    return DEV_API_KERNEL_ABORT_RESPONSE_RESULT_ERROR;
}

kernel_state_t get_kernel_state(kernel_id_t kernel_id)
{
    if (kernel_id >= KERNEL_ID_NONE)
    {
        return KERNEL_STATE_UNKNOWN;
    }
    else
    {
        return kernel_status[kernel_id].kernel_state;
    }
}

// Clear fields of kernel config so worker minion recognize it's inactive
static void clear_kernel_config(kernel_id_t kernel_id)
{
    volatile kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];
    kernel_config_ptr->kernel_info.shire_mask = 0;

    // Evict kernel config to point of coherency - sync threads and worker minion will read it
    FENCE
    evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
    WAIT_CACHEOPS
}
