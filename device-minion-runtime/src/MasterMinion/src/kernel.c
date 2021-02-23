#include "kernel.h"
#include "broadcast.h"
#include "cacheops.h"
#include "circbuff.h"
#include "device_api_non_privileged.h"
#include "esr_defines.h"
#include "fcc.h"
#include "hart.h"
#include "layout.h"
#include "log.h"
#include "mailbox.h"
#include "mailbox_id.h"
#include "mm_to_cm_iface.h"
#include "cm_to_mm_iface.h"
#include "shire.h"
#include "syscall_internal.h"

#include <esperanto/device-api/device_api.h>

//#define DEBUG_PRINT_KERNEL_INSTRUCTIONS

// Base Hart ID for Kernel Sync threads relative to Master Shire
#define KERNEL_SYNC_MS_HART_BASE    2U

typedef struct {
    struct kernel_launch_cmd_t launch_cmd;
    kernel_state_t kernel_state;
    uint64_t shire_mask;
    uint64_t start_time;
    uint64_t end_time;
} kernel_status_t;

typedef struct {
    // Kernel ID
    uint64_t kernel_id;
    // Coming from Kernel Launch command
    uint64_t code_start_address;
    uint64_t pointer_to_args;
    uint64_t shire_mask;
    // Flags
    uint64_t kernel_launch_flags;
} __attribute__((aligned(64))) kernel_config_t;

// Local state, only used by Master Thread
static kernel_status_t kernel_status[MAX_SIMULTANEOUS_KERNELS] = { 0 };

// Shared state between Master Thread and Kernel Workers
static kernel_config_t kernel_config[MAX_SIMULTANEOUS_KERNELS] = { 0 };

/// \brief preparate a response to the kernel respose
static void send_kernel_launch_response(const struct kernel_launch_cmd_t *const launch_cmd,
                                        const dev_api_kernel_launch_error_e error);

static void clear_kernel_config(kernel_id_t kernel_id);

void kernel_init(void)
{
    for (uint64_t kernel = 0; kernel < MAX_SIMULTANEOUS_KERNELS; kernel++) {
        clear_kernel_config(kernel);
    }
}

static void send_kernel_launch_response(const struct kernel_launch_cmd_t *const cmd,
                                        const dev_api_kernel_launch_error_e error)
{
    log_write(LOG_LEVEL_CRITICAL, "Kernel Launch Response %" PRIi64 "\r\n", error);
    struct kernel_launch_rsp_t rsp;
    rsp.response_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_LAUNCH_RSP;
    prepare_device_api_reply(&cmd->command_info, &rsp.response_info);
    rsp.error = error;

    int64_t result = MBOX_send(MBOX_PCIE, &rsp, sizeof(rsp));
    if (result != 0) {
        log_write(LOG_LEVEL_ERROR,
                  "DeviceAPI Kernel Launch Response MBOX_send error %" PRIi64 "\r\n", result);
    }
}

// Waits for all the shires associated with a kernel to report ready via a FCC,
// then synchronizes their release to run the kernel by sending a FCC
void __attribute__((noreturn)) kernel_sync_thread(uint64_t kernel_id)
{
    volatile const kernel_config_t *const kernel_config_ptr = &kernel_config[kernel_id];

    init_fcc(FCC_0);
    init_fcc(FCC_1);

    // Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    // But enable Supervisor Software Interrupts so that IPIs trap when in U-mode
    // RISC-V spec:
    //   "An interrupt i will be taken if bit i is set in both mip and mie,
    //    and if interrupts are globally enabled."
    asm volatile("csrci sstatus, 0x2\n");
    asm volatile("csrsi sie, 0x2\n");

    while (1) {
        // wait for a kernel launch sync request from master_thread
        WAIT_FCC(0);

        // read fresh kernel_config
        evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
        WAIT_CACHEOPS

        const uint64_t launch_shire_mask = kernel_config_ptr->shire_mask;
        const uint64_t launch_num_shires = (uint64_t)__builtin_popcountll(launch_shire_mask);

        if (launch_shire_mask != 0) {
            mm_to_cm_message_kernel_launch_t launch;
            launch.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
            launch.header.number = 0; // Filled by MM_To_CM_Iface_Multicast_Send
            launch.kw_base_id = KERNEL_SYNC_MS_HART_BASE;
            launch.slot_index = (uint8_t)kernel_id;
            launch.flags = (uint8_t)kernel_config_ptr->kernel_launch_flags;
            launch.code_start_address = kernel_config_ptr->code_start_address;
            launch.pointer_to_args = kernel_config_ptr->pointer_to_args;
            launch.shire_mask = launch_shire_mask;

            if (0 != MM_To_CM_Iface_Multicast_Send(launch_shire_mask, (cm_iface_message_t *)&launch)) {
                // Problem sending broadcast message, send error message to the master minion
                cm_to_mm_message_kernel_launch_nack_t nack_message;
                nack_message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_NACK;
                nack_message.shire_id = get_shire_id();
                nack_message.slot_index = (uint8_t)kernel_id;
                // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
                CM_To_MM_Iface_Unicast_Send(0, 0, (const cm_iface_message_t *)&nack_message);
            }

            // Send message to master minion indicating the kernel is starting
            cm_to_mm_message_kernel_launch_ack_t ack_message;
            ack_message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ACK;
            ack_message.slot_index = (uint8_t)kernel_id;
            // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
            CM_To_MM_Iface_Unicast_Send(0, 0, (const cm_iface_message_t *)&ack_message);

            /* Wait for a KERNEL_DONE message from each Shire involved in the launch.
             * Even if there was an exception, that Shire will still send a KERNEL_DONE */
            uint32_t done_cnt = 0;
            bool had_exception = false;
            while (done_cnt < launch_num_shires) {
                /* Wait for an IPI */
                asm volatile("wfi\n");
                asm volatile("csrci sip, 0x2");

                /* Process as many requests as available */
                while (1) {
                    cm_iface_message_t message;
                    int8_t status;

                    status = CM_To_MM_Iface_Unicast_Receive(1 + kernel_id, &message);
                    if (status != STATUS_SUCCESS)
                        break;

                    /* Handle message from Compute Threads */
                    switch (message.header.id) {
                    case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
                        log_write(LOG_LEVEL_DEBUG, "CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE\n");
                        /* Increase count of completed Shires */
                        done_cnt++;
                        break;
                    case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION:
                        log_write(LOG_LEVEL_DEBUG, "CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION\n");
                        had_exception = true;
                        break;
                    default:
                        break;
                    }
                }
            }

            // Send message to master minion indicating the kernel is complete (maybe with excp.)
            cm_to_mm_message_kernel_launch_completed_t completed_message;
            if (had_exception) {
                completed_message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION;
            } else {
                completed_message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE;
            }
            completed_message.shire_id = get_shire_id();
            completed_message.slot_index = (uint8_t)kernel_id;
            // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
            CM_To_MM_Iface_Unicast_Send(0, 0, (const cm_iface_message_t *)&completed_message);
        } else {
            // Invalid config, send error message to the master minion
            cm_to_mm_message_kernel_launch_nack_t nack_message;
            nack_message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_NACK;
            nack_message.shire_id = get_shire_id();
            nack_message.slot_index = (uint8_t)kernel_id;
            // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
            CM_To_MM_Iface_Unicast_Send(0, 0, (const cm_iface_message_t *)&nack_message);
        }
    }
}

void update_kernel_state(kernel_id_t kernel_id, kernel_state_t kernel_state)
{
    if (kernel_id >= KERNEL_ID_NONE) {
        return; // error
    }

    // Update kernel state per shire state change
    switch (kernel_state) {
    case KERNEL_STATE_UNUSED:
        kernel_status[kernel_id].kernel_state = KERNEL_STATE_UNUSED;
        break;

    case KERNEL_STATE_LAUNCHED:
        kernel_status[kernel_id].kernel_state = KERNEL_STATE_LAUNCHED;
        break;

    case KERNEL_STATE_RUNNING: {
        kernel_status[kernel_id].start_time = (uint64_t)syscall(SYSCALL_GET_MTIME_INT, 0, 0, 0);

        kernel_status[kernel_id].kernel_state = KERNEL_STATE_RUNNING;

        // Mark all shires associated with this kernel as running
        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
            if (kernel_status[kernel_id].shire_mask & (1ULL << shire)) {
                update_shire_state(shire, SHIRE_STATE_RUNNING);
            }
        }
        break;
    }

    case KERNEL_STATE_ABORTED:
        kernel_status[kernel_id].kernel_state = KERNEL_STATE_ABORTED;
        break;

    case KERNEL_STATE_ERROR: {
        // We might receive this many times within a single launch if there are many exceptions...
        if (kernel_status[kernel_id].kernel_state != KERNEL_STATE_ERROR) {
            send_kernel_launch_response(&kernel_status[kernel_id].launch_cmd,
                                        DEV_API_KERNEL_LAUNCH_ERROR_RESULT_ERROR);

            clear_kernel_config(kernel_id);
            kernel_status[kernel_id].kernel_state = KERNEL_STATE_ERROR;
        }
        break;
    }

    case KERNEL_STATE_COMPLETE: {
        kernel_status[kernel_id].end_time = (uint64_t)syscall(SYSCALL_GET_MTIME_INT, 0, 0, 0);
        // TODO FIXME mtime is currently 40MHz and not the specified 10MHz for two reasons:
        // 1. RTLMIN-5392: PU RV Timer is dividing clk_100Mhz /25 instead of /10
        // 2. In ZeBu, clk_100 is forced to 1GHz for now - it will reduce to 100MHz eventually.
        // 1GHz / 25 = 40Mhz
        uint64_t elapsed_time_us =
            (kernel_status[kernel_id].end_time - kernel_status[kernel_id].start_time) / 40;

        log_write(LOG_LEVEL_INFO, "kernel %d complete, %" PRId64 "us\r\n", kernel_id,
                  elapsed_time_us);

        send_kernel_launch_response(&kernel_status[kernel_id].launch_cmd,
                                    DEV_API_KERNEL_LAUNCH_ERROR_RESULT_OK);

        // Mark all shires associated with this kernel as complete
        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
            if (kernel_status[kernel_id].shire_mask & (1ULL << shire)) {
                update_shire_state(shire, SHIRE_STATE_COMPLETE);
            }
        }

        clear_kernel_config(kernel_id);
        kernel_status[kernel_id].kernel_state = KERNEL_STATE_UNUSED;
        break;
    }

    case KERNEL_STATE_UNKNOWN:
    default:
        break;
    }
}

void launch_kernel(const struct kernel_launch_cmd_t *const launch_cmd)
{
    const kernel_id_t kernel_id = 0; // TODO: Find available slot
    const uint64_t shire_mask = launch_cmd->shire_mask & 0x1FFFFFFFF;
    kernel_status_t *const kernel_status_ptr = &kernel_status[kernel_id];
    uint64_t num_shires = 0;
    bool allShiresReady = true;
    bool kernelReady = true;

    if (!all_shires_ready(shire_mask)) {
        log_write(LOG_LEVEL_ERROR,
                  "aborting kernel %d launch, not all shires ready, needed shires: 0x%" PRIx64
                  "\r\n",
                  kernel_id, shire_mask);
        allShiresReady = false;
    }

#ifdef DEBUG_PRINT_KERNEL_INSTRUCTIONS
    const uint32_t *pc = (uint32_t *)kernel_info_ptr->compute_pc;

    for (int i = 0; i < 10; i++) {
        log_write(LOG_LEVEL_INFO, "PC: 0x%010" PRIxPTR " data: 0x%08" PRIx32 "\r\n", pc, *pc);
        pc += 1;
    }
#endif

    // Confirm this kernel is not active
    if (kernel_status_ptr->kernel_state != KERNEL_STATE_UNUSED) {
        log_write(LOG_LEVEL_ERROR, "aborting kernel %d launch, state not unused\r\n", kernel_id);
        kernelReady = false;
    }

    if (allShiresReady && kernelReady) {
        // Update the kernel status cmd with the kernel-launch command we are going to use
        kernel_status_ptr->launch_cmd = *launch_cmd;
        kernel_status_ptr->launch_cmd.shire_mask = shire_mask;

        volatile kernel_config_t *const kernel_config_ptr = &kernel_config[kernel_id];

        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
            if (shire_mask & (1ULL << shire)) {
                num_shires++;
            }
        }

        uint64_t kernel_launch_flags = 0;

        // Additional kernel-launch flags
        // [SW-3260] Evict L3 to make sure no DMA-ed L3 lines are dirty at kernel launch
        kernel_launch_flags |= KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;

        // Copy kernel_id, params, info and flags into kernel config buffer
        kernel_config_ptr->kernel_id = kernel_id;
        kernel_config_ptr->code_start_address = launch_cmd->code_start_address;
        kernel_config_ptr->pointer_to_args = launch_cmd->pointer_to_args;
        kernel_config_ptr->shire_mask = shire_mask;
        kernel_config_ptr->kernel_launch_flags = kernel_launch_flags;

        // Evict kernel config to point of coherency - sync threads and worker minion will read it
        FENCE
        evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
        WAIT_CACHEOPS

        // notify the appropriate sync thread to manage kernel launch
        notify_kernel_sync_thread(kernel_id, FCC_0);

        update_kernel_state(kernel_id, KERNEL_STATE_LAUNCHED);
        // FIXME SW-1471 generate an event message back to the host that we launched
        kernel_status_ptr->shire_mask = shire_mask;

        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
            if (shire_mask & (1ULL << shire)) {
                set_shire_kernel_id(shire, kernel_id);
            }
        }

        log_write(LOG_LEVEL_CRITICAL, "Launching kernel %d (0x%" PRIx64 ")\r\n", kernel_id, shire_mask);
    } else {
        send_kernel_launch_response(&kernel_status[kernel_id].launch_cmd,
                                    DEV_API_KERNEL_LAUNCH_ERROR_SHIRES_NOT_READY);
    }
}

// Sends an abort message to all HARTs in all shires associated with the kernel_id if
// the kernel is running
dev_api_kernel_abort_response_result_e abort_kernel(kernel_id_t kernel_id)
{
    const kernel_state_t kernel_state = kernel_status[kernel_id].kernel_state;

    if ((kernel_state == KERNEL_STATE_LAUNCHED) || (kernel_state == KERNEL_STATE_RUNNING) ||
        (kernel_state == KERNEL_STATE_ERROR)) {
        cm_iface_message_t message = {
            .header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT,
            .data = { 0 },
        };

        if (0 == MM_To_CM_Iface_Multicast_Send(kernel_status[kernel_id].shire_mask, &message)) {
            log_write(LOG_LEVEL_CRITICAL, "abort_kernel: aborted kernel %d\r\n", kernel_id);
            update_kernel_state(kernel_id, KERNEL_STATE_ABORTED);

            return DEV_API_KERNEL_ABORT_RESPONSE_RESULT_OK;
        }
    }

    return DEV_API_KERNEL_ABORT_RESPONSE_RESULT_ERROR;
}

kernel_state_t get_kernel_state(kernel_id_t kernel_id)
{
    if (kernel_id >= KERNEL_ID_NONE) {
        return KERNEL_STATE_UNKNOWN;
    } else {
        return kernel_status[kernel_id].kernel_state;
    }
}

// Clear fields of kernel config so worker minion recognize it's inactive
static void clear_kernel_config(kernel_id_t kernel_id)
{
    volatile kernel_config_t *const kernel_config_ptr = &kernel_config[kernel_id];
    kernel_config_ptr->shire_mask = 0;

    // Evict kernel config to point of coherency - sync threads and worker minion will read it
    FENCE
    evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
    WAIT_CACHEOPS
}
