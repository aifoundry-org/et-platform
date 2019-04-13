.extern pullVectorTable
.extern swi_handler
.extern syscall_handler
.extern exception_handler

/*-----------------------------------------------------------*/
.section .text, "ax", @progbits
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func

trap_handler:
    addi sp, sp, -( 28 * 8 )
    sd x1,  1  * 8( sp )
    sd x5,  2  * 8( sp )
    sd x6,  3  * 8( sp )
    sd x7,  4  * 8( sp )
    sd x8,  5  * 8( sp )
    sd x9,  6  * 8( sp )
    sd x10, 7  * 8( sp )
    sd x11, 8  * 8( sp )
    sd x12, 9  * 8( sp )
    sd x13, 10 * 8( sp )
    sd x14, 11 * 8( sp )
    sd x15, 12 * 8( sp )
    sd x16, 13 * 8( sp )
    sd x17, 14 * 8( sp )
    sd x18, 15 * 8( sp )
    sd x19, 16 * 8( sp )
    sd x20, 17 * 8( sp )
    sd x21, 18 * 8( sp )
    sd x22, 19 * 8( sp )
    sd x23, 20 * 8( sp )
    sd x24, 21 * 8( sp )
    sd x25, 22 * 8( sp )
    sd x26, 23 * 8( sp )
    sd x27, 24 * 8( sp )
    sd x28, 25 * 8( sp )
    sd x29, 26 * 8( sp )
    sd x30, 27 * 8( sp )
    sd x31, 28 * 8( sp )

    csrr t2, mcause                     /* Leave a0-a7 unmodified for syscalls */
    csrr t3, mepc

test_if_asynchronous:
    srli t4, t2, 0x3f                   /* MSB of mcause is 1 if handing an asynchronous interrupt - shift to LSB to clear other bits. */
    beq t4, x0, handle_synchronous      /* Branch past interrupt handing if not asynchronous. */

test_if_external_interrupt:             /* Check to see if an external interrupt is pending. */
    lui t1, 0x80000                     /* set bit 31 */
    slli t1, t1, 0x20                   /* shift bit 31 to bit 63 */
    addi t1, t1, 0x3                    /* 0x8000000000000003 == Machine software interrupt. */
    beq t2, t1, handle_software_interrupt
    addi t1, t1, 0x8                    /* 0x800000000000000b == Machine external interrupt. */
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
    jal swi_handler
    j processed_source

handle_synchronous:
    addi t3, t3, 4                      /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */

test_if_environment_call:               /* TODO FIXME handle U/S calls in supervisor handler, M calls in separate machine handler */
    li t0, 0x8                          /* 0x8,9,B == environment call from U,S,M-mode. 0xA is reserved, so check bits 3:2. */
    andi t1, t2, 0xC
    bne t0, t1, is_exception            /* Not an environment call, so some other exception. */
    jal syscall_handler
    j processed_source

is_exception:
    jal exception_handler
    ebreak
    j is_exception

as_yet_unhandled:
    ebreak
    j as_yet_unhandled

processed_source:
    csrw mepc, t3                       /* Load mepc with the address of the next instruction in the task to run next. */

    ld  x1,  1  * 8( sp )
    ld  x5,  2  * 8( sp )
    ld  x6,  3  * 8( sp )
    ld  x7,  4  * 8( sp )
    ld  x8,  5  * 8( sp )
    ld  x9,  6  * 8( sp )
    ld  x10, 7  * 8( sp )
    ld  x11, 8  * 8( sp )
    ld  x12, 9  * 8( sp )
    ld  x13, 10 * 8( sp )
    ld  x14, 11 * 8( sp )
    ld  x15, 12 * 8( sp )
    ld  x16, 13 * 8( sp )
    ld  x17, 14 * 8( sp )
    ld  x18, 15 * 8( sp )
    ld  x19, 16 * 8( sp )
    ld  x20, 17 * 8( sp )
    ld  x21, 18 * 8( sp )
    ld  x22, 19 * 8( sp )
    ld  x23, 20 * 8( sp )
    ld  x24, 21 * 8( sp )
    ld  x25, 22 * 8( sp )
    ld  x26, 23 * 8( sp )
    ld  x27, 24 * 8( sp )
    ld  x28, 25 * 8( sp )
    ld  x29, 26 * 8( sp )
    ld  x30, 27 * 8( sp )
    ld  x31, 28 * 8( sp )
    addi sp, sp, 28 * 8

    mret                                /* mstatus.mpp => privilege mode, mstatus.mpie -> mstatus.mie, mepc => pc */
    .endfunc
