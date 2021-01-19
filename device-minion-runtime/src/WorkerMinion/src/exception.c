#include "log.h"
#include "macros.h"
#include "kernel_error.h"
#include "kernel_return.h"
#include "hart.h"
#include "message.h"
#include "message_types.h"
#include "mm_iface.h"
#include <stdbool.h>
#include <inttypes.h>

void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg);
static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                                   uint64_t hart_id, bool user_mode);

void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg)
{
    /* ecalls are handled elsewhere, and some U-mode exceptions are delegated to S-mode from M-mode */

    log_write(LOG_LEVEL_CRITICAL, "H%04" PRId64 ": WorkerMinon exception: scause=0x%" PRIx64 " @ 0x%" PRIx64 "\n", get_hart_id(), scause, sepc);

    const uint64_t hart_id = get_hart_id();
    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

    const bool user_mode = ((sstatus & 0x1800U) >> 11U) == 0;
    send_exception_message(scause, sepc, stval, sstatus, hart_id, user_mode);

    // TODO: Save context to Exception Buffer (if present)
    (void) reg;

    return_from_kernel(KERNEL_ERROR_EXCEPTION);
}

static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                                   uint64_t hart_id, bool user_mode)
{
    cm_to_mm_message_exception_t message;

    // The master minion needs to know if this is a recoverable kernel exception or an unrecoverable exception
    message.header.id = user_mode ? CM_TO_MM_MESSAGE_ID_U_MODE_EXCEPTION : CM_TO_MM_MESSAGE_ID_FW_EXCEPTION;
    message.hart_id   = hart_id;
    message.mcause    = mcause;
    message.mepc      = mepc;
    message.mtval     = mtval;
    message.mstatus   = mstatus;

    // TODO: Retrieve kernel_id/kw_id...
    CM_To_MM_Iface_Unicast_Send(0, (cm_iface_message_t *)&message);
}

