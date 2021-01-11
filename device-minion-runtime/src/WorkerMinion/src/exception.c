#include "log.h"
#include "macros.h"
#include "kernel_error.h"
#include "kernel_return.h"
#include "hart.h"
#include "message.h"
#include "message_types.h"
#include <stdbool.h>
#include <inttypes.h>

void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg);
static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                                   uint64_t hart_id, bool user_mode);


void exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg)
{
    /* ecalls are handled elsewhere, and some U-mode exceptions are delegated to S-mode from M-mode */
    (void)sepc;
    (void)stval;
    (void)reg;

    log_write(LOG_LEVEL_CRITICAL, "WorkerMinon exception: scause=0x%" PRIx64, scause);

    const uint64_t hart_id = get_hart_id();
    uint64_t sstatus;

    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

    const bool user_mode = ((sstatus & 0x1800U) >> 11U) == 0;
    send_exception_message(scause, sepc, stval, sstatus, hart_id, user_mode);

    return_from_kernel(KERNEL_ERROR_EXCEPTION);
}

static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                                   uint64_t hart_id, bool user_mode)
{
    message_exception_t message;

    // The master minion needs to know if this is a recoverable kernel exception or an unrecoverable exception
    message.header.id = user_mode ? CM_TO_MM_MESSAGE_ID_U_MODE_EXCEPTION : CM_TO_MM_MESSAGE_ID_FW_EXCEPTION;
    message.hart_id   = hart_id;
    message.mcause    = mcause;
    message.mepc      = mepc;
    message.mtval     = mtval;
    message.mstatus   = mstatus;

    message_send_worker(get_shire_id(), hart_id, (cm_iface_message_t *)&message);
}
