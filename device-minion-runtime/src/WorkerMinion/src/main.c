#include "build_configuration.h"
#include "cacheops.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "kernel.h"
#include "kernel_config.h"
#include "kernel_info.h"
#include "layout.h"
#include "message.h"
#include "syscall_internal.h"

#include <stdint.h>

// Shared state - Worker minion fetch kernel parameters from these
static const kernel_config_t* const kernel_config = (kernel_config_t*)FW_MASTER_TO_WORKER_KERNEL_CONFIGS;

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    bool result;

    // Setup supervisor trap vector and sscratch
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n"
        : "=&r" (temp)
    );

    const uint64_t shire_id = get_shire_id();
    const uint64_t hart_id = get_hart_id();
    const uint64_t shire_mask = 1ULL << shire_id;
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    message_init_worker(shire_id, hart_id);

    // Enable supervisor software interrupts
    asm volatile (
        "csrsi sie, 0x2     \n" // Enable supervisor software interrupts
        "csrsi sstatus, 0x2 \n" // Enable interrupts
    );

    // First HART in the shire, except Master Shire (32), where H0 of MasterFW will also enable userspace sync minions of that shire
    if ((hart_id % 64U) == 0 && (shire_id != MASTER_SHIRE))
    {
        // Enable all thread1s so they can run BIST, master can communicate with them, etc.
        syscall(SYSCALL_ENABLE_THREAD1_INT, 0, 0xFFFFFFFFu, 0);
    }

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    // TODO run BIST

    WAIT_FLB(thread_count, 31, result);

    // Last thread to join barrier sends ready message to master
    if (result)
    {
        const message_t message = {.id = MESSAGE_ID_SHIRE_READY, .data = {0}};
        message_send_worker(shire_id, hart_id, &message);
    }

    for (;;)
    {
        int64_t rv = -1;

        // Wait for a credit (kernel launch fastpath)
        // or SWI (message passing slow path)
        WAIT_FCC(FCC_0);

        for (uint64_t kernel_id = 0; kernel_id < MAX_SIMULTANEOUS_KERNELS; kernel_id++)
        {
            volatile const kernel_config_t* const kernel_config_ptr = &kernel_config[kernel_id];

            if (kernel_config_ptr->kernel_info.shire_mask & shire_mask)
            {
                const uint64_t* const kernel_entry_addr = (uint64_t*)kernel_config_ptr->kernel_info.compute_pc;
                const uint64_t* const kernel_stack_addr = (uint64_t*)(KERNEL_UMODE_STACK_BASE - (hart_id * KERNEL_UMODE_STACK_SIZE));
                const kernel_params_t* const kernel_params_ptr = kernel_config_ptr->kernel_info.kernel_params_ptr;
                const grid_config_t* const grid_config_ptr = NULL; // TODO FIXME
                const uint64_t kernel_launch_flags = kernel_config_ptr->kernel_launch_flags;

                rv = launch_kernel(kernel_entry_addr, kernel_stack_addr, kernel_params_ptr, grid_config_ptr, kernel_launch_flags);
                break;
            }
        }

        if (rv != 0)
        {
            // Something went wrong launching the kernel.
            // Can't rely on post_kernel_cleanup(), so evict to invalidate.
            for (uint64_t kernel_id = 0; kernel_id < MAX_SIMULTANEOUS_KERNELS; kernel_id++)
            {
                evict(to_L3, &kernel_config[kernel_id], sizeof(kernel_config_t));
            }
        }
    }
}
