#include "atomic_barrier.h"
#include "build_configuration.h"
#include "cacheops.h"
#include "interrupt.h"
#include "kernel_info.h"
#include "layout.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message.h"
#include "pcie_isr.h"
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
    uint64_t shire_error_mask;
    uint64_t shire_complete_mask;
} kernel_status_t;

typedef struct
{
    kernel_info_t kernel_info;
    kernel_params_t kernel_params;
} kernel_config_t;

// Local state
static shire_status_t shire_status[33];
static kernel_status_t kernel_status[MAX_SIMULTANEOUS_KERNELS];

// Shared state - Worker minion fetch kernel parameters from these
static kernel_config_t kernel_config[MAX_SIMULTANEOUS_KERNELS];

static message_t message;

//#define DEBUG_SEND_MESSAGES_TO_SP
#define DEBUG_REFLECT_MESSAGE_FROM_HOST
//#define DEBUG_FAKE_MESSAGE_FROM_HOST

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
static void fake_message_from_host(void);
#endif

static void handle_message_from_host(void);
static void handle_message_from_sp(void);
static void handle_messages_from_workers(void);
static void handle_message_from_worker(uint64_t shire, uint64_t hart);
static void update_shire_state(uint64_t shire, shire_state_t state);
static void update_kernel_state(kernel_id_t kernel_id, uint64_t shire, shire_state_t shire_state);
static void handle_pcie_events(void);
static void handle_timer_events(void);
static void launch_kernel(const kernel_params_t* const kernel_params_ptr, const kernel_info_t* const kernel_info_ptr);
static void print_log_message(uint64_t hart);

#ifdef DEBUG_SEND_MESSAGES_TO_SP
static uint16_t lfsr(void);
static uint16_t generate_message(uint8_t* const buffer);
#endif

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

    INT_init();

    printf("Initializing mailboxes...");
    MBOX_init();
    printf("done\r\n");

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

#ifdef DEBUG_SEND_MESSAGES_TO_SP
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH] __attribute__((aligned(MBOX_BUFFER_ALIGNMENT)));
    uint16_t length = generate_message(buffer);
#endif

    // Wait for a message from the host, worker minion, PCI-E, etc.
    for (;;)
    {

#ifdef DEBUG_SEND_MESSAGES_TO_SP
        MBOX_update_status(MBOX_SP);

        printf("Sending message to SP, length = %" PRId16 "\r\n", length);

        int64_t result = MBOX_send(MBOX_SP, buffer, length);

        if (result == 0)
        {
            length = generate_message(buffer);
        }
        else
        {
            printf("MBOX_send error %d\r\n, result");
        }
#endif

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
        fake_message_from_host();
#else
        asm volatile ("wfi");
#endif

        if (swi_flag)
        {
            swi_flag = false;

            // Ensure flag clears before messages are handled
            asm volatile ("fence");

            handle_message_from_sp();
            handle_messages_from_workers();
        }
        else
        {
            printf("no swi_flag\r\n");
        }

        // External interrupts
        if (pcie_interrupt_flag)
        {
            printf("PCI-E message interrupt received\r\n");

            pcie_interrupt_flag = false;

            // Ensure flag clears before messages are handled
            asm volatile ("fence");

            handle_message_from_host();
            handle_pcie_events();
        }

        // Timer interrupts
        handle_timer_events();
    }
}

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
static void fake_message_from_host(void)
{
    const kernel_id_t kernel_id = KERNEL_ID_0;

    // For now, fake host launches kernel 0 any time it's unused.
    if (kernel_status[kernel_id].kernel_state == KERNEL_STATE_UNUSED)
    {
        const kernel_params_t kernel_params = {
            .tensor_a = 0,
            .tensor_b = 0,
            .tensor_c = 0,
            .tensor_d = 0,
            .tensor_e = 0,
            .tensor_f = 0,
            .tensor_g = 0,
            .tensor_h = 0,
            .kernel_id = kernel_id
        };

        const kernel_info_t kernel_info = {
            .compute_pc = KERNEL_UMODE_ENTRY,
            .uber_kernel_nodes = 0, // unused
            .shire_mask = 1,
            .kernel_params_ptr = NULL, // gets fixed up
            .grid_config_ptr = NULL // TODO
        };

        printf("faking kernel launch message fom host\r\n");

        launch_kernel(&kernel_params, &kernel_info);
    }
}
#endif

static void handle_message_from_host(void)
{
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH] __attribute__((aligned(MBOX_BUFFER_ALIGNMENT)));
    int64_t length;

    MBOX_update_status(MBOX_PCIE);

    length = MBOX_receive(MBOX_PCIE, buffer, sizeof(buffer));

    if (length > 0)
    {
        printf("Received message from host, length = %" PRId64 "\r\n", length);

        for (int64_t i = 0; i < length; i++)
        {
            printf ("message[%" PRId64 "] = 0x%02" PRIu8 "\r\n", i, buffer[i]);
        }

#ifdef DEBUG_REFLECT_MESSAGE_FROM_HOST
        printf("Reflecting message to host\r\n");
        MBOX_send(MBOX_PCIE, buffer, (uint32_t)length);
#endif

        const mbox_message_id_t* const message_id_ptr = (void*)buffer;

        if (*message_id_ptr == MBOX_MESSAGE_ID_KERNEL_LAUNCH)
        {
            printf("received kernel launch message fom host\r\n");

            // For initial testing, message is { message_id_t, kernel_params_t, kernel_info_t }
            const kernel_params_t* const kernel_params_ptr = (void*)&buffer[sizeof(mbox_message_id_t)];
            const kernel_info_t* const kernel_info_ptr     = (void*)&buffer[sizeof(mbox_message_id_t) +
                                                                            sizeof(kernel_params_t)];

            launch_kernel(kernel_params_ptr, kernel_info_ptr);
        }
    }
}

static void handle_message_from_sp(void)
{
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH] __attribute__((aligned(MBOX_BUFFER_ALIGNMENT)));
    int64_t length;

#ifdef DEBUG_SEND_MESSAGES_TO_SP
    static uint8_t receive_data;
#endif

    MBOX_update_status(MBOX_SP);

    do
    {
        length = MBOX_receive(MBOX_SP, buffer, sizeof(buffer));

        if (length > 0)
        {
            printf("Received message from SP, length = %" PRId64 "\r\n", length);

#ifdef DEBUG_SEND_MESSAGES_TO_SP
            for (int64_t i = 0; i < length; i++)
            {
                uint8_t expected = receive_data++;

                if (buffer[i] != expected)
                {
                    printf ("message[%" PRId64 "] = 0x%02" PRIx8 " expected 0x%02" PRIx8 "\r\n", i, buffer[i], expected);
                }
            }
#endif
        }
    }
    while (length > 0);
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

        case MESSAGE_ID_LOG_WRITE:
            print_log_message(hart);
        break;

        default:
            printf("Unknown message id = 0x%016" PRIx64 "received from shire %" PRId64 " hart %" PRId64 "\r\n", message.id, shire, hart);
        break;
    }
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
            if (kernel_status[kernel_id].shire_complete_mask == kernel_config[kernel_id].kernel_info.shire_mask)
            {
                printf("kernel %d complete\r\n", kernel_id);
                const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_RESULT,
                                             kernel_id,
                                             MBOX_KERNEL_RESULT_OK};

                MBOX_send(MBOX_PCIE, response, sizeof(response));

                kernel_status[kernel_id].kernel_state = KERNEL_STATE_COMPLETE;
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

static void launch_kernel(const kernel_params_t* const kernel_params_ptr, const kernel_info_t* const kernel_info_ptr)
{
    const kernel_id_t kernel_id = kernel_params_ptr->kernel_id;
    const uint64_t shire_mask = kernel_info_ptr->shire_mask;
    kernel_status_t* const kernel_status_ptr = &kernel_status[kernel_id];
    int64_t num_shires = 0;
    bool allShiresReady = true;
    bool kernelReady = true;

    // Confirm that all the shires this kernel wants to use are ready
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        if (shire_mask & (1ULL << shire))
        {
            num_shires++;

            if (shire_status[shire].shire_state != SHIRE_STATE_READY)
            {
                printf("launch_kernel: kernel %d shire %d not ready\r\n", kernel_id, shire);
                allShiresReady = false;
            }
        }
    }

    // Confirm this kernel is not active
    if (kernel_status[kernel_id].kernel_state != KERNEL_STATE_UNUSED)
    {
        printf("launch_kernel: kernel %d state not unused\r\n", kernel_id);
        kernelReady = false;
    }

    if (allShiresReady && kernelReady)
    {
        // Copy params and info into kernel config buffer
        kernel_config[kernel_id].kernel_params = *kernel_params_ptr;
        kernel_config[kernel_id].kernel_info   = *kernel_info_ptr;

        // Fix up kernel_params_ptr for the copy in kernel_config
        kernel_config[kernel_id].kernel_info.kernel_params_ptr = &kernel_config[kernel_id].kernel_params;

        // Evict kernel config to point of coherency - worker minion will read this
        kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];
        evict_va(0, to_L3, (uint64_t)kernel_config_ptr, (sizeof(kernel_config_t) + 63) / 64, 64, 0, 0);
        WAIT_CACHEOPS

        // Initialize the barrier the shires will use to synchronize with each other before launching the kernel
        int64_t* const kernel_launch_barriers = (int64_t*)FW_MASTER_TO_WORKER_LAUNCH_BARRIERS;
        atomic_barrier_init(&kernel_launch_barriers[kernel_id], num_shires);

        // Create the message to broadcast to all the worker minion
        message.id = MESSAGE_ID_KERNEL_LAUNCH;
        message.data[0] = kernel_config[kernel_id].kernel_info.compute_pc;
        message.data[1] = (uint64_t)kernel_config[kernel_id].kernel_info.kernel_params_ptr;
        message.data[2] = (uint64_t)kernel_config[kernel_id].kernel_info.grid_config_ptr;

        if (0 == broadcast_message_send_master(shire_mask, 0xFFFFFFFFFFFFFFFFU, &message))
        {
            printf("launch_kernel: launching kernel %d\r\n", kernel_id);
            const uint8_t response[3] = {MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE,
                                         kernel_id,
                                         MBOX_KERNEL_LAUNCH_RESPONSE_OK};

            MBOX_send(MBOX_PCIE, response, sizeof(response));

            kernel_status_ptr->shire_error_mask = 0;
            kernel_status_ptr->shire_complete_mask = 0;
            kernel_status_ptr->kernel_state = KERNEL_STATE_RUNNING;

            for (uint64_t shire = 0; shire < 33; shire++)
            {
                if (shire_mask & (1ULL << shire))
                {
                    shire_status[shire].shire_state = SHIRE_STATE_RUNNING;
                    shire_status[shire].kernel_id = kernel_id;
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

static void print_log_message(uint64_t hart)
{
    const char* const data_ptr = (char*)message.data;
    const uint8_t length = data_ptr[0];

    printf("log: H%04d: ", hart);
    SERIAL_write(UART0, &data_ptr[1], length);
    SERIAL_write(UART0, "\r\n", 2);
}

#ifdef DEBUG_SEND_MESSAGES_TO_SP
static uint16_t lfsr(void)
{
    static uint16_t lfsr = 0xACE1u;  /* Any nonzero start state will work. */

    for (uint64_t i = 0; i < 16; i++)
    {
        lfsr ^= (uint16_t)(lfsr >> 7U);
        lfsr ^= (uint16_t)(lfsr << 9U);
        lfsr ^= (uint16_t)(lfsr >> 13U);
    }

    return lfsr;
}

// Generates a random length message with a predictable pattern
uint16_t generate_message(uint8_t* const buffer)
{
    static uint8_t transmit_data;
    uint16_t length;

    do {
        length = lfsr() & 0xFF;
    } while ((length == 0) || (length > MBOX_MAX_MESSAGE_LENGTH));

    for (uint64_t i = 0; i < length ; i++)
    {
        buffer[i] = transmit_data++;
    }

    return length;
}
#endif
