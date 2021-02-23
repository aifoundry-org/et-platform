#include "device_minion_runtime_build_configuration.h"
#include "cacheops.h"
#include "cm_to_mm_iface.h"
#include "device-mrt-trace.h"
#include "device_api_privileged.h"
#include "device_api_non_privileged.h"
#include "device_ops_api.h"
#include "fcc.h"
#include "hart.h"
#include "interrupt.h"
#include "kernel.h"
#include "layout.h"
#include "log.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "message_types.h"
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

#define DEBUG_PRINT_HOST_MESSAGE

// From third minion in the master shire, SQ workers are initialized
// SQ worker ID 0 maps to minion thread 0, 1 to minion thread 1, etc.
#define FIRST_SQ_WORKER_MINON 3

static global_fcc_flag_t sq_worker_sync[MM_VQ_COUNT] = { 0 };

extern void message_init_master(void);
static void master_thread(void);
/// \brief Thread for processing the commands sent by Host in Submission Queues.
/// For each Submission Queue, there is a sq_worker_thread instantiated.
/// \param[in] sq_index: Index of the Submission Queue on which the thread operates.
static void __attribute__((noreturn)) sq_worker_thread(uint32_t sq_index);

/// \brief Handle a command from the Host sent in Submission Queue
/// \param[in] sq_index: Index of submission queue from which the message is to be read.
static int8_t handle_message_from_host_sq(uint32_t sq_idx);

static void handle_messages_from_host(void);

/// \brief Handle a command from the host
/// \param[in] length: Size of the message in bytes
/// \param[in] buffer: Pointer to the data. The buffer is not made const in purpose
///             to allow us to modify the contents of the message upon arrive and record
////            necessary additional information, like timestamps
static void handle_message_from_host(int64_t length, uint8_t *buffer);

static void handle_messages_from_sp(void);
static void handle_message_from_sp(int64_t length, const uint8_t *const buffer);

static void dispatcher_handle_messages_on_unicast(void);

static void handle_pcie_events(void);
static void handle_timer_events(void);

#ifdef DEBUG_PRINT_HOST_MESSAGE
static void print_host_message(const uint8_t *const buffer, int64_t length);
#endif

extern void main2(void);

void __attribute__((noreturn)) main(void)
{
    /* This macro to route execution control to the new implementation
       of MM runtime comes as a build-time param from cmake. */
#ifdef IMPLEMENTATION_BYPASS

    main2();

    (void)master_thread;
    (void)sq_worker_thread;

    while (1) {
        asm volatile("wfi");
    }

#else
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
    } else if ((hart_id >= 2054) && (hart_id % 2054 <= MM_VQ_COUNT)) {
        // SQ Workers
        sq_worker_thread((uint32_t)(hart_id % 2054));
    } else {
        while (1) {
            asm volatile("wfi");
        }
    }

#endif
}

// Sends a FCC_0 to the appropriate SQ worker thread (HART) for the sq_worker_id
static inline void notify_sq_worker_thread(uint32_t sq_worker_id)
{
    const uint32_t minion = FIRST_SQ_WORKER_MINON + (sq_worker_id / 2);
    const uint32_t thread = sq_worker_id % 2;

    global_fcc_flag_notify(&sq_worker_sync[sq_worker_id], minion, thread);
}

static inline void check_and_handle_sp_and_worker_messages(void)
{
    if (swi_flag) {
        swi_flag = false;

        // Ensure flag clears before messages are handled
        asm volatile("fence");

        handle_messages_from_sp();
        dispatcher_handle_messages_on_unicast(); // Coming from Compute FW and KW
    }
}

static inline void check_and_handle_host_messages_and_pcie_events(void)
{
    // External interrupts
    if (pcie_interrupt_flag) {
        pcie_interrupt_flag = false;

        // Ensure flag clears before messages are handled
        asm volatile("fence");

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

static int32_t wait_sp_ready(void)
{
    // TODO: Proper delay, or better to make this async? Or maybe from device interface regs?
    uint32_t timeout = 100000;

    while (timeout > 0) {
        if (MBOX_get_status(MBOX_SP, MBOX_MASTER) == MBOX_STATUS_READY) {
            log_write(LOG_LEVEL_INFO, "\nSP synced!\n");
            return SP_MM_HANDSHAKE_POLL_SUCCESS;
        }
        --timeout;
    }
    // If we reach this point, the SP did reach sync point
    log_write(LOG_LEVEL_ERROR, "\nSP not ready!\n");
    return SP_MM_HANDSHAKE_POLL_TIMEOUT;
}

static volatile MM_DEV_INTF_REG_s *g_mm_dev_intf_reg = (void *)MM_DEV_INTF_BASE_ADDR;

static void mm_dev_interface_reg_init(void)
{
    g_mm_dev_intf_reg->version     = MM_DEV_INTF_REG_VERSION;
    g_mm_dev_intf_reg->size        = sizeof(MM_DEV_INTF_REG_s);
    // Populate the MM VQs information
    g_mm_dev_intf_reg->mm_vq.vq_count   = MM_VQ_COUNT;
    g_mm_dev_intf_reg->mm_vq.bar        = MM_VQ_BAR;
    g_mm_dev_intf_reg->mm_vq.bar_offset = MM_VQ_OFFSET;
    g_mm_dev_intf_reg->mm_vq.bar_size   = MM_VQ_SIZE;
    g_mm_dev_intf_reg->mm_vq.size_info.control_size      = VQUEUE_CONTROL_REGION_SIZE;
    g_mm_dev_intf_reg->mm_vq.size_info.element_count     = VQUEUE_ELEMENT_COUNT;
    g_mm_dev_intf_reg->mm_vq.size_info.element_size      = VQUEUE_ELEMENT_SIZE;
    g_mm_dev_intf_reg->mm_vq.size_info.element_alignment = VQUEUE_ELEMENT_ALIGNMENT;
    for (uint8_t i = 0; i < MM_VQ_COUNT; i++) {
        // TODO: SW-4597: Start from vector one when MSI-X and SP VQ is available. Vector zero is reserved for SP CQ.
        g_mm_dev_intf_reg->mm_vq.interrupt_vector[i] = i;
    }

    g_mm_dev_intf_reg->ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].attr    = MM_DEV_INTF_DDR_REGION_ATTR_READ_WRITE;
    g_mm_dev_intf_reg->ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].bar     = MM_DEV_INTF_USER_KERNEL_SPACE_BAR;
    g_mm_dev_intf_reg->ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].offset  = MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET;
    g_mm_dev_intf_reg->ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].devaddr = HOST_MANAGED_DRAM_START;
    g_mm_dev_intf_reg->ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].size    = MM_DEV_INTF_USER_KERNEL_SPACE_SIZE;

    // Update Status to indicate MM Device Interface Registers are initialized
    g_mm_dev_intf_reg->status = MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY_INITIALIZED;
}

static void __attribute__((noreturn)) sq_worker_thread(uint32_t sq_index)
{
    // Flag for pending SQs
    bool sq_pending;
    int8_t status;

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

            for (i = 0, sq_pending = false; i < MM_VQ_COUNT; i++) {
                if (i == VQUEUE_SQ_HP_ID) {
                    // Process all commands from SQ0
                    do {
                        status = handle_message_from_host_sq(i);
                    } while ((status == 0) || (status == VQ_ERROR_CQ_FULL));
                } else {
                    // Process rest of the SQs with equal weight
                    status = handle_message_from_host_sq(i);
                    if ((status == 0) || (status == VQ_ERROR_CQ_FULL)) {
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
    uint64_t functional_shires = boot_config->minion_shires & ((1ULL << NUM_SHIRES) - 1);

    // Ensure that FCC global flags for SQ workers sync notifications are initialized.
    for (uint8_t i = 0; i < MM_VQ_COUNT; i++) {
        global_fcc_flag_init(&sq_worker_sync[i]);
    }

    SERIAL_init(UART0);
    log_write(LOG_LEVEL_CRITICAL, "\r\nMaster minion " GIT_VERSION_STRING "\r\n");

    mm_dev_interface_reg_init();

    INT_init();
    TRACE_init_master(functional_shires);
    message_init_master();

    TRACE_string(LOG_LEVELS_CRITICAL, "Trace message from Master minion");

    init_fcc(FCC_0);
    init_fcc(FCC_1);

    set_functional_shires(functional_shires);
    kernel_init();

    log_write(LOG_LEVEL_INFO, "Boot config Minion Shires: 0x%" PRIx64 "\n", functional_shires);

    // Enable supervisor external and software interrupts
    asm volatile("li    %0, 0x202    \n"
                 "csrs  sie, %0      \n" // Enable supervisor external and software interrupts
                 "csrsi sstatus, 0x2 \n" // Enable interrupts
                 : "=&r"(temp));

    // Bring up Compute Minions
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, functional_shires, 0x1u, 0);

    log_write(LOG_LEVEL_INFO, "All Compute Minions configured!\n");

    // Wait until all Shires have booted before starting the main loop that handles PCIe messages
    wait_all_shires_booted(functional_shires);
    log_write(LOG_LEVEL_CRITICAL, "Shires (0x%" PRIx64 ") ready!\n", functional_shires);

    // Initialize VQs
    VQUEUE_init();
    // Update Status to indicate MM VQ is ready to use
    g_mm_dev_intf_reg->status = MM_DEV_INTF_MM_BOOT_STATUS_VQ_READY;
    log_write(LOG_LEVEL_CRITICAL, "MM VQs Ready!\r\n");

    // Set MM (Slave) Ready, and wait for SP (Master) Ready
    MBOX_set_status(MBOX_SP, MBOX_SLAVE, MBOX_STATUS_READY);
    if (wait_sp_ready() != SP_MM_HANDSHAKE_POLL_SUCCESS) {
        // Set Device Interface Register to communicate error to Host
        g_mm_dev_intf_reg->status = MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_MB_TIMEOUT;
    }

    // Indicate to Host MM is ready to accept new commands
    MBOX_init();
    log_write(LOG_LEVEL_CRITICAL, "MB initialized\r\n");

    // Wait for a message from the host, worker minion, PCI-E, etc.
    while (1) {
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

static int8_t handle_message_from_host_sq(uint32_t sq_idx)
{
    int64_t length;
    int8_t status = VQ_ERROR_NOT_READY;

    // Check and update the virtual queue status
    VQUEUE_update_status(sq_idx);

    // Check if VQ is ready to use and SQ has some command available
    if (VQUEUE_ready(sq_idx) && !VQUEUE_empty(SQ, sq_idx)) {
        // Only pop from SQ if CQ has space for new respnonse handling
        if (!VQUEUE_full(CQ, sq_idx)) {
            length = handle_device_ops_cmd(sq_idx);
            if (length > 0) {
                // success
                status = 0;
            }
        } else {
            status = VQ_ERROR_CQ_FULL;
        }
    }

    return status;
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
    (void)buffer;
    // TODO: Handle messages
}

static void dispatcher_handle_messages_on_unicast(void)
{
    /* Process as many requests as available */
    while (1) {
        cm_iface_message_t message;
        int8_t status;

        // Unicast to dispatcher is slot 0 of unicast circular-buffers
        status = CM_To_MM_Iface_Unicast_Receive(0, &message);
        if (status != STATUS_SUCCESS)
            break;

        switch (message.header.id) {
        case CM_TO_MM_MESSAGE_ID_NONE:
            log_write(LOG_LEVEL_DEBUG, "Invalid MESSAGE_ID_NONE received\r\n");
            break;

        case CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY: {
            const mm_to_cm_message_shire_ready_t *shire_ready =
                (const mm_to_cm_message_shire_ready_t *)&message;
            log_write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_SHIRE_READY received from shire %" PRId64 "\r\n",
                      shire_ready->shire_id);
            update_shire_state(shire_ready->shire_id, SHIRE_STATE_READY);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ACK: {
            const cm_to_mm_message_kernel_launch_ack_t *msg =
                (const cm_to_mm_message_kernel_launch_ack_t *)&message;
            log_write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_LAUNCH_ACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_kernel_state(msg->slot_index, KERNEL_STATE_RUNNING);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_NACK: {
            const cm_to_mm_message_kernel_launch_nack_t *msg =
                (const cm_to_mm_message_kernel_launch_nack_t *)&message;
            log_write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_shire_state(msg->shire_id, SHIRE_STATE_ERROR);
            update_kernel_state(msg->slot_index, KERNEL_STATE_ERROR);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_ABORT_NACK: {
            const cm_to_mm_message_kernel_launch_nack_t *msg =
                (const cm_to_mm_message_kernel_launch_nack_t *)&message;
            log_write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_shire_state(msg->shire_id, SHIRE_STATE_ERROR);
            update_kernel_state(msg->slot_index, KERNEL_STATE_ERROR);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
            log_write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_COMPLETE received\r\n");
            update_kernel_state(((cm_to_mm_message_kernel_launch_completed_t *)&message)->slot_index,
                                KERNEL_STATE_COMPLETE);
            break;

        case CM_TO_MM_MESSAGE_ID_FW_EXCEPTION: {
            cm_to_mm_message_exception_t *exception = (cm_to_mm_message_exception_t *)&message;
            print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
                            exception->hart_id);
            // non-kernel exceptions are unrecoverable. Put the shire in error state
            update_shire_state((exception->hart_id) / 64u, SHIRE_STATE_ERROR);
            const int kernel = 0; // TODO: Properly get kernel_id....
            update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION: {
            //cm_to_mm_message_exception_t *exception = (cm_to_mm_message_exception_t *)&message;
            //print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
            //                exception->hart_id);
            const int kernel = 0; // TODO: Properly get kernel_id....
            update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
            break;
        }

        default:
            log_write(LOG_LEVEL_CRITICAL,
                      "Unknown message id = 0x%016" PRIx64 " received (unicast dispatcher)\r\n",
                      message.header.id);
            break;
        }
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
