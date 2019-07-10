#include "atomic_barrier.h"
#include "build_configuration.h"
#include "kernel_info.h"
#include "layout.h"
#include "message.h"
#include "printf.h"
#include "print_exception.h"
#include "serial.h"
#include "shire.h"
#include "swi.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

typedef enum
{
    KERNEL_ID_0 = 0,
    KERNEL_ID_1,
    KERNEL_ID_2,
    KERNEL_ID_3,
    KERNEL_ID_NONE
} kernel_id_t;

typedef enum
{
    SHIRE_STATE_UNKNOWN = 0,
    SHIRE_STATE_READY,
    SHIRE_STATE_RUNNING,
    SHIRE_STATE_ERROR,
    SHIRE_STATE_COMPLETE
} shire_state_t;

typedef struct
{
    shire_state_t shire_state;
    kernel_id_t kernel_id;
} shire_status_t;

typedef enum
{
    KERNEL_STATE_UNUSED = 0,
    KERNEL_STATE_RUNNING,
    KERNEL_STATE_ERROR,
    KERNEL_STATE_COMPLETE
} kernel_state_t;

typedef struct
{
    kernel_state_t kernel_state;
    uint64_t shire_mask; // bitmask of which shires are associated with this kernel
    uint64_t shire_error_mask;
    uint64_t shire_complete_mask;
} kernel_status_t;

static shire_status_t shire_status[33];
static kernel_status_t kernel_status[4];

static message_t message;

static void handle_message_from_host(void);
static void handle_messages_from_workers(void);
static void handle_message_from_worker(uint64_t shire, uint64_t hart);
static void handle_message_from_sp(void);
static void update_shire_state(uint64_t shire, shire_state_t state);
static void update_kernel_state(kernel_id_t kernel_id, uint64_t shire, shire_state_t shire_state);
static void handle_pcie_events(void);
static void handle_timer_events(void);
static void launch_kernel(uint64_t shire_mask, uint64_t entry_addr, const kernel_params_t* const kernel_params_ptr, const grid_config_t* const grid_config_ptr);
void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    if (get_hart_id() != 2048)
    {
        asm volatile ("wfi");
    }

    // Configure supervisor trap vector and sscratch (supervisor stack pointer)
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n" // supervisor trap vector
        "csrw  sscratch, sp     \n" // Initial saved stack pointer points to S-mode stack scratch region
        : "=&r" (temp)
    );

    SERIAL_init(UART0);

    printf("\r\nMaster minion " GIT_VERSION_STRING "\r\n");

    printf("Initializing message buffers...");
    message_init_master();
    printf("done\r\n");

    // Enable supervisor external and software interrupts
    asm volatile (
        "li    %0, 0x202    \n"
        "csrs  sie, %0      \n" // Enable supervisor external and software interrupts
        "csrsi sstatus, 0x2 \n" // Enable interrupts
        : "=&r" (temp)
    );

    // Wait for a message from the host, worker minion, PCI-E, etc.
    for (;;)
    {
        // TODO FIXME UNCOMMENT
        //asm volatile ("wfi");

        if (swi_flag)
        {
            swi_flag = false;
            handle_messages_from_workers();
        }

        // TODO FIXME HACK belongs under if (swi_flag) but faking for now
        handle_message_from_host();

        // External interrupts
        handle_message_from_sp();
        handle_pcie_events();

        // Timer interrupts
        handle_timer_events();
    }
}

static void handle_message_from_host(void)
{
    // For now, fake host launches kernel 0 any time it's unused.
    if (kernel_status[KERNEL_ID_0].kernel_state == KERNEL_STATE_UNUSED)
    {
        kernel_params_t kernel_params = {
            .tensor_a = 0,
            .tensor_b = 0,
            .tensor_c = 0,
            .tensor_d = 0,
            .tensor_e = 0,
            .tensor_f = 0,
            .tensor_g = 0,
            .tensor_h = 0,
            .kernel_id = KERNEL_ID_0
        };

        kernel_info_t kernel_info = {
            .shire_mask = 1,
            .compute_pc = 0, // TODO FIXME HACK worker firmware ignores this for now
            .kernel_params_ptr = &kernel_params,
            .grid_config_ptr = NULL
        };

        launch_kernel(kernel_info.shire_mask,
                      kernel_info.compute_pc,
                      kernel_info.kernel_params_ptr,
                      kernel_info.grid_config_ptr);
    }
}

static void handle_messages_from_workers(void)
{
    // Check for messages from every hart in every shire
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        const uint64_t flags = get_message_flags(shire);

        if (flags)
        {
            for (uint64_t hart = 0; hart < 64; hart++)
            {
                if (flags & (1ULL << hart))
                {
                    handle_message_from_worker(shire, hart);
                }
            }
        }
    }
}

static void handle_message_from_worker(uint64_t shire, uint64_t hart)
{
    message_receive_master(shire, hart, &message);

    switch (message.id)
    {
        case MESSAGE_ID_NONE:
            printf("Invalid MESSAGE_ID_NONE received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
        break;

        case MESSAGE_ID_SHIRE_READY:
            printf("MESSAGE_ID_SHIRE_READY received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_READY);
        break;

        case MESSAGE_ID_KERNEL_LAUNCH:
            printf("Invalid MESSAGE_ID_KERNEL_LAUNCH received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
        break;

        case MESSAGE_ID_KERNEL_ABORT:
            printf("Invalid MESSAGE_ID_KERNEL_ABORT received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
        break;

        case MESSAGE_ID_KERNEL_LAUNCH_NACK:
            printf("MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_ERROR);
        break;

        case MESSAGE_ID_KERNEL_ABORT_NACK:
            printf("MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_ERROR);
        break;

        case MESSAGE_ID_KERNEL_COMPLETE:
            printf("MESSAGE_ID_KERNEL_COMPLETE received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_COMPLETE);
        break;

        case MESSAGE_ID_LOOPBACK:
            printf("MESSAGE_ID_LOOPBACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
        break;

        case MESSAGE_ID_EXCEPTION:
            print_exception(message.data[1], message.data[2], message.data[3], message.data[4], message.data[0]);
            update_shire_state(shire, SHIRE_STATE_ERROR);
        break;

        default:
            printf("Unknown message id = 0x%016" PRIx64 "received from shire %" PRId64 " hart %" PRId64 "\r\n", message.id, shire, hart);
        break;
    }
}

static void handle_message_from_sp(void)
{

}

static void update_shire_state(uint64_t shire, shire_state_t shire_state)
{
    // Get the kernel_id of the kernel associated with this shire, if any.
    const kernel_id_t kernel_id = shire_status[shire].kernel_id;

    // Update state

    // TODO FIXME this is hokey, clean up state handling.
    if (shire_state == SHIRE_STATE_COMPLETE)
    {
        shire_status[shire].shire_state = SHIRE_STATE_READY;
    }
    else
    {
        shire_status[shire].shire_state = shire_state;
    }

    // If there was a kernel assocaited with this shire, update its state.
    if (kernel_id != KERNEL_ID_NONE)
    {
        update_kernel_state(kernel_id, shire, shire_state);
    }
}

static void update_kernel_state(kernel_id_t kernel_id, uint64_t shire, shire_state_t shire_state)
{
    if (kernel_id == KERNEL_ID_NONE)
    {
        return; // error
    }

    // Update kernel state per shire state change
    switch (shire_state)
    {
        case SHIRE_STATE_UNKNOWN:
        break;

        case SHIRE_STATE_READY:
        break;

        case SHIRE_STATE_RUNNING:
        break;

        case SHIRE_STATE_ERROR:
        {
            // Update completion mask of associated kernel
            kernel_status[kernel_id].shire_error_mask |= (1ULL << shire);
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_ERROR;

            // TODO FIXME @Will send message/data back to the host
        }
        break;

        case SHIRE_STATE_COMPLETE:
        {
            // Update completion mask of associated kernel
            kernel_status[kernel_id].shire_complete_mask |= (1ULL << shire);

            // If this was the final shire to complete, update kernel status.
            if (kernel_status[kernel_id].shire_complete_mask == kernel_status[kernel_id].shire_mask)
            {
                printf("All shires complete\r\n");

                kernel_status[kernel_id].kernel_state = KERNEL_STATE_COMPLETE;

                // TODO FIXME @Will send message/data back to the host
            }
        }
        break;

        default:
        break;
    }
}

static void handle_pcie_events(void)
{
    // Keep the PCI-E data pump going and update kernel state as needed, i.e. transitioning a kernel
    // from KERNEL_STATE_COMPLETE to KERNEL_STATE_UNUSED once all the device->host data transfer is complete.
    if (kernel_status[KERNEL_ID_0].kernel_state == KERNEL_STATE_COMPLETE)
    {
        kernel_status[KERNEL_ID_0].kernel_state = KERNEL_STATE_UNUSED;
    }
}

static void handle_timer_events(void)
{
    // We will need watchdog style timeouts on kernel launches, etc. so we can detect when something's
    // gone wrong and trigger an abort/cleanup.
}

static void launch_kernel(uint64_t shire_mask, uint64_t entry_addr, const kernel_params_t* const kernel_params_ptr, const grid_config_t* const grid_config_ptr)
{
    kernel_status_t* const kernel_status_ptr = &kernel_status[kernel_params_ptr->kernel_id];
    int64_t num_shires = 0;
    bool allShiresReady = true;

    // Confirm that all the shires this kernel wants to use are ready
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        if (shire_mask & (1ULL << shire))
        {
            num_shires++;

            if (shire_status[shire].shire_state != SHIRE_STATE_READY)
            {
                allShiresReady = false;
            }
        }
    }

    if (allShiresReady)
    {
        int64_t* const kernel_launch_barriers = (int64_t*)FW_MASTER_TO_WORKER_LAUNCH_BARRIERS;

        // initialize the barrier the shires will use to synchronize with each other before starting the kernel
        atomic_barrier_init(&kernel_launch_barriers[KERNEL_ID_0], num_shires);

        message.id = MESSAGE_ID_KERNEL_LAUNCH;
        message.data[0] = entry_addr;
        message.data[1] = (uint64_t)kernel_params_ptr;
        message.data[2] = (uint64_t)grid_config_ptr;

        if (0 == broadcast_message_send_master(shire_mask, 0xFFFFFFFFFFFFFFFFU, &message))
        {
            printf("launching kernel\r\n");

            kernel_status_ptr->shire_mask = shire_mask;
            kernel_status_ptr->kernel_state = KERNEL_STATE_RUNNING;

            for (uint64_t shire = 0; shire < 33; shire++)
            {
                if (shire_mask & (1ULL << shire))
                {
                    shire_status[shire].shire_state = SHIRE_STATE_RUNNING;
                    shire_status[shire].kernel_id = kernel_params_ptr->kernel_id;
                }
            }
        }
        else
        {
            printf("error launching kernel\r\n");
        }
    }
    else
    {
        printf("aborting kernel launch, not all shires are ready\r\n");
    }
}
