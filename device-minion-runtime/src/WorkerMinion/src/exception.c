#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <etsoc/common/common_defs.h>
#include <etsoc/isa/macros.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>
#include <transports/mm_cm_iface/message_types.h>

#include "cm_to_mm_iface.h"
#include "cm_mm_defines.h"
#include "kernel.h"
#include "log.h"
#include "syscall_internal.h"
#include "trace.h"

void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg);
static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
    uint64_t hart_id, uint32_t shire_id, bool user_mode);

/* Note: ecalls are handled in syscall_handler, and some U-mode exceptions are delegated to S-mode from M-mode */
void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg)
{
    const uint64_t hart_id = get_hart_id();
    const uint32_t shire_id = get_shire_id();
    uint64_t sstatus;
    bool user_mode;

    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    user_mode = (sstatus & SSTATUS_SPP) == 0;

    /* S-mode exception */
    if (!user_mode)
    {
        struct dev_context_registers_t context = { .epc = sepc,
            .tval = stval,
            .status = sstatus,
            .cause = scause };

        Log_Write(LOG_LEVEL_ERROR,
            "S-mode XCPT scause=0x%" PRIx64 ", sepc=0x%" PRIx64 ", stval=0x%" PRIx64 "\n", scause,
            sepc, stval);

        Log_Write(LOG_LEVEL_INFO, "exception_handler:Saving context on S-mode exception\r\n");

        /* Copy all the GPRs except x0 (hardwired to zero) */
        memcpy(context.gpr, &reg[1], sizeof(uint64_t) * TRACE_DEV_CONTEXT_GPRS);

        /* Log the execution stack event to trace */
        Trace_Execution_Stack(Trace_Get_CM_CB(), &context);

        /* Evict S-mode Trace buffer to L3. */
        Trace_Evict_CM_Buffer();

        /* Sends exception message to MM */
        send_exception_message(scause, sepc, stval, sstatus, hart_id, shire_id, user_mode);

        /* Should only return from kernel if the kernel was launched on this hart.
           This is only required if S-Mode exception occurred while handling the U-Mode exception */
        if (kernel_info_has_thread_launched(shire_id, hart_id & (HARTS_PER_SHIRE - 1)))
        {
            return_from_kernel(0, KERNEL_RETURN_EXCEPTION);
        }
    }
    else /* U-mode exception */
    {
        /* Get the kernel exception buffer */
        uint64_t exception_buffer = kernel_info_get_exception_buffer(shire_id);

        Log_Write(LOG_LEVEL_ERROR,
            "U-mode XCPT: scause=0x%" PRIx64 ", sepc=0x%" PRIx64 ", stval=0x%" PRIx64 "\n", scause,
            sepc, stval);

        /* If the kernel exception buffer is available */
        if (exception_buffer != 0)
        {
            internal_execution_context_t context = { .scause = scause,
                .sepc = sepc,
                .sstatus = sstatus,
                .stval = stval,
                .regs = reg };

            Log_Write(LOG_LEVEL_INFO, "exception_handler:Saving context on U-mode exception\r\n");

            /* Save the execution context in the buffer provided */
            CM_To_MM_Save_Execution_Context((execution_context_t *)exception_buffer,
                CM_CONTEXT_TYPE_UMODE_EXCEPTION, hart_id, &context);
        }

        /* Evict S-mode and U-mode Trace buffers to L3. */
        Trace_Evict_CM_Buffer();
        Trace_Evict_UMode_Buffer();

        /* Only send kernel launch exception message once to MM. To avoid flood of global atomics,
        the shire-level check for exception is done first using local atomics. */
        if ((kernel_info_set_local_exception_mask(shire_id, hart_id & (HARTS_PER_SHIRE - 1)) ==
                0) &&
            (kernel_launch_set_global_exception_mask(shire_id) == 0))
        {
            /* Sends exception message to MM */
            send_exception_message(scause, sepc, stval, sstatus, hart_id, shire_id, user_mode);
        }

        return_from_kernel(0, KERNEL_RETURN_EXCEPTION);
    }
}

static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
    uint64_t hart_id, uint32_t shire_id, bool user_mode)
{
    cm_to_mm_message_exception_t message;
    uint8_t kw_base_id;
    uint8_t slot_index;
    int8_t status;

    /* Populate the exception message */
    message.shire_id = shire_id;
    message.hart_id = hart_id;
    message.mcause = mcause;
    message.mepc = mepc;
    message.mtval = mtval;
    message.mstatus = mstatus;

    /* If the exception is recoverable, inform kernel worker, else inform dispatcher */
    if (user_mode)
    {
        Log_Write(LOG_LEVEL_DEBUG, "send_exception_message:Reporting U-mode exception to KW\r\n");

        message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION;

        /* Get the kernel info attributes */
        kernel_info_get_attributes(shire_id, &kw_base_id, &slot_index);

        /* Send exception message to appropriate kernel worker */
        status = CM_To_MM_Iface_Unicast_Send((uint64_t)(kw_base_id + slot_index),
            (uint64_t)(CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + slot_index),
            (cm_iface_message_t *)&message);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "CM->MM:U-mode_exceptionUnicast send failed! Error code: %d\n", status);
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "send_exception_message:Reporting S-mode exception to MM dispatcher\r\n");

        message.header.id = CM_TO_MM_MESSAGE_ID_FW_EXCEPTION;

        /* Send exception message to dispatcher (Master shire Hart 0) */
        status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (cm_iface_message_t *)&message);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "CM->MM:S-mode_exception:Unicast send failed! Error code: %d\n", status);
        }
    }
}
