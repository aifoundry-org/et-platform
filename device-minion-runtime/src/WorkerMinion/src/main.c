#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "etsoc/isa/hart.h"
#include "etsoc/isa/sync.h"
#include "etsoc/isa/riscv_encoding.h"
#include "transports/mm_cm_iface/message_types.h"
#include "etsoc/drivers/pmu/pmu.h"

#include "layout.h"
#include "log.h"

#include "device_minion_runtime_build_configuration.h"
#include "cm_mm_defines.h"
#include "cm_to_mm_iface.h"
#include "mm_to_cm_iface.h"
#include "trace.h"

#include <stdint.h>

extern void trap_handler(void);

void __attribute__((noreturn)) main(void)
{
    bool result;
    int8_t status;
    spinlock_t *lock;

    // Setup supervisor trap vector
    asm volatile("csrw  stvec, %0\n"
                 : : "r"(&trap_handler));

    // Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    // But enable Supervisor Software Interrupts so that IPIs trap when in U-mode
    // RISC-V spec:
    //   "An interrupt i will be taken if bit i is set in both mip and mie,
    //    and if interrupts are globally enabled."
    asm volatile("csrci sstatus, 0x2\n");
    asm volatile("csrsi sie, %0\n" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    // Enable all available PMU counters to be sampled in U-mode
    asm volatile("csrw scounteren, %0\n"
        : : "r"(((1u << PMU_NR_HPM) - 1) << PMU_FIRST_HPM));

    const uint64_t shire_id = get_shire_id();
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    /* Initialize Trace with default configurations. */
    Trace_Init_CM(NULL);

    Trace_String(TRACE_EVENT_STRING_CRITICAL, Trace_Get_CM_CB(), "Trace Initialized!!\n");

    WAIT_FLB(thread_count, 31, result);

    // Last thread to join barrier sends ready message to master
    if (result) {
        const mm_to_cm_message_shire_ready_t message = {
            .header.id = CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY,
            .shire_id = get_shire_id()
        };

        /* Acquire the unicast lock */
        lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[CM_MM_MASTER_HART_UNICAST_BUFF_IDX];
        acquire_global_spinlock(lock);

        // To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0)
        status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (const cm_iface_message_t *)&message);

        /* Release the unicast lock */
        lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[CM_MM_MASTER_HART_UNICAST_BUFF_IDX];
        release_global_spinlock(lock);

        if(status != 0)
        {
            log_write(LOG_LEVEL_ERROR,
                "H%04lld: CM->MM:Shire_ready:Unicast send failed! Error code: %d\n",
                get_hart_id(), status);
        }
    }

    /* Initialize the MM-CM Iface */
    MM_To_CM_Iface_Init();

    /* Start the main processing loop */
    MM_To_CM_Iface_Main_Loop();
}
