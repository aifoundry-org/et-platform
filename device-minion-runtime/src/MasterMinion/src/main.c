#include "build_configuration.h"
#include "hart.h"
#include "host_message.h"
#include "interrupt.h"
#include "kernel.h"
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
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>


//#define DEBUG_SEND_MESSAGES_TO_SP
//#define DEBUG_FAKE_MESSAGE_FROM_HOST

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
static void fake_message_from_host(void);
#endif

static void master_thread(void);

static void handle_message_from_host(void);
static void handle_message_from_sp(void);
static void handle_messages_from_workers(void);
static void handle_message_from_worker(uint64_t shire, uint64_t hart);
static void handle_pcie_events(void);
static void handle_timer_events(void);

static void print_log_message(uint64_t hart, const message_t* const message);

#ifdef DEBUG_SEND_MESSAGES_TO_SP
static uint16_t lfsr(void);
static uint16_t generate_message(uint8_t* const buffer);
#endif

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    // Configure supervisor trap vector and sscratch (supervisor stack pointer)
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n" // supervisor trap vector
        "csrw  sscratch, sp     \n" // Initial saved stack pointer points to S-mode stack scratch region
        : "=&r" (temp)
    );

    const uint64_t hart_id = get_hart_id();

    if (hart_id == 2048)
    {
        // Enable thread 1 on minion 1 and 2 for kernel sync threads
        syscall(SYSCALL_ENABLE_THREAD1, 0xFFFFFFFC, 0, 0);

        master_thread();
    }
    else if ((hart_id >= 2050) && (hart_id < 2054))
    {
        kernel_sync_thread(hart_id - 2050);
    }
    else
    {
        while (1)
        {
            asm volatile ("wfi");
        }
    }
}

static void __attribute__((noreturn)) master_thread(void)
{
    uint64_t temp;

    SERIAL_init(UART0);
    printf("\r\nMaster minion " GIT_VERSION_STRING "\r\n");

    INT_init();

    printf("Initializing mailboxes...");
    MBOX_init();
    printf("done\r\n");

    printf("Initializing message buffers...");
    message_init_master();
    printf("done\r\n");

    kernel_init();

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
        pcie_interrupt_flag = true;
#endif

        asm volatile("csrci sstatus, 0x2"); // Disable supervisor interrupts

        bool event_pending = swi_flag || pcie_interrupt_flag;

        if (!event_pending)
        {
            asm volatile("wfi");
        }

        asm volatile("csrsi sstatus, 0x2"); // Enable supervisor interrupts

        if (swi_flag)
        {
            swi_flag = false;

            // Ensure flag clears before messages are handled
            asm volatile ("fence");

            handle_message_from_sp();
            handle_messages_from_workers();
        }

        // External interrupts
        if (pcie_interrupt_flag)
        {
            pcie_interrupt_flag = false;

            // Ensure flag clears before messages are handled
            asm volatile ("fence");

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
            fake_message_from_host();
#endif

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
    const kernel_id_t kernel_id = KERNEL_ID_1;

    // For now, fake host launches kernel 0 any time it's unused.
    if (get_kernel_state(kernel_id) == KERNEL_STATE_UNUSED)
    {
        const host_message_t host_message = {
            .message_id = MBOX_MESSAGE_ID_KERNEL_LAUNCH,
            .kernel_params = {
                .tensor_a = 0,
                .tensor_b = 0,
                .tensor_c = 0,
                .tensor_d = 0,
                .tensor_e = 0,
                .tensor_f = 0,
                .tensor_g = 0,
                .tensor_h = 0,
                .kernel_id = kernel_id
            },
            .kernel_info = {
                .compute_pc = KERNEL_UMODE_ENTRY,
                .uber_kernel_nodes = 0, // unused
                .shire_mask = 1,
                .kernel_params_ptr = NULL, // gets fixed up
                .grid_config_ptr = NULL // TODO
            }
        };

        printf("faking kernel launch message fom host\r\n");

        launch_kernel(&host_message.kernel_params, &host_message.kernel_info);
    }
}
#endif

static void handle_message_from_host(void)
{
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH] __attribute__((aligned(MBOX_BUFFER_ALIGNMENT)));
    int64_t length;

    MBOX_update_status(MBOX_PCIE);

    length = MBOX_receive(MBOX_PCIE, buffer, sizeof(buffer));

    if (length < 0)
    {
        return;
    }

    if ((size_t)length < sizeof(mbox_message_id_t))
    {
        printf("Invalid message: length = %" PRId64 ", min length %d\r\n", length, sizeof(mbox_message_id_t));
        return;
    }

    const mbox_message_id_t* const message_id = (void*)buffer;

    if (*message_id == MBOX_MESSAGE_ID_KERNEL_LAUNCH)
    {
        printf("received kernel launch message fom host , length = %" PRId64 "\r\n", length);
        for (int64_t i = 0; i < length / 8; i++)
        {
            printf ("message[%" PRId64 "] = 0x%016" PRIx64 "\r\n", i, ((uint64_t*)(void*)buffer)[i] );
        }

        const host_message_t* const host_message_ptr = (void*)buffer;

        launch_kernel(&host_message_ptr->kernel_params, &host_message_ptr->kernel_info);
    }
    else if (*message_id == MBOX_MESSAGE_ID_REFLECT_TEST)
    {
        MBOX_send(MBOX_PCIE, buffer, (uint32_t)length);
    }
    else
    {
        printf("Invalid message id: %" PRIu64 "\r\n", *message_id);

        for (int64_t i = 0; i < length / 8; i++)
        {
            printf ("message[%" PRId64 "] = 0x%016" PRIx64 "\r\n", i, ((uint64_t*)(void*)buffer)[i] );
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
    static message_t message;
    const kernel_id_t kernel = get_shire_kernel_id(shire);

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

        case MESSAGE_ID_KERNEL_LAUNCH_ACK:
            printf("MESSAGE_ID_KERNEL_LAUNCH_ACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_kernel_state(message.data[0], KERNEL_STATE_RUNNING);
        break;

        case MESSAGE_ID_KERNEL_LAUNCH_NACK:
            printf("MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_ERROR);
            update_kernel_state(message.data[0], KERNEL_STATE_ERROR);
        break;

        case MESSAGE_ID_KERNEL_ABORT_NACK:
            printf("MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_shire_state(shire, SHIRE_STATE_ERROR);
            update_kernel_state(kernel, KERNEL_STATE_ERROR);
        break;

        case MESSAGE_ID_KERNEL_COMPLETE:
        {
            printf("MESSAGE_ID_KERNEL_COMPLETE received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
            update_kernel_state(message.data[0], KERNEL_STATE_COMPLETE);
        }
        break;

        case MESSAGE_ID_LOOPBACK:
            printf("MESSAGE_ID_LOOPBACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire, hart);
        break;

        case MESSAGE_ID_EXCEPTION:
            print_exception(message.data[1], message.data[2], message.data[3], message.data[4], message.data[0]);
            update_shire_state(shire, SHIRE_STATE_ERROR);
            update_kernel_state(kernel, KERNEL_STATE_ERROR);
        break;

        case MESSAGE_ID_LOG_WRITE:
            print_log_message(hart, &message);
        break;

        default:
            printf("Unknown message id = 0x%016" PRIx64 "received from shire %" PRId64 " hart %" PRId64 "\r\n", message.id, shire, hart);
        break;
    }
}

static void handle_pcie_events(void)
{
    // Keep the PCI-E data pump going and update kernel state as needed, i.e. transitioning a kernel
    // from KERNEL_STATE_COMPLETE to KERNEL_STATE_UNUSED once all the device->host data transfer is complete.
}

static void handle_timer_events(void)
{
    // We will need watchdog style timeouts on kernel launches, etc. so we can detect when something's
    // gone wrong and trigger an abort/cleanup.
}

static void print_log_message(uint64_t hart, const message_t* const message)
{
    const char* const data_ptr = (const char* const)message->data;
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
