#include "worker.h"
#include "interrupt.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"
#include "test_kernels.h"

#include <stdbool.h>

#define MASTER_SHIRE 32U

static void pre_kernel_setup(void);
static void post_kernel_cleanup(void);

void WORKER_thread(void)
{
    // Set MIE.MSIE to enable machine software interrupts
    asm volatile ("csrsi mie, 0x8");

    // Enable global interrupts
    INT_enableInterrupts();

    while (1)
    {
        pre_kernel_setup();

        // Drop to U-mode before calling kernel code
        //syscall(SYSCALL_U_MODE);

        // Wait for FCC1 from master indicating the kernel should start
        WAIT_FCC(1);

        // Put parameters in registers before calling kernel entry point

        // jal kernel entry point

        // Test kernel:
        // // thread 0s run compute kernel
        // if (get_thread_id() == 0)
        // {
        //     // TODO get PC of compute kernel from master net_desc.compute_pc - IPI? Need to define this aspect of Minion API.
        //     test_compute_kernel();
        // }
        // // For now, all active thread 1s run prefetch kernel
        // else if (get_thread_id() == 1)
        // {
        //     // TODO get PC of prefetch kernel from master - IPI? net_desc only has compute_pc for now.
        //     test_prefetch_kernel();
        // }
        // else
        // {
        //     asm volatile ("wfi");
        // }

        post_kernel_cleanup();
    }
}

static void pre_kernel_setup(void)
{
    bool result;

    // First HART in the shire
    if (get_hart_id() % 64U == 0U)
    {
        // Enable all thread 1s
        SET_THREAD1_DISABLE(0);

        // Init all kernel FLBs except reserved FLB 31
        for (unsigned int barrier = 0; barrier < 31; barrier++)
        {
            INIT_FLB(THIS_SHIRE, barrier);
        }
    }
    else
    {
        // All the fast local barriers are in an unknown state
        // TODO FIXME need a sequence to init them that works
        // for starting FLB in 0...0xFFFF
    }

    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        // Enable L1 split and scratchpad
        syscall(SYSCALL_ENABLE_L1_SCRATCHPAD);
    }

    // Every thread of every minion

    // Empty all FCCs
    while (read_fcc(0) > 0)
    {
        WAIT_FCC(0);
    }

    while (read_fcc(1) > 0)
    {
        WAIT_FCC(1);
    }

    // Disable message ports
    {
        // TODO
    }

    // Unlock all cachelines
    {
        // TODO
    }

    // Zero out TenC
    {
        // TODO
    }

    // TensorExtensionCSRs all 0
    {
        // TODO
    }

    // GPR and VPU set to 0s
    {
        // TODO
    }

    // Initialize stack pointer
    {
        // TODO for now, crt.S is doing this per hartID to fit in ZeBu mini-SoC 32MB DRAM
    }

    WAIT_FLB(64, 31, result);

    if (result)
    {
        // send FCC1 to master to let it know this shire is ready
        SEND_FCC(MASTER_SHIRE, THREAD_0, FCC_1, 1);
    }
}

static void post_kernel_cleanup(void)
{
    bool result;

    // First HART in each neighborhood
    if (get_hart_id() % 16U == 0U)
    {
        syscall(SYSCALL_DRAIN_COALESCING_BUFFER);
    }

    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        syscall(SYSCALL_FLUSH_L1_TO_L2);
    }

    // Wait for all L1 to L2 flushes to complete before flushing L2 to L3
    // TODO FIXME which barrier is safe to use here? Can't use one the kernel left in an unknown state.
    // using FLB31 as a reserved FLB for now
    WAIT_FLB(64, 31, result);

    // Last thread to join barrier
    if (result)
    {
        syscall(SYSCALL_FLUSH_L2_TO_L3);

        // send FCC1 to master indicating the kernel is complete
        SEND_FCC(MASTER_SHIRE, THREAD_0, FCC_1, 1);
    }
}
