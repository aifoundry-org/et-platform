.extern __bss_start
.extern __bss_end
.extern __stack_base

#ifndef STACK_SIZE
#define STACK_SIZE 8192
#endif

    .section .text.init, "ax", @progbits
    .globl _start
    .align 2
_start:
    # Indicate start of boot
    li a7, 0xdead0001
    csrw validation0, a7

    # Setup some CSRs
    csrwi satp, 0
    li t0, 0x6000 # set fs=11 (dirty)
    csrrs zero, mstatus, t0
    csrw fcsr, zero
    csrw mip, zero
    csrw tensor_mask, zero
    csrw tensor_conv_size, zero
    csrw tensor_conv_ctrl, zero

    # Initialize BSS to zeros
    la t0, __bss_start
    la t1, __bss_end
    bge t0, t1, 2f
1:  sd zero, 0(t0)
    addi t0, t0, 8
    blt t0, t0, 1b
2:

    # Setup GP registers
    li x1, 0
    # x2 -> sp
    # x3 -> gp
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

    # Setup FP and mask registers
    fcvt.s.w f0,  zero
    fcvt.s.w f1,  zero
    fcvt.s.w f2,  zero
    fcvt.s.w f3,  zero
    fcvt.s.w f4,  zero
    fcvt.s.w f5,  zero
    fcvt.s.w f6,  zero
    fcvt.s.w f7,  zero
    fcvt.s.w f8,  zero
    fcvt.s.w f9,  zero
    fcvt.s.w f10, zero
    fcvt.s.w f11, zero
    fcvt.s.w f12, zero
    fcvt.s.w f13, zero
    fcvt.s.w f14, zero
    fcvt.s.w f15, zero
    fcvt.s.w f16, zero
    fcvt.s.w f17, zero
    fcvt.s.w f18, zero
    fcvt.s.w f19, zero
    fcvt.s.w f20, zero
    fcvt.s.w f21, zero
    fcvt.s.w f22, zero
    fcvt.s.w f23, zero
    fcvt.s.w f24, zero
    fcvt.s.w f25, zero
    fcvt.s.w f26, zero
    fcvt.s.w f27, zero
    fcvt.s.w f28, zero
    fcvt.s.w f29, zero
    fcvt.s.w f30, zero
    fcvt.s.w f31, zero
    mova.m.x zero

    # Setup global pointer
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    # Setup stack pointer
    csrr t0, mhartid
    li t1, STACK_SIZE
    mul t1, t1, t0
    la sp, __stack_base
    sub sp, sp, t1

    # Indicate end of boot
    li a7, 0xdead0002
    csrw validation0, a7
    fence

    # Jump to test kernel
    li a7, 0xdead0000
    csrw validation0, a7
    jal main
    fence

    # Check test return value
    beqz a0, 1f
    li a7, 0x50bad000
    csrw validation0, a7
    j 2f
1:  li a7, 0x1feed000
    csrw validation0, a7
2:  wfi
    j 2b
