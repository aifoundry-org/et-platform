.section .text, "ax", @progbits /* TODO FIXME move to S +RX page sections */
.global trap_handler

.macro save_context
    addi sp, sp, -( 29 * 8 )
    sd x1,  1  * 8( sp )
    sd x3,  2  * 8( sp )
    sd x5,  3  * 8( sp )
    sd x6,  4  * 8( sp )
    sd x7,  5  * 8( sp )
    sd x8,  6  * 8( sp )
    sd x9,  7  * 8( sp )
    sd x10, 8  * 8( sp )
    sd x11, 9  * 8( sp )
    sd x12, 10 * 8( sp )
    sd x13, 11 * 8( sp )
    sd x14, 12 * 8( sp )
    sd x15, 13 * 8( sp )
    sd x16, 14 * 8( sp )
    sd x17, 15 * 8( sp )
    sd x18, 16 * 8( sp )
    sd x19, 17 * 8( sp )
    sd x20, 18 * 8( sp )
    sd x21, 19 * 8( sp )
    sd x22, 20 * 8( sp )
    sd x23, 21 * 8( sp )
    sd x24, 22 * 8( sp )
    sd x25, 23 * 8( sp )
    sd x26, 24 * 8( sp )
    sd x27, 25 * 8( sp )
    sd x28, 26 * 8( sp )
    sd x29, 27 * 8( sp )
    sd x30, 28 * 8( sp )
    sd x31, 29 * 8( sp )
    .endm

.macro restore_context
    ld  x1,  1  * 8( sp )
    ld  x3,  2  * 8( sp )
    ld  x5,  3  * 8( sp )
    ld  x6,  4  * 8( sp )
    ld  x7,  5  * 8( sp )
    ld  x8,  6  * 8( sp )
    ld  x9,  7  * 8( sp )
    ld  x10, 8  * 8( sp )
    ld  x11, 9  * 8( sp )
    ld  x12, 10 * 8( sp )
    ld  x13, 11 * 8( sp )
    ld  x14, 12 * 8( sp )
    ld  x15, 13 * 8( sp )
    ld  x16, 14 * 8( sp )
    ld  x17, 15 * 8( sp )
    ld  x18, 16 * 8( sp )
    ld  x19, 17 * 8( sp )
    ld  x20, 18 * 8( sp )
    ld  x21, 19 * 8( sp )
    ld  x22, 20 * 8( sp )
    ld  x23, 21 * 8( sp )
    ld  x24, 22 * 8( sp )
    ld  x25, 23 * 8( sp )
    ld  x26, 24 * 8( sp )
    ld  x27, 25 * 8( sp )
    ld  x28, 26 * 8( sp )
    ld  x29, 27 * 8( sp )
    ld  x30, 28 * 8( sp )
    ld  x31, 29 * 8( sp )
    addi sp, sp, 29 * 8
    .endm

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func

trap_handler:
    csrrw sp, sscratch, sp              /* Swap sp with sscratch */
    save_context

    csrr t2, scause                     /* Leave a0-a3 unmodified for environment calls */
    csrr s1, sepc

test_if_asynchronous:
    bgt t2, x0, handle_synchronous      /* Branch past asynchronous handling if not asynchronous. */

test_if_external_interrupt:             /* Check to see if an external interrupt is pending. */
    lui t1, 0x80000                     /* set bit 31 */
    slli t1, t1, 0x20                   /* shift bit 31 to bit 63 */
    addi t1, t1, 0x2                    /* 0x8000000000000002 == Supervisor software interrupt. */
    beq t2, t1, handle_software_interrupt
    addi t1, t1, 0x7                    /* 0x8000000000000009 == Supervisor external interrupt. */
    bne t2, t1, as_yet_unhandled        /* Something as yet unhandled. */

handle_external_interupt:
    li s0, 0x0010200004                 /* Load the address of ulMaxID into s0. */
    ld t0, pullVectorTable              /* Load base address of vector table into t0 */
    lwu s1, 0(s0)                       /* Load ulMaxID into s1 to claim the interrupt */
    beqz s1, processed_source           /* If ulMaxID is zero, the interrupt has already been claimed (unlikely) */
    slli t1, s1, 3                      /* Shift ulMaxID to 8-byte aligned index into vector table in t1 */
    add t0, t0, t1                      /* Add 8-byte aligned index to base address of vector table in t0 */
    ld t1, 0(t0)                        /* load vector from vector table into t1 */
    jalr t1                             /* jump to interrupt vector */
    sw s1, 0(s0)                        /* write ulMaxID to indicate the interrupt has been serviced */
    j processed_source

handle_software_interrupt:
    la ra, processed_source
    j swi_handler

handle_synchronous:
    addi s1, s1, 4                      /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */

.option push
.option norelax
    la   gp, __global_pointer$          /* Switch to global pointer for this image */
.option pop

test_if_environment_call:
    li t0, 0x8                          /* 0x8 == user environment call */
    bne t2, t0, is_exception            /* Not an environment call, so some other exception. */
    jal syscall_handler
    sd   a0, 8 * 8( sp )                /* update a0 in saved context with environment call return value */
    j processed_source

is_exception:
    la ra, processed_source
    j exception_handler

as_yet_unhandled:
    ebreak
    j as_yet_unhandled

processed_source:
    csrw sepc, s1                       /* Load mepc with the address of the next instruction in the task to run next. */
    restore_context
    csrrw sp, sscratch, sp              /* Swap sp with sscratch */
    sret                                /* mstatus.mpp => privilege mode, mstatus.mpie -> mstatus.mie, mepc => pc */
    .endfunc
