#include "worker.h"
#include "interrupt.h"
#include "macros.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"
#include "test_kernels.h"

#include <stdbool.h>

#define MASTER_SHIRE 32U

static void kernel_return_function(void) __attribute__ ((used, section ("kernel")));
static void dummy_kernel_function(void) __attribute__ ((used, section ("kernel")));

static void pre_kernel_setup(void);
static void post_kernel_cleanup(void);

// Runs in S-mode
void WORKER_thread(void)
{
    // Set MIE.MSIE to enable machine software interrupts
    asm volatile ("csrsi mie, 0x8");

    // TODO FIXME all traps and firmware run in M-mode for now
    //asm volatile("csrrsi x0, mideleg, 0x2") // Delegate SSIP so supervisor syscalls are handled in supervisor mode

    // Enable global interrupts
    INT_enableInterrupts();

    // TODO FIXME drop to S-mode here

    while (1)
    {
        pre_kernel_setup();

        // Wait for FCC1 from master indicating the kernel should start
        WAIT_FCC(1);

        // TODO FIXME right now all threads are waiting and synchronizing in setup/cleanup, but what if we don't want all?
        // Do we need to track which N are active per shire instead of assuming 64?

        // TODO FIXME put parameters in registers before calling kernel entry point

        if (get_thread_id() == 0U)
        {
            syscall(SYSCALL_LAUNCH_KERNEL);
        }

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

// This must to a a RX function in user space, no W from user!
static void kernel_return_function(void)
{
    syscall(SYSCALL_RETURN_FROM_KERNEL);
}

static void dummy_kernel_function(void)
{

}

// Runs in S-mode, only need to syscall for M-mode services
static void pre_kernel_setup(void)
{
    bool result;

    // First HART in the shire
    if (get_hart_id() % 64U == 0U)
    {
        // Enable all thread 1s
        syscall(SYSCALL_ENABLE_THREAD1);

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
        // Disable L1 split and scratchpad. Unlocks and evicts all lines.
        syscall(SYSCALL_INIT_L1);
    }

    // Every thread of every minion

    // Empty all FCCs
    for (uint64_t i = read_fcc(0); i > 0; i--)
    {
        WAIT_FCC(0);
    }

    for (uint64_t i = read_fcc(1); i > 0; i--)
    {
        WAIT_FCC(1);
    }

    // Disable message ports
    {
        // portctrl0(0);
        // portctrl1(0);
        // portctrl2(0);
        // portctrl3(0);
    }

    // Zero out TenC
    {
        // TODO (perform a 0*0 operation)
    }

    // TensorExtensionCSRs all 0
    {
        // TODO
        // tensormask_write(0);
        // tensorerror_write(0);
        // for (tmp = 0; tmp <= 255; tmp++)
        //     tensorcooperation_write(tmp);

    }

    // GPR and VPU set to 0s
    {
        // TODO
    }

    // Wipe kernel stack
    {
        // TODO
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

    // Wait for all tensor ops to complete
    WAIT_TENSOR_LOAD_0
    WAIT_TENSOR_LOAD_1
    WAIT_TENSOR_LOAD_L2_0
    WAIT_TENSOR_LOAD_L2_1
    WAIT_PREFETCH_0
    WAIT_PREFETCH_1
    WAIT_CACHEOPS
    WAIT_TENSOR_FMA
    WAIT_TENSOR_STORE
    WAIT_TENSOR_REDUCE

    // TODO FIXME wait for all the tensor ops to complete in each neighborhood before draining coalescing buffer

    // First HART in each neighborhood
    if (get_hart_id() % 16U == 0U)
    {
        syscall(SYSCALL_DRAIN_COALESCING_BUFFER);
    }

    // Thread 0 in each minion
    if (get_thread_id() == 0U)
    {
        syscall(SYSCALL_EVICT_L1_TO_L2);
    }

    // Wait for all L1 to L2 evicts to complete before evicting L2 to L3
    // TODO FIXME which barrier is safe to use here? Can't use one the kernel left in an unknown state.
    // using FLB31 as a reserved FLB for now
    WAIT_FLB(64, 31, result);

    // Last thread to join barrier
    if (result)
    {
        syscall(SYSCALL_EVICT_L2_TO_L3);

        // send FCC1 to master indicating the kernel is complete
        SEND_FCC(MASTER_SHIRE, THREAD_0, FCC_1, 1);
    }
}
