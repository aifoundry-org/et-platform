#include "device_minion_runtime_build_configuration.h"
#include "device-mrt-trace.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "message_types.h"
#include "cm_to_mm_iface.h"
#include "mm_to_cm_iface.h"
#include "riscv_encoding.h"

#include <stdint.h>

extern void MM_To_CM_Iface_Process(void);

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    bool result;

    // Setup supervisor trap vector and sscratch
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

    const uint64_t shire_id = get_shire_id();
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    WAIT_FLB(thread_count, 31, result);

    // Last thread to join barrier sends ready message to master
    if (result) {
        const mm_to_cm_message_shire_ready_t message = {
            .header.id = CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY,
            .shire_id = get_shire_id()
        };

        // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
        CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_UNICAST_BUFF_IDX, 0,
            (const cm_iface_message_t *)&message);
    }

    // Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    // But enable Supervisor Software Interrupts so that IPIs trap when in U-mode
    // RISC-V spec:
    //   "An interrupt i will be taken if bit i is set in both mip and mie,
    //    and if interrupts are globally enabled."
    asm volatile("csrci sstatus, 0x2\n");
    asm volatile("csrsi sie, %0\n" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    for (;;) {
        // Wait for an IPI (Software Interrupt)
        asm volatile("wfi\n");

        // We got a software interrupt (IPI) handed down from M-mode.
        // M-mode already cleared the MSIP (Machine Software Interrupt Pending)
        // Clear Supervisor Software Interrupt Pending (SSIP)
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        // Handle messages from MM
        MM_To_CM_Iface_Process();
    }
}
