#include "layout.h"
#include "log.h"
#include "device-common/macros.h"
#include "common_defs.h"
#include "kernel.h"
#include "device-common/hart.h"
#include "message_types.h"
#include "cm_to_mm_iface.h"
#include "cm_mm_defines.h"
#include "riscv_encoding.h"
#include "syscall_internal.h"
#include <stdbool.h>
#include <inttypes.h>

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
    user_mode = ((sstatus & 0x100U) >> 8U) == 0;

    /* TODO: Save the execution context in the buffer provided as an argument in kernel launch */
    /* Save the execution context in the buffer provided */
    CM_To_MM_Save_Execution_Context((execution_context_t*)CM_EXECUTION_CONTEXT_BUFFER,
        kernel_launch_get_pending_shire_mask(), hart_id, scause, sepc, stval, sstatus, reg);

    if (!user_mode)
    {
        log_write(LOG_LEVEL_CRITICAL,
            "H%04" PRId64 ": Worker S-mode exception: scause=0x%" PRIx64 ", sepc=0x%" PRIx64 ", stval=0x%" PRIx64 "\n",
            hart_id, scause, sepc, stval);

        send_exception_message(scause, sepc, stval, sstatus, hart_id, shire_id, user_mode);
    }

    /* First hart in the shire that took exception will do a self abort
    and send IPI to other harts in the shire to abort as well
    NOTE: The harts in U-mode will trap only on IPI. Harts that will enter exception handler
    must return from kernel since traps on IPIs are disabled in S-mode */
    if(kernel_info_set_abort_flag(shire_id))
    {
        /* Only send kernel launch abort message once to MM.
        MM will send kernel abort message to rest of the shires */
        if(kernel_launch_set_global_abort_flag() && user_mode)
        {
            /* Sends exception message to MM */
            send_exception_message(scause, sepc, stval, sstatus, hart_id, shire_id, user_mode);
        }

        /* Send the IPI to all other Harts in this shire */
        syscall(SYSCALL_IPI_TRIGGER_INT, MASK_RESET_BIT(0xFFFFFFFFFFFFFFFFu, hart_id % 64), shire_id, 0);
    }

    return_from_kernel(KERNEL_ERROR_EXCEPTION);
}

static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                                   uint64_t hart_id, uint32_t shire_id, bool user_mode)
{
    cm_to_mm_message_exception_t message;
    uint8_t kw_base_id;
    uint8_t slot_index;
    int8_t status;

    /* Populate the exception message */
    message.shire_id  = shire_id;
    message.hart_id   = hart_id;
    message.mcause    = mcause;
    message.mepc      = mepc;
    message.mtval     = mtval;
    message.mstatus   = mstatus;

    /* If the exception is recoverable, inform kernel worker, else inform dispatcher */
    if(user_mode)
    {
        message.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION;

        /* Get the kernel info attributes */
        kernel_info_get_attributes(shire_id, &kw_base_id, &slot_index);

        /* Send exception message to appripriate kernel worker */
        status = CM_To_MM_Iface_Unicast_Send((uint64_t)(kw_base_id + slot_index),
            (uint64_t)(CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + slot_index),
            (cm_iface_message_t *)&message);

        if(status != STATUS_SUCCESS)
        {
            log_write(LOG_LEVEL_ERROR,
                "H%04" PRId64 ": CM->MM:U-mode_exceptionUnicast send failed! Error code: " PRIi8 "\n",
                hart_id, status);
        }
    }
    else
    {
        message.header.id = CM_TO_MM_MESSAGE_ID_FW_EXCEPTION;

        /* Send exception message to dispatcher (Master shire Hart 0) */
        status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (cm_iface_message_t *)&message);

        if(status != STATUS_SUCCESS)
        {
            log_write(LOG_LEVEL_ERROR,
                "H%04" PRId64 ": CM->MM:S-mode_exception:Unicast send failed! Error code: " PRIi8 "\n",
                hart_id, status);
        }
    }
}

