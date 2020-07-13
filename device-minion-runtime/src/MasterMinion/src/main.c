#include "build_configuration.h"
#include "fcc.h"
#include "cacheops.h"
#include "device_api_non_privileged.h"
#include "hart.h"
#include "host_message.h"
#include "interrupt.h"
#include "kernel.h"
#include "layout.h"
#include "log.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message.h"
#include "minion_fw_boot_config.h"
#include "pcie_device.h"
#include "pcie_dma.h"
#include "pcie_isr.h"
#include "print_exception.h"
#include "serial.h"
#include "shire.h"
#include "swi.h"
#include "syscall_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

//#define DEBUG_PRINT_HOST_MESSAGE
//#define DEBUG_SEND_MESSAGES_TO_SP
//#define DEBUG_FAKE_MESSAGE_FROM_HOST
//#define DEBUG_FAKE_ABORT_FROM_HOST

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
#include <esperanto/device-api/device_api.h>
static void fake_message_from_host(void);
#endif

static void master_thread(void);

static void handle_messages_from_host(void);

/// \brief Handle a command from the host
/// \param[in] length: Size of the message in bytes
/// \param[in] buffer: Pointer to the data. The buffer is not made const in purpose
///             to allow us to modify the contents of the message upon arrive and record
////            necessary additional information, like timestamps
static void handle_message_from_host(int64_t length, uint8_t *buffer);

static void handle_messages_from_sp(void);
static void handle_message_from_sp(int64_t length, const uint8_t *const buffer);

static void handle_messages_from_workers(void);
static void handle_message_from_worker(uint64_t shire, uint64_t hart);

static void handle_pcie_events(void);
static void handle_timer_events(void);

#ifdef DEBUG_PRINT_HOST_MESSAGE
static void print_host_message(const uint8_t *const buffer, int64_t length);
#endif

static void print_log_message(uint64_t shire, uint64_t hart, const message_t *const message);

#ifdef DEBUG_SEND_MESSAGES_TO_SP
static uint16_t lfsr(void);
static uint16_t generate_message(uint8_t *const buffer);
#endif

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    // Configure supervisor trap vector and sscratch (supervisor stack pointer)
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n" // supervisor trap vector
                 : "=&r"(temp));

    const uint64_t hart_id = get_hart_id();

    if (hart_id == 2048) {
        // Enable thread 1 on minion 1 and 2 for kernel sync fw-threads
        // Also enable thread 1 of minions 16-31 (worker minions user-mode kernel sync minions)
        syscall(SYSCALL_ENABLE_THREAD1_INT, 0, 0xFFFF0000u | 6u, 0);

        master_thread();
    } else if ((hart_id >= 2050) && (hart_id < 2054)) {
        kernel_sync_thread(hart_id - 2050);
    } else {
        while (1) {
            asm volatile("wfi");
        }
    }
}

static inline void check_and_handle_sp_and_worker_messages(void)
{
    if (swi_flag) {
        swi_flag = false;

        // Ensure flag clears before messages are handled
        asm volatile("fence");

        handle_messages_from_sp();
        handle_messages_from_workers();
    }
}

static inline void check_and_handle_host_messages_and_pcie_events(void)
{
    // External interrupts
    if (pcie_interrupt_flag) {
        pcie_interrupt_flag = false;

        // Ensure flag clears before messages are handled
        asm volatile("fence");

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
        fake_message_from_host();
#endif

        handle_messages_from_host();
        handle_pcie_events();
    }
}

static void wait_all_shires_booted(uint64_t expected)
{
    while (1) {
        if (all_shires_booted(expected))
            break;

        asm volatile("csrci sstatus, 0x2"); // Disable supervisor interrupts

        if (!swi_flag) {
            asm volatile("wfi");
        }

        asm volatile("csrsi sstatus, 0x2"); // Enable supervisor interrupts

        check_and_handle_sp_and_worker_messages();
        handle_timer_events();
    }
}

static void wait_sp_mm_mbox_ready(mbox_e mbox)
{
    // Wait for 20 seconds
    uint32_t timeout = 1000000;

    while (timeout > 0) {
        if (MBOX_ready(mbox)) {
            log_write(LOG_LEVEL_CRITICAL, "\n MM -> SP Mbox ready !\n");
            break;
        }
        --timeout;
    }
}

static void __attribute__((noreturn)) master_thread(void)
{
    uint64_t temp;
    volatile minion_fw_boot_config_t *boot_config =
        (volatile minion_fw_boot_config_t *)FW_MINION_FW_BOOT_CONFIG;
    uint64_t boot_minion_shires = boot_config->minion_shires & ((1ULL << NUM_SHIRES) - 1);

    SERIAL_init(UART0);
    log_write(LOG_LEVEL_CRITICAL, "\r\nMaster minion " GIT_VERSION_STRING "\r\n");

    INT_init();

    log_write(LOG_LEVEL_INFO, "Initializing message buffers...");
    message_init_master();
    log_write(LOG_LEVEL_INFO, "done\r\n");

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    kernel_init();

    log_write(LOG_LEVEL_INFO, "Boot config Minion Shires: 0x%" PRIx64 "\n", boot_minion_shires);

    // Enable supervisor external and software interrupts
    asm volatile("li    %0, 0x202    \n"
                 "csrs  sie, %0      \n" // Enable supervisor external and software interrupts
                 "csrsi sstatus, 0x2 \n" // Enable interrupts
                 : "=&r"(temp));

    // Bring up Compute Minions
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, boot_minion_shires, 0x1u, 0);

    log_write(LOG_LEVEL_INFO, "All Compute Minions configured!\n");

    // Wait until all Shires have booted before starting the main loop that handles PCIe messages
    wait_all_shires_booted(boot_minion_shires);
    log_write(LOG_LEVEL_CRITICAL, "All Shires (0x%" PRIx64 ") ready!\n", boot_minion_shires);

    // Wait for SP -> MM Mbox being ready
    wait_sp_mm_mbox_ready(MBOX_SP);

    // Indicate to Host MM is ready to accept new commands
    MBOX_init();
    log_write(LOG_LEVEL_CRITICAL, "Mailbox to Host initialzed\r\n");

#ifdef DEBUG_SEND_MESSAGES_TO_SP
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH]
        __attribute__((aligned(MBOX_BUFFER_ALIGNMENT))) = { 0 };
    uint16_t length = generate_message(buffer);
#endif

    // Wait for a message from the host, worker minion, PCI-E, etc.
    while (1) {
#ifdef DEBUG_SEND_MESSAGES_TO_SP
        MBOX_update_status(MBOX_SP);

        log_write(LOG_LEVEL_DEBUG, "Sending message to SP, length = %" PRId16 "\r\n", length);

        int64_t result = MBOX_send(MBOX_SP, buffer, length);

        if (result == 0) {
            length = generate_message(buffer);
        } else {
            log_write(LOG_LEVEL_DEBUG, "MBOX_send error %d\r\n, result");
        }
#endif

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
        pcie_interrupt_flag = true;
#endif

        asm volatile("csrci sstatus, 0x2"); // Disable supervisor interrupts

        bool event_pending = swi_flag || pcie_interrupt_flag;

        if (!event_pending) {
            asm volatile("wfi");
        }

        asm volatile("csrsi sstatus, 0x2"); // Enable supervisor interrupts

        check_and_handle_sp_and_worker_messages();
        check_and_handle_host_messages_and_pcie_events();
        handle_timer_events();
    }
}

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
static void fake_message_from_host(void)
{
    const kernel_id_t kernel_id = KERNEL_ID_1;

    const kernel_state_t kernel_state = get_kernel_state(kernel_id);

    // For now, fake host launches kernel 0 any time it's unused.
    if (kernel_state == KERNEL_STATE_UNUSED) {
        const struct kernel_launch_cmd_t launch_cmd = {
            .command_info = {
                .message_id = MESSAGE_ID_KERNEL_LAUNCH,
                .command_id = 0,
                .host_timestamp = 0,
                .device_timestamp_mtime = 0,
                .stream_id = 0
            },
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
                .shire_mask = 0x1, //0xFFFFFFFF
            },
            .uber_kernel = 0 // Don't care, unused
        };

        log_write(LOG_LEVEL_DEBUG, "faking kernel launch message fom host\r\n");

        launch_kernel(&launch_cmd);
    }

#ifdef DEBUG_FAKE_ABORT_FROM_HOST
    if ((kernel_state == KERNEL_STATE_LAUNCHED)) || (kernel_state == KERNEL_STATE_RUNNING))
        {
            log_write(LOG_LEVEL_CRITICAL, "faking kernel abort message fom host\r\n");

            abort_kernel(kernel_id);
        }
#endif
}
#endif

static void handle_messages_from_host(void)
{
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH]
        __attribute__((aligned(MBOX_BUFFER_ALIGNMENT))) = { 0 };
    int64_t length;

    MBOX_update_status(MBOX_PCIE);

    do {
        length = MBOX_receive(MBOX_PCIE, buffer, sizeof(buffer));

        if (length > 0) {
            handle_message_from_host(length, buffer);
        }
    } while (length > 0);
}

static void handle_message_from_host(int64_t length, uint8_t *buffer)
{
    if (length < 0) {
        return;
    }

    if ((size_t)length < sizeof(mbox_message_id_t)) {
        log_write(LOG_LEVEL_ERROR, "Invalid message: length = %" PRId64 ", min length %d\r\n",
                  length, sizeof(mbox_message_id_t));
        return;
    }

    const mbox_message_id_t *const message_id = (const void *const)buffer;

    if (*message_id == MBOX_MESSAGE_ID_REFLECT_TEST) {
        MBOX_send(MBOX_PCIE, buffer, (uint32_t)length);
    } else if (*message_id == MBOX_MESSAGE_ID_DMA_RUN_TO_DONE) {
        int rc;

        //Starts the DMA engine, and blocks for the DMA to complete.
        const dma_run_to_done_message_t *const message = (const void *const)buffer;

        dma_done_message_t done_message = {
            .message_id = MBOX_MESSAGE_ID_DMA_DONE,
            .chan = message->chan,
            .status = 0,
        };

        if (message->chan <= DMA_CHAN_READ_3) {
            rc = dma_configure_read(message->chan);
        } else if (message->chan >= DMA_CHAN_WRITE_0 && message->chan <= DMA_CHAN_WRITE_3) {
            rc = dma_configure_write(message->chan);
        } else {
            rc = -1;
        }

        if (rc != 0) {
            log_write(LOG_LEVEL_ERROR, "Failed to configure DMA chan %d (errno %d)\r\n",
                      message->chan, rc);
            done_message.status = rc;
            MBOX_send(MBOX_PCIE, &done_message, sizeof(done_message));
            return;
        }

        dma_start(message->chan);
        while (!dma_check_done(message->chan))
            ; //TODO: setup DMA ISR, wait using that and MBOX_send from there instead
        dma_clear_done(message->chan);

        //TODO: does not deal with error conditions that could abort DMA transfer at all.

        MBOX_send(MBOX_PCIE, &done_message, sizeof(done_message));

        //TODO: notify glow kernel HARTs that data is done being transferred
    } else if (MBOX_DEVAPI_NON_PRIVILEGED_MID_NONE < *message_id
             && *message_id < MBOX_DEVAPI_NON_PRIVILEGED_MID_LAST) {
        handle_device_api_non_privileged_message_from_host(message_id, buffer);
    } else {
        log_write(LOG_LEVEL_ERROR, "Invalid message id: %" PRIu64 "\r\n", *message_id);

#ifdef DEBUG_PRINT_HOST_MESSAGE
        print_host_message(buffer, length);
#endif
    }
}

static void handle_messages_from_sp(void)
{
    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH]
        __attribute__((aligned(MBOX_BUFFER_ALIGNMENT))) = { 0 };
    int64_t length;

    MBOX_update_status(MBOX_SP);

    do {
        length = MBOX_receive(MBOX_SP, buffer, sizeof(buffer));

        if (length > 0) {
            handle_message_from_sp(length, buffer);
        }

    } while (length > 0);
}

static void handle_message_from_sp(int64_t length, const uint8_t *const buffer)
{
    log_write(LOG_LEVEL_INFO, "Received message from SP, length = %" PRId64 "\r\n", length);

#ifdef DEBUG_SEND_MESSAGES_TO_SP
    static uint8_t receive_data = 0;

    for (int64_t i = 0; i < length; i++) {
        uint8_t expected = receive_data++;

        if (buffer[i] != expected) {
            log_write(LOG_LEVEL_INFO,
                      "message[%" PRId64 "] = 0x%02" PRIx8 " expected 0x%02" PRIx8 "\r\n", i,
                      buffer[i], expected);
        }
    }
#else
    (void)buffer;
#endif
}

static void handle_messages_from_workers(void)
{
    // Check for messages from every hart in every shire
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
        const uint64_t flags = get_message_flags(shire);

        if (flags) {
            for (uint64_t hart = 0; hart < 64; hart++) {
                if (flags & (1ULL << hart)) {
                    handle_message_from_worker(shire, hart);
                }
            }
        }
    }
}

static void handle_message_from_worker(uint64_t shire, uint64_t hart)
{
    static message_t message = { 0 };
    const kernel_id_t kernel = get_shire_kernel_id(shire);

    message_receive_master(shire, hart, &message);

    switch (message.id) {
    case MESSAGE_ID_NONE:
        log_write(LOG_LEVEL_DEBUG,
                  "Invalid MESSAGE_ID_NONE received from shire %" PRId64 " hart %" PRId64 "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_SHIRE_READY:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_SHIRE_READY received from shire %" PRId64 " hart %" PRId64 "\r\n",
                  shire, hart);
        update_shire_state(shire, SHIRE_STATE_READY);
        break;

    case MESSAGE_ID_KERNEL_LAUNCH:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_KERNEL_LAUNCH received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_KERNEL_ABORT:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_KERNEL_ABORT received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_KERNEL_LAUNCH_ACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_LAUNCH_ACK received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        update_kernel_state(message.data[0], KERNEL_STATE_RUNNING);
        break;

    case MESSAGE_ID_KERNEL_LAUNCH_NACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        update_shire_state(shire, SHIRE_STATE_ERROR);
        update_kernel_state(message.data[0], KERNEL_STATE_ERROR);
        break;

    case MESSAGE_ID_KERNEL_ABORT_NACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        update_shire_state(shire, SHIRE_STATE_ERROR);
        update_kernel_state(kernel, KERNEL_STATE_ERROR);
        break;

    case MESSAGE_ID_KERNEL_COMPLETE: {
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_COMPLETE received from shire %" PRId64 " hart %" PRId64 "\r\n",
                  shire, hart);
        update_kernel_state(message.data[0], KERNEL_STATE_COMPLETE);
    } break;

    case MESSAGE_ID_LOOPBACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_LOOPBACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire,
                  hart);
        break;

    case MESSAGE_ID_EXCEPTION:
        print_exception(message.data[1], message.data[2], message.data[3], message.data[4],
                        message.data[0]);
        update_shire_state(
            shire,
            SHIRE_STATE_ERROR); // non-kernel exceptions are unrecoverable. Put the shire in error state
        update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
        break;

    case MESSAGE_ID_KERNEL_EXCEPTION:
        print_exception(message.data[1], message.data[2], message.data[3], message.data[4],
                        message.data[0]);
        update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
        break;

    case MESSAGE_ID_LOG_WRITE:
        if (get_log_level() > LOG_LEVEL_INFO) {
            print_log_message(shire, hart, &message);
        }
        break;

    case MESSAGE_ID_SET_LOG_LEVEL:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_SET_LOG_LEVEL received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        break;

    default:
        log_write(LOG_LEVEL_WARNING,
                  "Unknown message id = 0x%016" PRIx64 "received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  message.id, shire, hart);
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

#ifdef DEBUG_PRINT_HOST_MESSAGE
static void print_host_message(const uint8_t *const buffer, int64_t length)
{
    for (int64_t i = 0; i < length / 8; i++) {
        log_write(LOG_LEVEL_INFO, "message[%" PRId64 "] = 0x%016" PRIx64 "\r\n", i,
                  ((const uint64_t *const)(const void *const)buffer)[i]);
    }
}
#endif

static void print_log_message(uint64_t shire, uint64_t hart, const message_t *const message)
{
    const char *const string_ptr = (const char *const)message->data;

    // messages are passed with shire and 0-63 intra-shire hart index - convert back to global 0-2112 hart index
    uint64_t hart_id = (shire * HARTS_PER_SHIRE) + hart;

    // Print all messages receives - worker minion have their own warning level threshold.
    // Limit length of displayed string in case we receive garbage
    log_write(LOG_LEVEL_CRITICAL, "H%04" PRId64 ": %.*s\r\n", hart_id, sizeof(message->data) - 1,
              string_ptr);
}

#ifdef DEBUG_SEND_MESSAGES_TO_SP
static uint16_t lfsr(void)
{
    static uint16_t lfsr = 0xACE1u; /* Any nonzero start state will work. */

    for (uint64_t i = 0; i < 16; i++) {
        lfsr ^= (uint16_t)(lfsr >> 7U);
        lfsr ^= (uint16_t)(lfsr << 9U);
        lfsr ^= (uint16_t)(lfsr >> 13U);
    }

    return lfsr;
}

// Generates a random length message with a predictable pattern
uint16_t generate_message(uint8_t *const buffer)
{
    static uint8_t transmit_data = 0;
    uint16_t length;

    do {
        length = lfsr() & 0xFF;
    } while ((length == 0) || (length > MBOX_MAX_MESSAGE_LENGTH));

    for (uint64_t i = 0; i < length; i++) {
        buffer[i] = transmit_data++;
    }

    return length;
}
#endif
