#include "device_minion_runtime_build_configuration.h"
#include "cacheops.h"
#include "device-mrt-trace.h"
#include "device_api_privileged.h"
#include "device_api_non_privileged.h"
#include "fcc.h"
#include "hart.h"
#include "interrupt.h"
#include "kernel.h"
#include "layout.h"
#include "log.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message.h"
#include "minion_fw_boot_config.h"
#include "pcie_device.h"
#include "pcie_isr.h"
#include "print_exception.h"
#include "serial.h"
#include "shire.h"
#include "swi.h"
#include "syscall_internal.h"
#include "mm_dev_intf_reg.h"
#include "sync.h"
#include "vqueue.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

//#define DEBUG_PRINT_HOST_MESSAGE
//#define DEBUG_SEND_MESSAGES_TO_SP
//#define DEBUG_FAKE_MESSAGE_FROM_HOST
//#define DEBUG_FAKE_ABORT_FROM_HOST

static global_fcc_flag_t sq_worker_sync[VQUEUE_COUNT] = { 0 };

#ifdef DEBUG_FAKE_MESSAGE_FROM_HOST
#include <esperanto/device-api/device_api.h>
static void fake_message_from_host(void);
#endif

static void master_thread(void);
/// \brief Thread for processing the commands sent by Host in Submission Queues.
/// For each Submission Queue, there is a sq_worker_thread instantiated.
/// \param[in] sq_index: Index of the Submission Queue on which the thread operates.
static void __attribute__((noreturn)) sq_worker_thread(uint32_t sq_index);

/// \brief Handle a command from the Host sent in Submission Queue
/// \param[in] sq_index: Index of submission queue from which the message is to be read.
static int64_t handle_messages_from_host_sq(uint32_t sq_index);

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
        master_thread();
    } else if ((hart_id >= 2050) && (hart_id < 2054)) {
        kernel_sync_thread(hart_id - 2050);
    } else if ((hart_id >= 2054) && (hart_id % 2054 <= VQUEUE_COUNT)) {
        // SQ Workers
        // TODO: VQUEUE_COUNT should come from Device Interface Regs
        sq_worker_thread((uint32_t)(hart_id % 2054));
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

// From third minion in the master shire, SQ workers are initialized
// SQ worker ID 0 maps to minion thread 0, 1 to minion thread 1, etc.
#define FIRST_SQ_WORKER_MINON 3

// Sends a FCC_0 to the appropriate SQ worker thread (HART) for the sq_worker_id
static inline void notify_sq_worker_thread(uint32_t sq_worker_id)
{
    const uint32_t minion = FIRST_SQ_WORKER_MINON + (sq_worker_id / 2);
    const uint32_t thread = sq_worker_id % 2;

    global_fcc_flag_notify(&sq_worker_sync[sq_worker_id], minion, thread);
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

        // Send FCC_0 to SQ handler to process SQ
        // TODO: Using SQ0 thread only for now. Will be updated in VQ WP2
        notify_sq_worker_thread(0U);
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

static void wait_sp_mm_mbox_ready(void)
{
    // TODO: Proper delay, or better to make this async? Does MM FW init depend on SP?
    uint32_t timeout = 10;

    while (timeout > 0) {
        if (MBOX_ready(MBOX_SP)) {
            log_write(LOG_LEVEL_CRITICAL, "\n MM -> SP Mbox ready !\n");
            break;
        }
        --timeout;
    }
}

static MM_DEV_INTF_REG_s *g_master_min_dev_intf_reg = (void *)DEV_INTF_BASE_ADDR;

static void dev_interface_reg_init(void)
{
    uint64_t next_offset_;
    g_master_min_dev_intf_reg->version                                   = DEV_INTF_REG_VERSION;
    g_master_min_dev_intf_reg->size                                      = sizeof(MM_DEV_INTF_REG_s);
    g_master_min_dev_intf_reg->mm_vq_chan                                = MM_VQ_CHANNEL;

    for (uint8_t i=0; i< MM_VQ_CHANNEL; i++) {

       next_offset_ = MM_VQ_OFFSET + (i*MM_VQ_SIZE);

       g_master_min_dev_intf_reg->mm_vq[i].bar                           = MM_VQ_BAR;
       g_master_min_dev_intf_reg->mm_vq[i].offset                        = next_offset_;
       g_master_min_dev_intf_reg->mm_vq[i].size                          = MM_VQ_SIZE;
    }

    g_master_min_dev_intf_reg->ddr_region[MAP_USER_KERNEL_SPACE].attr    = ATTR_READ_WRITE;
    g_master_min_dev_intf_reg->ddr_region[MAP_USER_KERNEL_SPACE].bar     = USER_KERNEL_SPACE_BAR;
    g_master_min_dev_intf_reg->ddr_region[MAP_USER_KERNEL_SPACE].offset  = USER_KERNEL_SPACE_OFFSET;
    g_master_min_dev_intf_reg->ddr_region[MAP_USER_KERNEL_SPACE].size    = USER_KERNEL_SPACE_SIZE;
    // Update Status to indicate MM VQ is ready to use
    g_master_min_dev_intf_reg->status                                    = STAT_DEV_INTF_READY_INITIALIZED;
}

static void __attribute__((noreturn)) sq_worker_thread(uint32_t sq_index)
{
    // Flag for pending SQs
    bool sq_pending;
    int64_t temp;

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while (1) {
        // wait for notification from MM
        global_fcc_flag_wait(&sq_worker_sync[sq_index]);

        // VQUEUE_SQ_HP_ID is Higher priority Submission Queue. Remaining SQs are of normal priority.
        // All available commands in VQUEUE_SQ_HP_ID will be processed first, then each command from 
        // remaining SQs will be processed in Round-Robin fashion with a single command processing at a time.
        sq_pending = true;
        
        while (sq_pending) {
            uint32_t i;

            // TODO: VQUEUE_COUNT should come from Device Interface Regs
            for (i = 0, sq_pending = false; i < VQUEUE_COUNT; i++) {
                if (i == VQUEUE_SQ_HP_ID) {
                    // Process all commands from SQ0
                    do {
                        temp = handle_messages_from_host_sq(i);
                    } while ((temp >= 0) || (temp == VQ_ERROR_CQ_FULL));
                } else {
                    // Process rest of the SQs with equal weights
                    temp = handle_messages_from_host_sq(i);
                    if ((temp >= 0) || (temp == VQ_ERROR_CQ_FULL)) {
                        sq_pending = true;
                    }
                }
            }
        }
    }
}

static void __attribute__((noreturn)) master_thread(void)
{
    uint64_t temp;
    volatile minion_fw_boot_config_t *boot_config =
        (volatile minion_fw_boot_config_t *)FW_MINION_FW_BOOT_CONFIG;
    uint64_t boot_minion_shires = boot_config->minion_shires & ((1ULL << NUM_SHIRES) - 1);

    // Ensure that FCC global flags for SQ workers sync notifications are initialized. 
    for (uint8_t i = 0; i < VQUEUE_COUNT; i++) {
        global_fcc_flag_init(&sq_worker_sync[i]);
    }

    SERIAL_init(UART0);
    log_write(LOG_LEVEL_CRITICAL, "\r\nMaster minion " GIT_VERSION_STRING "\r\n");

    INT_init();

    dev_interface_reg_init();

    log_write(LOG_LEVEL_CRITICAL, "Master: Initializing trace subsystem...");
    TRACE_init_master();
    log_write(LOG_LEVEL_CRITICAL, "done\r\n");
    TRACE_string(LOG_LEVELS_CRITICAL, "Trace message from Master minion");

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
    // TODO: Should we block wait for this?
    wait_sp_mm_mbox_ready();

    // Initialize VQs
    VQUEUE_init();
    log_write(LOG_LEVEL_CRITICAL, "MM queues to Host initialzed\r\n");
    
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

static int64_t handle_messages_from_host_sq(uint32_t vq_index)
{
    static uint8_t buffer[CIRCBUFFER_SIZE]
        __attribute__((aligned(8))) = { 0 };
    int64_t length;

    VQUEUE_update_status(vq_index);

    // Only pop from SQ when CQ has space for new respnonse handling
    if (VQUEUE_full(CQ, vq_index)) {
        return VQ_ERROR_CQ_FULL;
    }

    length = VQUEUE_pop(SQ, vq_index, buffer, sizeof(buffer));

    if (length > 0) {
        // TODO: MBOX references to be changed in VQ_WP2
        const mbox_message_id_t *const message_id = (const void *const)buffer;

        if (*message_id == MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_CMD) {
            const struct reflect_test_cmd_t* const cmd = (const void* const) buffer;
            struct reflect_test_rsp_t rsp;
            rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_RSP;
            prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
            int64_t result = VQUEUE_push(CQ, vq_index, &rsp, sizeof(rsp));
            if (result != 0) {
                log_write(LOG_LEVEL_ERROR, "DeviceAPI Reflect Test send error %" PRIi64 "\r\n", result);
            }
        } else {
            log_write(LOG_LEVEL_ERROR, "Invalid message id: %" PRIu64 "\r\n", *message_id);

#ifdef DEBUG_PRINT_HOST_MESSAGE
            print_host_message(buffer, length);
#endif
        }
    }
    return length;
}

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

    if (MBOX_DEVAPI_PRIVILEGED_MID_NONE < *message_id
        && *message_id < MBOX_DEVAPI_PRIVILEGED_MID_LAST) {
        handle_device_api_privileged_message_from_host(message_id, buffer);
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

    switch (message.header.id) {
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
        update_kernel_state(((message_kernel_launch_ack_t *)&message)->kernel_id,
                            KERNEL_STATE_RUNNING);
        break;

    case MESSAGE_ID_KERNEL_LAUNCH_NACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        update_shire_state(shire, SHIRE_STATE_ERROR);
        update_kernel_state(((message_kernel_launch_nack_t *)&message)->kernel_id,
                            KERNEL_STATE_ERROR);
        break;

    case MESSAGE_ID_KERNEL_ABORT_NACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        update_shire_state(shire, SHIRE_STATE_ERROR);
        update_kernel_state(kernel, KERNEL_STATE_ERROR);
        break;

    case MESSAGE_ID_KERNEL_COMPLETE:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_KERNEL_COMPLETE received from shire %" PRId64 " hart %" PRId64 "\r\n",
                  shire, hart);
        update_kernel_state(((message_kernel_launch_completed_t *)&message)->kernel_id,
                            KERNEL_STATE_COMPLETE);
        break;

    case MESSAGE_ID_LOOPBACK:
        log_write(LOG_LEVEL_DEBUG,
                  "MESSAGE_ID_LOOPBACK received from shire %" PRId64 " hart %" PRId64 "\r\n", shire,
                  hart);
        break;

    case MESSAGE_ID_FW_EXCEPTION: {
        message_exception_t *exception = (message_exception_t *)&message;
        print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
                        exception->hart_id);
        // non-kernel exceptions are unrecoverable. Put the shire in error state
        update_shire_state(shire, SHIRE_STATE_ERROR);
        update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
        break;
    }

    case MESSAGE_ID_U_MODE_EXCEPTION: {
        message_exception_t *exception = (message_exception_t *)&message;
        print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
                        exception->hart_id);
        update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
        break;
    }

    case MESSAGE_ID_LOG_WRITE:
        // Always print messages coming from workers
        print_log_message(shire, hart, &message);
        break;

    case MESSAGE_ID_SET_LOG_LEVEL:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_SET_LOG_LEVEL received from shire %" PRId64 " hart %" PRId64
                  "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_TRACE_UPDATE_CONTROL:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_UPDATE_TRACE_CONTROL received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_TRACE_BUFFER_RESET:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_TRACE_BUFFER_RESET received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_TRACE_BUFFER_EVICT:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_TRACE_BUFFER_EVICT received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  shire, hart);
        break;

    case MESSAGE_ID_PMC_CONFIGURE:
        log_write(LOG_LEVEL_WARNING,
                  "Invalid MESSAGE_ID_PMC_CONFGIURE received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  shire, hart);
        break;

    default:
        log_write(LOG_LEVEL_WARNING,
                  "Unknown message id = 0x%016" PRIx64 " received from shire %" PRId64
                  " hart %" PRId64 "\r\n",
                  message.header.id, shire, hart);
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
