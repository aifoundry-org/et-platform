#include "device_minion_runtime_build_configuration.h"
#include "cacheops.h"
#include "device-mrt-trace.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "kernel.h"
#include "kernel_config.h"
#include "layout.h"
#include "message.h"
#include "syscall_internal.h"

#include <stdint.h>

// Shared state - Worker minion fetch kernel parameters from these
static const kernel_config_t *const kernel_config =
    (kernel_config_t *)FW_MASTER_TO_WORKER_KERNEL_CONFIGS;

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    bool result;

    // Setup supervisor trap vector and sscratch
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

#ifndef IMPLEMENTATION_BYPASS
    // Init trace for worker minion
    TRACE_init_worker();
    TRACE_string(LOG_LEVELS_CRITICAL, "Trace message from worker");
#endif /* IMPLEMENTATION_BYPASS */

    const uint64_t shire_id = get_shire_id();
    const uint64_t hart_id = get_hart_id();
    const uint64_t shire_mask = 1ULL << shire_id;
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    message_init_worker(shire_id, hart_id);

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    // TODO run BIST

    WAIT_FLB(thread_count, 31, result);

    // Last thread to join barrier sends ready message to master
    if (result) {
        const cm_iface_message_t message = {
            .header.id = CM_TO_MM_MESSAGE_ID_SHIRE_READY,
            .data = { 0 },
        };
        message_send_worker(shire_id, hart_id, &message);
    }

    // Enable supervisor software interrupts
    asm volatile("csrsi sie, 0x2\n");

    for (;;) {
        int64_t rv = -1;

        // Enable global interrupts (sstatus.SIE = 1)
        asm volatile("csrsi sstatus, 0x2\n");

        // Wait for a credit (kernel launch fastpath)
        // or SWI (message passing slow path)
        WAIT_FCC(FCC_0);

        for (uint64_t i = 0; i < MAX_SIMULTANEOUS_KERNELS; i++) {
            volatile const kernel_config_t *const kernel_config_ptr = &kernel_config[i];

            if (kernel_config_ptr->shire_mask & shire_mask) {
                uint64_t kernel_id = kernel_config_ptr->kernel_id;
                const uint64_t *const code_start_address =
                    (uint64_t *)kernel_config_ptr->code_start_address;
                const uint64_t *const kernel_stack_addr =
                    (uint64_t *)(KERNEL_UMODE_STACK_BASE - (hart_id * KERNEL_UMODE_STACK_SIZE));
                const uint64_t *const pointer_to_args =
                    (uint64_t *)kernel_config_ptr->pointer_to_args;
                const uint64_t kernel_launch_flags = kernel_config_ptr->kernel_launch_flags;

                rv = launch_kernel(kernel_id, code_start_address, kernel_stack_addr, pointer_to_args,
                                   kernel_launch_flags);
                break;
            }
        }

        if (rv != 0) {
            // Something went wrong launching the kernel.
            // Can't rely on post_kernel_cleanup(), so evict to invalidate.
            for (uint64_t i = 0; i < MAX_SIMULTANEOUS_KERNELS; i++) {
                evict(to_L3, &kernel_config[i], sizeof(kernel_config_t));
            }
        }
    }
}
