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

/* Internal data structures */
typedef struct {
    cm_iface_message_number_t number;
} __attribute__((aligned(64))) cm_iface_message_number_internal_t;

/* Helper macros */
#define CURRENT_THREAD_MASK      ((0x1UL << (get_hart_id() % 64)))
#define GET_SHIRE_MASK(shire_id) (1ULL << shire_id)
#define GET_CM_INDEX(hart_id)    ((hart_id < 2048U) ? hart_id : (hart_id - 32U))
#define MM_NOTIFY_ASYNC_MSG(shire_id, msg_header)            \
    {                                                        \
        if (!(msg_header.flags & CM_IFACE_FLAG_SYNC_CMD))    \
        {                                                    \
            /* Ack back to MM upon receiving the message. */ \
            notify_mm(shire_id);                             \
        }                                                    \
    }
#define MM_NOTIFY_SYNC_MSG(shire_id, msg_header)             \
    {                                                        \
        if (msg_header.flags & CM_IFACE_FLAG_SYNC_CMD)       \
        {                                                    \
            /* Ack back to MM upon completion of command. */ \
            notify_mm(shire_id);                             \
        }                                                    \
    }

/* MM -> CM global variables */
#define mm_cm_msg_number ((cm_iface_message_number_internal_t *)CM_MM_HART_MESSAGE_COUNTER)
static spinlock_t mm_cm_msg_read[NUM_SHIRES] = { 0 };

/* MM -> CM message buffers */
#define master_to_worker_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define master_to_worker_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

/* Local function prototypes */
static void mm_to_cm_iface_handle_message(
    cm_iface_message_t *const message_ptr, void *const optional_arg);

/* Finds the last shire involved in MM->CM message and notifies the MM */
static inline void notify_mm(uint64_t shire_id)
{
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    /* Last thread per shire clears the global shire bitmask */
    if (atomic_add_local_32(&mm_cm_msg_read[shire_id].flag, 1U) == (thread_count - 1))
    {
        /* Reset the MM-CM msg read counter */
        init_local_spinlock(&mm_cm_msg_read[shire_id], 0);

        /* Clear bit for current shire to send msg acknowledgment to MM */
        atomic_and_global_64(
            &master_to_worker_broadcast_message_ctrl_ptr->shire_mask, ~GET_SHIRE_MASK(shire_id));
    }
}

void MM_To_CM_Iface_Init(void)
{
    const uint32_t thread_idx = GET_CM_INDEX(get_hart_id());

    /* Initialize the globals to zero */
    mm_cm_msg_number[thread_idx].number = 0U;

    /* Reset the MM-CM msg read counter */
    init_local_spinlock(&mm_cm_msg_read[get_shire_id()], 0);
}

void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void)
{
    uint64_t sip;

    Log_Write(LOG_LEVEL_DEBUG, "CM:Ready to process interrupts.\r\n");

    for (;;)
    {
        /* Wait for an interrupt */
        WFI

            /* Read pending interrupts */
            SUPERVISOR_PENDING_INTERRUPTS(sip);

        Log_Write(LOG_LEVEL_DEBUG, "Exiting WFI! SIP: 0x%" PRIx64 "\r\n", sip);

        if (sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* We got a software interrupt (IPI) handed down from M-mode.
            M-mode already cleared the MSIP (Machine Software Interrupt Pending)
            Clear Supervisor Software Interrupt Pending (SSIP) */
            SUPERVISOR_INTERRUPT_PENDING_CLEAR(SUPERVISOR_SOFTWARE_INTERRUPT)

            /* Receive and process message buffer */
            MM_To_CM_Iface_Multicast_Receive(0);
        }

        if (sip & (1 << BUS_ERROR_INTERRUPT))
        {
            /* Clear Bus Error Interrupt Pending */
            asm volatile("csrc sip, %0" : : "r"(1 << BUS_ERROR_INTERRUPT));

            Log_Write(
                LOG_LEVEL_ERROR, "CM:Bus error interrupt received:SIP:0x%" PRIx64 "\r\n", sip);
        }
    }
}

void MM_To_CM_Iface_Multicast_Receive(void *const optional_arg)
{
    const uint32_t thread_idx = GET_CM_INDEX(get_hart_id());
    cm_iface_message_t *message = master_to_worker_broadcast_message_buffer_ptr;

    /* Evict stale line from L1D */
    ETSOC_MEM_EVICT(message, sizeof(cm_iface_message_t), to_L2)

    /* Check for pending MM->CM message */
    if (message->header.number != mm_cm_msg_number[thread_idx].number)
    {
        /* Update the global copy of read messages */
        mm_cm_msg_number[thread_idx].number = message->header.number;

        Log_Write(LOG_LEVEL_DEBUG, "MM->CM:Msg received:msg_id:%d:msg_num:%d\r\n",
            message->header.id, message->header.number);

        /* Handle the message */
        mm_to_cm_iface_handle_message(message, optional_arg);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "MM->CM: Tried to read a non-pending message:Msg Number:%d:\r\n",
            message->header.number);
    }
}

static void mm_to_cm_iface_handle_message(
    cm_iface_message_t *const message_ptr, void *const optional_arg)
{
    cm_iface_message_header_t msg_header = message_ptr->header;
    const uint32_t shire = get_shire_id();
    const uint32_t hart = get_hart_id();

    switch (msg_header.id)
    {
        case MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH:
        {
            int64_t rv = 0;
            const mm_to_cm_message_kernel_launch_t *launch =
                (mm_to_cm_message_kernel_launch_t *)message_ptr;

            /* Check if this Shire is involved in the kernel launch */
            if (launch->kernel.shire_mask & (1ULL << shire))
            {
                Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM: Launching Kernel on Shire 0x%llx\r\n",
                    msg_header.tag_id, 1ULL << shire);

                mm_to_cm_message_kernel_params_t kernel;
                kernel.kw_base_id = launch->kernel.kw_base_id;
                kernel.slot_index = launch->kernel.slot_index;
                kernel.flags = launch->kernel.flags;
                kernel.code_start_address = launch->kernel.code_start_address;
                kernel.pointer_to_args = launch->kernel.pointer_to_args;
                kernel.shire_mask = launch->kernel.shire_mask;
                kernel.exception_buffer = launch->kernel.exception_buffer;

                /* Notify MM after copying the msg locally */
                MM_NOTIFY_ASYNC_MSG(shire, msg_header)

                /* Launch the kernel in U-mode */
                rv = launch_kernel(kernel);
            }
            else
            {
                /* Notify MM after parsing the msg */
                MM_NOTIFY_ASYNC_MSG(shire, msg_header)

                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:MM->CM:Kernel launch msg received on shire not involved in kernel launch\r\n",
                    msg_header.tag_id);
            }

            if (rv != 0)
            {
                /* Something went wrong launching the kernel. */
                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:MM->CM: Kernel completed with error code:%ld\r\n", msg_header.tag_id,
                    rv);
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_KERNEL_ABORT:
        {
            /* Notify MM after parsing the msg */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            Log_Write(
                LOG_LEVEL_DEBUG, "TID[%u]:MM->CM:Kernel abort msg received\r\n", msg_header.tag_id);

            /* Should only abort if the kernel was launched on this hart */
            if (kernel_info_has_thread_launched(shire, hart & (HARTS_PER_SHIRE - 1)))
            {
                uint64_t exception_buffer = 0;

                Log_Write(
                    LOG_LEVEL_WARNING, "TID[%u]:MM->CM:Aborting kernel...\r\n", msg_header.tag_id);

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

                    Log_Write(LOG_LEVEL_INFO, "TID[%u]:MM->CM:Saving context on kernel abort\r\n",
                        msg_header.tag_id);

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
            Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM:Trace update control msg received\r\n",
                msg_header.tag_id);

            mm_to_cm_message_trace_rt_control_t *cmd =
                (mm_to_cm_message_trace_rt_control_t *)message_ptr;
            uint64_t thread_mask = cmd->thread_mask;
            uint32_t cm_control = cmd->cm_control;

            /* Notify MM after copying the msg locally */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            if (thread_mask & CURRENT_THREAD_MASK)
            {
                Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM: Trace RT Control for current hart.\r\n",
                    msg_header.tag_id);
                Trace_RT_Control_CM(cm_control);
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_TRACE_CONFIGURE:
        {
            const mm_to_cm_message_trace_rt_config_t *cmd =
                (mm_to_cm_message_trace_rt_config_t *)message_ptr;
            struct trace_config_info_t cm_trace_config = { .filter_mask = cmd->filter_mask,
                .event_mask = cmd->event_mask,
                .threshold = cmd->threshold };
            uint64_t thread_mask = cmd->thread_mask;
            uint64_t shire_mask = cmd->shire_mask;

            /* Notify MM after copying the msg locally */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            /* Disable Trace for Harts not specified in given Shire and Thread mask. */
            if ((thread_mask & CURRENT_THREAD_MASK) && (shire_mask & GET_SHIRE_MASK(shire)))
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "TID[%u]:MM->CM: Trace Config. Event:0x%x:Filter:0x%x:Threshold:%d\r\n",
                    msg_header.tag_id, cm_trace_config.event_mask, cm_trace_config.filter_mask,
                    cm_trace_config.threshold);

                /* Update Trace configuration for current hart */
                int32_t status = Trace_Configure_CM(&cm_trace_config);

                /* Enable the Trace. Even Trace config failed it will enable Trace on existing configurations. */
                Trace_Set_Enable_CM(TRACE_ENABLE);

                if (status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_CRITICAL, "CM:TRACE_RT_CONFIG:Done!!\n");
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR, "CM:TRACE_RT_CONFIG:Failed:Status:%d!\r\n", status);
                }
            }
            else
            {
                Trace_Set_Enable_CM(TRACE_DISABLE);
            }

            break;
        }
        case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT:
        {
            const mm_to_cm_message_trace_buffer_evict_t *cmd =
                (mm_to_cm_message_trace_buffer_evict_t *)message_ptr;
            uint64_t thread_mask = cmd->thread_mask;

            /* Notify MM after copying the msg locally */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM:Trace buffer evict msg received\r\n",
                msg_header.tag_id);

            if (thread_mask & CURRENT_THREAD_MASK)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "TID[%u]:MM->CM: Evict Trace Buffer for current hart\r\n", msg_header.tag_id);
                /* Evict Trace buffer. */
                Trace_Evict_CM_Buffer();
            }
            break;
        }
        case MM_TO_CM_MESSAGE_ID_DUMP_THREAD_CONTEXT:
        {
            /* Notify MM after parsing the msg */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:MM->CM:Dump thread context msg received\r\n",
                msg_header.tag_id);

            /* Check if pointer to execution context was set */
            if (optional_arg != 0)
            {
                const internal_execution_context_t *context =
                    (internal_execution_context_t *)optional_arg;
                struct dev_context_registers_t trace_context = { .epc = context->sepc,
                    .tval = context->stval,
                    .status = context->sstatus,
                    .cause = context->scause };

                Log_Write(LOG_LEVEL_INFO, "TID[%u]:MM->CM:Dumping the context in trace buffer\r\n",
                    msg_header.tag_id);

                /* Copy all the GPRs except x0 (hardwired to zero) */
                memcpy(trace_context.gpr, &context->regs[1],
                    sizeof(uint64_t) * TRACE_DEV_CONTEXT_GPRS);

                /* Log the execution stack event to trace */
                Trace_Execution_Stack(Trace_Get_CM_CB(), &trace_context);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "TID[%u]:MM->CM:Context not available in WFI loop\r\n",
                    msg_header.tag_id);
            }
            break;
        }
        default:
            /* Notify MM after parsing the msg */
            MM_NOTIFY_ASYNC_MSG(shire, msg_header)

            /* Unknown message received */
            Log_Write(LOG_LEVEL_ERROR, "TID[%u]:MM->CM:Unknown msg received:ID:%d\r\n",
                msg_header.tag_id, msg_header.id);

            const cm_to_mm_message_fw_error_t message = { .header.id = CM_TO_MM_MESSAGE_ID_FW_ERROR,
                .hart_id = hart,
                .error_code = CM_INVALID_MM_TO_CM_MESSAGE_ID };

            /* To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0) */
            int32_t status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
                CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (const cm_iface_message_t *)&message);

            if (status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR, "CM->MM:Unicast send failed! Error code: %d\n", status);
            }
            break;
    }
    /* Check and notify MM for synchronous msg */
    MM_NOTIFY_SYNC_MSG(shire, msg_header)
}
