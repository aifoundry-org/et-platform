#include "kernel.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "fcc.h"
#include "hart.h"
#include "layout.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message.h"
#include "printf.h"
#include "shire.h"

typedef struct
{
    kernel_state_t kernel_state;
    uint64_t shire_mask;
} kernel_status_t;

typedef struct
{
    kernel_info_t kernel_info;
    kernel_params_t kernel_params;
    uint64_t num_shires;
} kernel_config_t;

// Local state
static kernel_status_t kernel_status[MAX_SIMULTANEOUS_KERNELS];

// Shared state - Worker minion fetch kernel parameters from these
static kernel_config_t kernel_config[MAX_SIMULTANEOUS_KERNELS];

static void notify_sync_thread(kernel_id_t kernel_id);

// Waits for all the shires associated with a kernel to report ready via a FCC,
// then synchronizes their release to run the kernel by sending a FCC
void __attribute__((noreturn)) kernel_sync_thread(uint64_t kernel_id)
{
    volatile const kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];

    while (1)
    {
        // wait for a kernel launch sync request from master_thread
        WAIT_FCC(0);

        // read fresh kernel_config
        evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
        WAIT_CACHEOPS

        const uint64_t num_shires = kernel_config_ptr->num_shires;
        const uint64_t shire_mask = kernel_config_ptr->kernel_info.shire_mask;

        // Wait for a ready FCC1 from each shire
        for (uint64_t i = 0; i < num_shires; i++)
        {
            WAIT_FCC(1);
        }

        // Broadcast go FCC to all HARTs in all shires in shire_mask
        broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC1); // thread 0 FCC 1
        broadcast(0xFFFFFFFFU, shire_mask, PRV_U, ESR_SHIRE_REGION, ESR_SHIRE_FCC3); // thread 1 FCC 1
    }
}

void update_kernel_state(kernel_id_t kernel_id, kernel_state_t kernel_state)
{
    if ((kernel_id == KERNEL_ID_NONE) || (kernel_id > KERNEL_ID_3))
    {
        return; // error
    }

    // Update kernel state per shire state change
    switch (kernel_state)
    {
        case KERNEL_STATE_UNUSED:
        break;

        case KERNEL_STATE_RUNNING:
        break;

        case KERNEL_STATE_ERROR:
        {
            const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_RESULT,
                                         kernel_id,
                                         MBOX_KERNEL_RESULT_ERROR};

            MBOX_send(MBOX_PCIE, response, sizeof(response));

            kernel_status[kernel_id].kernel_state = KERNEL_STATE_ERROR;
        }
        break;

        case KERNEL_STATE_COMPLETE:
        {
            printf("kernel %d complete\r\n", kernel_id);
            const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_RESULT,
                                         kernel_id,
                                         MBOX_KERNEL_RESULT_OK};

            MBOX_send(MBOX_PCIE, response, sizeof(response));

            kernel_status[kernel_id].kernel_state = KERNEL_STATE_UNUSED;
        }
        break;

        default:
        break;
    }
}

void launch_kernel(const kernel_params_t* const kernel_params_ptr, const kernel_info_t* const kernel_info_ptr)
{
    // TODO FIXME HACK
    printf("kernel_params_ptr = 0x%010" PRIx64 "\r\n", (uint64_t)kernel_params_ptr);
    printf("kernel_info_ptr = 0x%010" PRIx64 "\r\n", (uint64_t)kernel_info_ptr);

    static message_t message;
    const kernel_id_t kernel_id = kernel_params_ptr->kernel_id;
    const uint64_t shire_mask = kernel_info_ptr->shire_mask;
    kernel_status_t* const kernel_status_ptr = &kernel_status[kernel_id];
    uint64_t num_shires = 0;
    bool allShiresReady = true;
    bool kernelReady = true;

    if (!all_shires_ready(shire_mask))
    {
        printf("launch_kernel: kernel %d not all shires ready\r\n", kernel_id);
        allShiresReady = false;
    }

    // Confirm this kernel is not active
    if (kernel_status_ptr->kernel_state != KERNEL_STATE_UNUSED)
    {
        printf("launch_kernel: kernel %d state not unused\r\n", kernel_id);
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

        // Create the message to broadcast to all the worker minion
        message.id = MESSAGE_ID_KERNEL_LAUNCH;
        message.data[0] = kernel_config[kernel_id].kernel_info.compute_pc;
        message.data[1] = (uint64_t)&kernel_config[kernel_id].kernel_params;
        message.data[2] = 0; // TODO grid config

        if (0 == broadcast_message_send_master(shire_mask, 0xFFFFFFFFFFFFFFFFU, &message))
        {
            printf("launch_kernel: launching kernel %d \r\n", kernel_id);
            const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
                                         kernel_id,
                                         MBOX_KERNEL_LAUNCH_RESPONSE_OK};

            MBOX_send(MBOX_PCIE, response, sizeof(response));

            // notify the appropriate sync thread to manage kernel launch
            notify_sync_thread(kernel_id);

            kernel_status_ptr->shire_mask = shire_mask;
            kernel_status_ptr->kernel_state = KERNEL_STATE_RUNNING;

            for (uint64_t shire = 0; shire < 33; shire++)
            {
                if (shire_mask & (1ULL << shire))
                {
                    update_shire_state(shire, SHIRE_STATE_RUNNING);
                    set_shire_kernel_id(shire, kernel_id);
                }
            }
        }
        else
        {
            printf("launch_kernel: error broadcasting kernel %d launch message\r\n", kernel_id);
            const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
                                         kernel_id,
                                         MBOX_KERNEL_LAUNCH_RESPONSE_ERROR};

            MBOX_send(MBOX_PCIE, response, sizeof(response));
        }
    }
    else
    {
        printf("launch_kernel: aborting kernel %d launch\r\n", kernel_id);
        const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
                                     kernel_id,
                                     MBOX_KERNEL_LAUNCH_RESPONSE_ERROR_SHIRES_NOT_READY};

        MBOX_send(MBOX_PCIE, response, sizeof(response));
    }
}

// Notifies the HART running the sync_thread for the kernel_id
static void notify_sync_thread(kernel_id_t kernel_id)
{
    const uint64_t bitmask = 1U << (FIRST_KERNEL_LAUNCH_SYNC_MINON + (kernel_id / 2));
    const uint64_t thread = kernel_id % 2;

    SEND_FCC(THIS_SHIRE, thread, 0, bitmask);
}

bool kernel_complete(kernel_id_t kernel_id)
{
    return all_shires_complete(kernel_status[kernel_id].shire_mask);
}

kernel_state_t get_kernel_state(kernel_id_t kernel_id)
{
    return kernel_status[kernel_id].kernel_state;
}
