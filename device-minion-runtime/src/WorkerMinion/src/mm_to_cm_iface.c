#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <system/layout.h>
#include <transports/mm_cm_iface/message_types.h>

#include "cm_to_mm_iface.h"
#include "error_codes.h"
#include "kernel.h"
#include "log.h"
#include "mm_to_cm_iface.h"
#include "syscall_internal.h"
#include "trace.h"

typedef struct {
    cm_iface_message_number_t number;
} __attribute__((aligned(64))) cm_iface_message_number_internal_t;

#define CURRENT_THREAD_MASK ((0x1UL << (get_hart_id() % 64)))
#define CURRENT_SHIRE_MASK  (1ULL << get_shire_id())
/* MM -> CM message counters */
#define mm_cm_msg_number ((cm_iface_message_number_internal_t *)CM_MM_HART_MESSAGE_COUNTER)
static spinlock_t pre_msg_local_barrier[NUM_SHIRES] = { 0 };
static spinlock_t msg_sync_local_barrier[NUM_SHIRES] = { 0 };

/* MM -> CM message buffers */
#define master_to_worker_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define master_to_worker_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

/* Local function prototypes */
static void mm_to_cm_iface_handle_message(
    uint32_t shire, uint64_t hart, cm_iface_message_t *const message_ptr, void *const optional_arg);

/* Finds the last shire involved in MM->CM message and notifies the MM */
static inline void read_msg_and_notify_mm(uint64_t shire_id, cm_iface_message_t *const message)
{
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;
    uint32_t thread_num = atomic_add_local_32(&pre_msg_local_barrier[shire_id].flag, 1U);

    /* First thread brings message from L3 */
    if (thread_num == 0)
    {
        /* Bring data from L3 and copy message from shared global memory to local buffer */
        ETSOC_MEM_EVICT_AND_COPY(
            message, master_to_worker_broadcast_message_buffer_ptr, sizeof(*message), to_L3)

        /* Set the local barrier flag */
        init_local_spinlock(&msg_sync_local_barrier[shire_id], 1);
    }
    else
    {
        /* All threads in Shire wait for first Thread to set flag */
        local_spinwait_wait(&msg_sync_local_barrier[shire_id], 1, 0);

        /* Bring data from L2 and copy message from shared global memory to local buffer */
        ETSOC_MEM_EVICT_AND_COPY(
            message, master_to_worker_broadcast_message_buffer_ptr, sizeof(*message), to_L2)
    }

    /* Last thread per shire decrements global counter */
    if (thread_num == (thread_count - 1))
    {
        /* Reset the pre msg local barrier flag */
        init_local_spinlock(&pre_msg_local_barrier[shire_id], 0);

        /* Reset the msg sync local barrier flag */
        init_local_spinlock(&msg_sync_local_barrier[shire_id], 0);

        /* Clear bit for current shire to send msg acknowledgment to MM */
        atomic_and_global_64(
            &master_to_worker_broadcast_message_ctrl_ptr->shire_mask, ~CURRENT_SHIRE_MASK);
    }
}

void MM_To_CM_Iface_Init(void)
{
    const uint32_t hart_id = get_hart_id();

    /* Initialize the MM-CM message counter to zero */
    mm_cm_msg_number[hart_id].number = 0U;
    ETSOC_MEM_EVICT(
        (void *)&mm_cm_msg_number[hart_id].number, sizeof(cm_iface_message_number_t), to_L3)
}

void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void)
{
    log_write(LOG_LEVEL_DEBUG, "MM->CM:Ready to process msgs from MM.\r\n");

    for (;;)
    {
        /* Wait for an IPI (Software Interrupt) */
        asm volatile("wfi\n");

        /* We got a software interrupt (IPI) handed down from M-mode.
        M-mode already cleared the MSIP (Machine Software Interrupt Pending)
        Clear Supervisor Software Interrupt Pending (SSIP) */
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        /* Receive and process message buffer */
        MM_To_CM_Iface_Multicast_Receive(0);
    }
}

void MM_To_CM_Iface_Multicast_Receive(void *const optional_arg)
{
    const uint32_t shire_id = get_shire_id();
    const uint32_t hart_id = get_hart_id();
    cm_iface_message_t message;

    read_msg_and_notify_mm(shire_id, &message);

    /* Check for pending MM->CM message */
    if (message.header.number != mm_cm_msg_number[hart_id].number)
    {
        /* Update the global copy of read messages */
        mm_cm_msg_number[hart_id].number = message.header.number;

        /* Handle the message */
        mm_to_cm_iface_handle_message(shire_id, hart_id, &message, optional_arg);
    }
    else
    {
        log_write(LOG_LEVEL_WARNING, ":MM->CM: Tried to read a non-pending message!\n");
    }
}

static void mm_to_cm_iface_handle_message(
    uint32_t shire, uint64_t hart, cm_iface_message_t *const message_ptr, void *const optional_arg)
{
    switch (message_ptr->header.id)
    {
        case MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH:
        {
            int64_t rv = -1;
            const mm_to_cm_message_kernel_launch_t *launch =
                (mm_to_cm_message_kernel_launch_t *)message_ptr;
            /* Check if this Shire is involved in the kernel launch */
            if (launch->kernel.shire_mask & (1ULL << shire))
            {
                log_write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM: Launching Kernel on Shire 0x%lx\r\n",
                    message_ptr->header.tag_id, 1ULL << shire);

                mm_to_cm_message_kernel_params_t kernel;
                kernel.kw_base_id = launch->kernel.kw_base_id;
                kernel.slot_index = launch->kernel.slot_index;
                kernel.flags = launch->kernel.flags;
                kernel.code_start_address = launch->kernel.code_start_address;
                kernel.pointer_to_args = launch->kernel.pointer_to_args;
                kernel.shire_mask = launch->kernel.shire_mask;
                kernel.exception_buffer = launch->kernel.exception_buffer;

                uint64_t kernel_stack_addr =
                    KERNEL_UMODE_STACK_BASE - (hart * KERNEL_UMODE_STACK_SIZE);
                rv = launch_kernel(kernel, kernel_stack_addr);
            }

            if (rv != 0)
            {
                /* Something went wrong launching the kernel. */
                log_write(LOG_LEVEL_ERROR,
                    "TID[%u]:MM->CM: Kernel completed with error code:%d\r\n",
                    message_ptr->header.tag_id, rv);
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_KERNEL_ABORT:
        {
            /* Should only abort if the kernel was launched on this hart */
            if (kernel_info_has_thread_launched(shire, hart & (HARTS_PER_SHIRE - 1)))
            {
                uint64_t exception_buffer = 0;

                /* Check if pointer to execution context was set */
                if (optional_arg != 0)
                {
                    /* Get the kernel exception buffer */
                    exception_buffer = kernel_info_get_exception_buffer(shire);
                }

                /* If the kernel exception buffer is available */
                if (exception_buffer != 0)
                {
                    const internal_execution_context_t *context =
                        (internal_execution_context_t *)optional_arg;

                    /* Save the execution context in the buffer provided */
                    CM_To_MM_Save_Execution_Context((execution_context_t *)exception_buffer,
                        CM_CONTEXT_TYPE_SYSTEM_ABORT, hart, context);
                }

                return_from_kernel(0, KERNEL_RETURN_SYSTEM_ABORT);
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL:
        {
            mm_to_cm_message_trace_rt_control_t *cmd =
                (mm_to_cm_message_trace_rt_control_t *)message_ptr;
            if (cmd->thread_mask & CURRENT_THREAD_MASK)
            {
                log_write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM: Trace RT Control for current hart.\r\n",
                    message_ptr->header.tag_id);
                Trace_RT_Control_CM(cmd->cm_control);
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_TRACE_CONFIGURE:
        {
            const mm_to_cm_message_trace_rt_config_t *cmd =
                (mm_to_cm_message_trace_rt_config_t *)message_ptr;

            if (cmd->thread_mask & CURRENT_THREAD_MASK)
            {
                struct trace_config_info_t cm_trace_config = { .filter_mask = cmd->filter_mask,
                    .event_mask = cmd->event_mask,
                    .threshold = cmd->threshold };

                log_write(LOG_LEVEL_DEBUG,
                    "TID[%u]:MM->CM: Trace Config. Event:0x%lx:Filter:0x%lx:Threshold:%d\r\n",
                    message_ptr->header.tag_id, cmd->event_mask, cmd->filter_mask, cmd->threshold);
                Trace_Configure_CM(&cm_trace_config);
                Trace_String(
                    TRACE_EVENT_STRING_CRITICAL, Trace_Get_CM_CB(), "CM:TRACE_RT_CONFIG:Done!!\n");
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT:
        {
            const mm_to_cm_message_trace_buffer_evict_t *cmd =
                (mm_to_cm_message_trace_buffer_evict_t *)message_ptr;

            if (cmd->thread_mask & CURRENT_THREAD_MASK)
            {
                log_write(LOG_LEVEL_DEBUG,
                    "TID[%u]:MM->CM: Evict Trace Buffer for current hart\r\n",
                    message_ptr->header.tag_id);
                /* Evict Trace buffer. */
                Trace_Evict_CM_Buffer();
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_PMC_CONFIGURE:
            // Make a syscall to M-mode to configure PMCs
            syscall(SYSCALL_CONFIGURE_PMCS_INT, 0,
                ((mm_to_cm_message_pmc_configure_t *)message_ptr)->conf_buffer_addr, 0);
            break;
        default:
            /* Unknown message received */
            log_write(LOG_LEVEL_ERROR, "TID[%u]:MM->CM:Unknown msg received:ID:%d\r\n",
                message_ptr->header.tag_id, message_ptr->header.id);

            const cm_to_mm_message_fw_error_t message = { .header.id = CM_TO_MM_MESSAGE_ID_FW_ERROR,
                .hart_id = get_hart_id(),
                .error_code = CM_INVALID_MM_TO_CM_MESSAGE_ID };

            /* To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0) */
            int32_t status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
                CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (const cm_iface_message_t *)&message);

            if (status == STATUS_SUCCESS)
            {
                log_write(LOG_LEVEL_ERROR, "CM->MM:Unicast send failed! Error code: %d\n", status);
            }
            break;
    }
}
