#include "syscall.h"
#include "cacheops.h"
#include "shire.h"

#include <stdint.h>

int syscall_handler(unsigned int syscall);

static void launch_kernel(void);
static void return_from_kernel(void);
static void enable_thread1(void);

static void evict_l1_to_l2(void);
static void evict_l2_to_l3(void);
static void init_l1(void);

// TODO FIXME break into different handlers depending on where call is from:
// U mode kernel
// S mode firmware
// M mode firmware (should only be due to faults)

int syscall_handler(unsigned int syscall)
{
    switch (syscall)
    {
        case SYSCALL_EVICT_L1_TO_L2:
            evict_l1_to_l2();
        break;

        case SYSCALL_EVICT_L2_TO_L3:
            evict_l2_to_l3();
        break;

        case SYSCALL_INIT_L1:
            init_l1();
        break;

        case SYSCALL_DRAIN_COALESCING_BUFFER:
            // todo
        break;

        case SYSCALL_LAUNCH_KERNEL:
            launch_kernel();
        break;

        case SYSCALL_RETURN_FROM_KERNEL:
            return_from_kernel();
        break;

        case SYSCALL_ENABLE_THREAD1:
            enable_thread1();
        break;

        default:
            // unhandled syscall!
        break;
    }

    return 0;
}

uint64_t kernel_call_depth = 0;

typedef struct
{
    uint64_t sp;
} tcb_t;

tcb_t firmware_tcb; // or use mscratch to hold firmware sp?

// Saves firmware context and launches kernel in user mode with clean state
static void launch_kernel(void)
{
    const uint64_t* const kernel_entry_addr  = (uint64_t*)0x800020102c; // TODO FIXME parameter
    const uint64_t* const return_from_kernel_addr = (uint64_t*)0x8000201000; //return_from_kernel_function;
    const uint64_t* const kernel_stack_addr  = (uint64_t*)0x8000201000; // TODO FIXME HACK

    uint64_t temp;

    if (kernel_call_depth == 0)
    {
        kernel_call_depth++;

        // We've already saved context on the supervisor stack in the trap_handler
        // push pc to stack, sp to firmware_tcb
        // Switch to kernel stack
        // Store a frame on the kernel stack so it can ret to return_from_kernel_addr
        // wipe register state (no leakage from S to U mode)
        // mret to kernel task in user mode
        asm volatile (
            "csrr  x1, mepc			    \n" // load pc from mepc TODO FIXME Could just use t3 from trap handler, already has mepc in it
            "sd    x1, 0(sp)            \n" // push pc to stack
            "addi  sp, sp, -8           \n" //
            "sd    sp, %0               \n" // store sp to firmware_tcb
            "mv    ra, %2               \n" // set ra to return_from_kernel_addr
            "mv    s0, %3               \n" // switch to kernel stack: set s0 (frame pointer) to kernel_stack_addr
            "addi  sp, s0, -32          \n" // switch to kernel stack: set sp to kernel stack after stack frame
            "sd	   ra, 24(sp)           \n" // push ra
            "sd	   s0, 16(sp)           \n" // push s0
            "li    %1, 0x1800           \n" // bitmask to clear mstatus MPP=user
            //"li    %1, 0x100            \n" // bitmask to clear mstatus SPP=user *** TODO FIXME change from mret to sret ***
            "csrc  mstatus, %1          \n" // clear mstatus xPP
            "csrsi mstatus, 0x10        \n" // set mstatus UPIE
            "csrw  mepc, %4             \n" // kernel address to jump to in user mode
            //"csrw  sepc, %4             \n" // kernel address to jump to in user mode *** TODO FIXME change from mret to sret ***
            "mv    x5, x0               \n" // Wipe registers: don't leak state from S to U
            "mv    x6, x0               \n"
            "mv    x7, x0               \n"
            "mv    x8, x0               \n"
            "mv    x9, x0               \n"
            //"mv    x10, x0              \n" TODO pass argument in a0
            //"mv    x11, x0              \n" TODO pass argument in a1
            "mv    x12, x0              \n"
            "mv    x13, x0              \n"
            "mv    x14, x0              \n"
            "mv    x15, x0              \n"
            "mv    x16, x0              \n"
            "mv    x17, x0              \n"
            "mv    x18, x0              \n"
            "mv    x19, x0              \n"
            "mv    x20, x0              \n"
            "mv    x21, x0              \n"
            "mv    x22, x0              \n"
            "mv    x23, x0              \n"
            "mv    x24, x0              \n"
            "mv    x25, x0              \n"
            "mv    x26, x0              \n"
            "mv    x27, x0              \n"
            "mv    x28, x0              \n"
            "mv    x29, x0              \n"
            "mv    x30, x0              \n"
            "mv    x31, x0              \n"
            "mret                       \n" // mstatus.mpp => privilege mode, sepc => pc
            //"sret                       \n" // mstatus.mpp => privilege mode, sepc => pc *** TODO FIXME change from mret to sret ***
            : "=m" (firmware_tcb), "=&r" (temp)
            : "r" (return_from_kernel_addr), "r" (kernel_stack_addr), "r" (kernel_entry_addr)
        );
    }
    else
    {
        asm volatile ("ebreak");
    }
}

// Restores firmware context from user mode after a kernel completes
static void return_from_kernel(void)
{
    uint64_t temp;

    if (kernel_call_depth == 1)
    {
        kernel_call_depth--;

        // Switch to kernel stack via firmware_tcb
        // restore context from stack
	    asm volatile (
            "ld   sp, %1               \n" // load sp from tcb
            "ld   x1,  1  * 8( sp )	   \n" // load pc from stack
	        "csrw mepc, x1             \n" // load mepc from pc *** TODO FIXME change from mpec to sepc ***
            //"csrw sepc, x1             \n" // load sepc from pc
            "ld   x1,  2  * 8( sp )    \n" // restore context
            "ld   x5,  3  * 8( sp )    \n"
            "ld   x6,  4  * 8( sp )    \n"
            "ld   x7,  5  * 8( sp )    \n"
            "ld   x8,  6  * 8( sp )    \n"
            "ld   x9,  7  * 8( sp )    \n"
            "ld   x10, 8  * 8( sp )    \n"
            "ld   x11, 9  * 8( sp )    \n"
            "ld   x12, 10 * 8( sp )    \n"
            "ld   x13, 11 * 8( sp )    \n"
            "ld   x14, 12 * 8( sp )    \n"
            "ld   x15, 13 * 8( sp )    \n"
            "ld   x16, 14 * 8( sp )    \n"
            "ld   x17, 15 * 8( sp )    \n"
            "ld   x18, 16 * 8( sp )    \n"
            "ld   x19, 17 * 8( sp )    \n"
            "ld   x20, 18 * 8( sp )    \n"
            "ld   x21, 19 * 8( sp )    \n"
            "ld   x22, 20 * 8( sp )    \n"
            "ld   x23, 21 * 8( sp )    \n"
            "ld   x24, 22 * 8( sp )    \n"
            "ld   x25, 23 * 8( sp )    \n"
            "ld   x26, 24 * 8( sp )    \n"
            "ld   x27, 25 * 8( sp )    \n"
            "ld   x28, 26 * 8( sp )    \n"
            "ld   x29, 27 * 8( sp )    \n"
            "ld   x30, 28 * 8( sp )    \n"
            "ld   x31, 29 * 8( sp )    \n"
            "addi sp, sp, 29 * 8       \n"
            "li    %0, 0x1880          \n" // bitmask to set mstatus MPP=supervisor and MPIE
            //"li    %0, 0x120           \n" // bitmask to set mstatus SPP and SPIE *** TODO FIXME change from mret to sret ***
            "csrs  mstatus, %0         \n" // clear mstatus xPP
            "mret                      \n"
            //"sret                      \n"*** TODO FIXME change from mret to sret ***
            : "=&r" (temp)
            : "m" (firmware_tcb)
        );
    }
    else
    {
        asm volatile ("ebreak");
    }
}

static void enable_thread1(void)
{
    // Set 1 to DISABLE a thread 1: bit 0 = minion 0, bit 31 = minion 31
    *(volatile uint64_t* const)(0x1C0340010ULL) = 0;
}

// Can only be run from M-mode
static void evict_l1_to_l2(void)
{
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    const uint64_t mcache_control_reg = mcache_control_get();

    excl_mode(1); // get exclusive access to the processor

    if ((mcache_control_reg & 0x3) == 3)
    {
        // Split mode with scratchpad enabled. Evict sets 12-15
        evict_sw(0, 1, 0, 12, 3, 0);
        evict_sw(0, 1, 1, 12, 3, 0);
        evict_sw(0, 1, 2, 12, 3, 0);
        evict_sw(0, 1, 3, 12, 3, 0);
    }
    else if ((mcache_control_reg & 0x3) == 1)
    {
        // Split mode with scratchpad disabled. Evict sets 0-7 and 14-15
        evict_sw(0, 1, 0, 0, 7, 0);
        evict_sw(0, 1, 1, 0, 7, 0);
        evict_sw(0, 1, 2, 0, 7, 0);
        evict_sw(0, 1, 3, 0, 7, 0);

        evict_sw(0, 1, 0, 14, 1, 0);
        evict_sw(0, 1, 1, 14, 1, 0);
        evict_sw(0, 1, 2, 14, 1, 0);
        evict_sw(0, 1, 3, 14, 1, 0);
    }
    else
    {
        // Shared mode. Evict all sets 0-15
        evict_sw(0, 1, 0, 0, 15, 0);
        evict_sw(0, 1, 1, 0, 15, 0);
        evict_sw(0, 1, 2, 0, 15, 0);
        evict_sw(0, 1, 3, 0, 15, 0);
    }

    WAIT_CACHEOPS
    FENCE

    excl_mode(0); // release exclusive access to the processor
}

static void evict_l2_to_l3(void)
{
    // TODO
}

// Can only be run from M-mode
static void init_l1(void)
{
    // TODO FIXME RTLMIN-3596 CSR mcache_control should return the last written value instead of 0s
    const uint64_t mcache_control_reg = mcache_control_get();

    // If the cache has split or scratchpad enabled, reset to shared mode
    if ((mcache_control_reg & 0x3) != 0)
    {
        // Disable L1 split and scratchpad per PRM-8 Cache Control Extension section 1.1.3
        // "Changing configuration from Split back to Shared" and "Enabling/Disabling the Scratchpad"
        excl_mode(1); // get exclusive access to the processor

        if ((mcache_control_reg & 0x3) == 3)
        {
            // Split mode with scratchpad enabled
            // No need to unlock lines: we will turn D1Split off which resets all sets
            // Evict sets 12-15 (12-13 for HART 0, 14-15 for HART1)
            evict_sw(0, 1, 0, 12, 3, 0);
            evict_sw(0, 1, 1, 12, 3, 0);
            evict_sw(0, 1, 2, 12, 3, 0);
            evict_sw(0, 1, 3, 12, 3, 0);
        }
        else if ((mcache_control_reg & 0x3) == 1)
        {
            // Split mode with scratchpad disabled
            // No need to unlock lines: we will turn D1Split off which resets all sets
            // Evict sets 0-7 for HART0
            evict_sw(0, 1, 0, 0, 7, 0);
            evict_sw(0, 1, 1, 0, 7, 0);
            evict_sw(0, 1, 2, 0, 7, 0);
            evict_sw(0, 1, 3, 0, 7, 0);

            // Evict sets 14-15 for HART1
            evict_sw(0, 1, 0, 14, 1, 0);
            evict_sw(0, 1, 1, 14, 1, 0);
            evict_sw(0, 1, 2, 14, 1, 0);
            evict_sw(0, 1, 3, 14, 1, 0);
        }
        else
        {
            // This should never happen
            asm volatile ("ebreak");
        }

        WAIT_CACHEOPS
        FENCE

        // Comments in PRM-8 section 1.1.1:
        // "Missing specification of what happens when going from shared to split mode and vice versa. Do we clear the whole cache?
        //  The RTL implements the correct behaviour that should be specified here, which is to clear all sets when changing to/from shared mode."
        mcache_control(0, 0, 0); // Write 0 into mcache_control.D1Split

        excl_mode(0); // release exclusive access to the processor
    }
    else
    {
        // Cache is in shared mode
        // Unlock sets 0-15
        for (uint64_t set = 0; set < 16; set++)
        {
            for (uint64_t way = 0; way < 4; way++)
            {
                unlock_sw(way, set, 0);
            }
        }
    }
}
