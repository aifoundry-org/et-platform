#include "worker.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"
#include "test_kernels.h"

#include <stdbool.h>

static void pre_kernel_setup(void);
static void post_kernel_cleanup(void);

void WORKER_thread(void)
{
    // enable software interrupts
    asm volatile ("csrsi mie, 0xA"); // Set MSIP and SSIP to enable machine and supervisor software interrupts

    while (1)
    {
        // Drop to U-mode before calling kernel code
        //syscall(SYSCALL_U_MODE);

        // Wait for FCC1 from master
        //WAIT_FCC(1);

        // thread 0s run compute kernel
        if (get_thread_id() == 0)
        {
            pre_kernel_setup();

            // TODO get PC of compute kernel from master net_desc.compute_pc - IPI? Need to define this aspect of Minion API.
            test_compute_kernel();
        }
        // For now, all active thread 1s run prefetch kernel
        else if (get_thread_id() == 1)
        {
            // TODO get PC of prefetch kernel from master - IPI? net_desc only has compute_pc for now.
            test_prefetch_kernel();
        }
        else
        {
            asm volatile ("wfi");
        }

        // send FCC0 to master
        //SEND_FCC(32, 0, 0, 1);

        post_kernel_cleanup();
    }
}

static void pre_kernel_setup(void)
{
    const unsigned int hart_id = get_hart_id();
    const unsigned int thread_id = get_thread_id();

    // First HART in each shire
    if (hart_id % 64U == 0U)
    {
        // Enable minion 0 thread 1
        SET_THREAD1_DISABLE(0xFFFFFFFE);
    }

    // Thread 0 in each minion
    if (thread_id == 0U)
    {
        // Enable L1 split and scratchpad
        syscall(SYSCALL_ENABLE_L1_SCRATCHPAD);

        // Empty all FLBs
        for (unsigned int barrier = 0; barrier < 64; barrier++)
        {
            INIT_FLB(THIS_SHIRE, barrier);
        }
    }

    // Every thread of every minion

    // // Empty all FCCs TODO FIXME these hang
    // while (read_fcc(0) > 0)
    // {
    //     wait_fcc(0);
    // }

    // while (read_fcc(1) > 0)
    // {
    //     wait_fcc(1);
    // }

//     // Disable message ports
//     {
//         // TODO
//     }

//     // Unlock all cachelines
//     {
//         // TODO
//     }

//     // Zero out TenC
//     {
//         // TODO
//     }

//     // TensorExtensionCSRs all 0
//     {
//         // TODO
//     }

//     // GPR and VPU set to 0s
//     {
//         // TODO
//     }

//     // stack pointer initialized
//     {
//         // TODO for now, crt.S is doing this per hartID to fit in ZeBu mini-SoC 32MB DRAM
//     }
}

static void post_kernel_cleanup(void)
{
    const unsigned int hart_id = get_hart_id();
    const unsigned int thread_id = get_thread_id();
    bool result;

    // First HART in each shire
    if (hart_id % 64U == 0U)
    {
        // Disable all thread1s
        SET_THREAD1_DISABLE(0xFFFFFFFF);
    }

    // First HART in each neighborhood
    if (hart_id % 16U == 0U)
    {
        syscall(SYSCALL_DRAIN_COALESCING_BUFFER);
    }

    // thread 0 in each minion
    if (thread_id == 0U)
    {
        syscall(SYSCALL_FLUSH_L1_TO_L2);
    }

    // Wait for all L1 to L2 flushes to complete before flushing L2 to L3
    WAIT_FLB(32, 32, result); // TODO FIXME which barrier is safe to use here? Can't use one the kernel left in an unknown state.

    // Last minion to finish flushing L1
    if (result)
    {
        syscall(SYSCALL_FLUSH_L2_TO_L3);
    }
}
