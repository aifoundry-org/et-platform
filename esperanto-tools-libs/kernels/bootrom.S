#=========================================================================
# bootrom.S : Startup entry point for RISC-V Esperanto SOC
#=========================================================================

#include "sys_inc.h"

    .text
    .global _start
_start:
# init system CSR
    csrwi satp, 0
    csrr t0, mstatus
    lui t1, 0x6
    or t0, t0, t1
    csrw mstatus, t0
    fssr zero
    csrw mip, zero

# zero global regs
    li ra, 0
    li gp, 0
    li tp, 0
    li t0, 0
    li t1, 0
    li t2, 0
    li s0, 0
    li s1, 0
    li a0, 0
    li a1, 0
    li a2, 0
    li a3, 0
    li a4, 0
    li a5, 0
    li a6, 0
    li a7, 0
    li s2, 0
    li s3, 0
    li s4, 0
    li s5, 0
    li s6, 0
    li s7, 0
    li s8, 0
    li s9, 0
    li s10, 0
    li s11, 0
    li t3, 0
    li t4, 0
    li t5, 0
    li t6, 0

# zero FP regs
    fcvt.s.w ft0, zero
    fcvt.s.w ft1, zero
    fcvt.s.w ft2, zero
    fcvt.s.w ft3, zero
    fcvt.s.w ft4, zero
    fcvt.s.w ft5, zero
    fcvt.s.w ft6, zero
    fcvt.s.w ft7, zero
    fcvt.s.w fs0, zero
    fcvt.s.w fs1, zero
    fcvt.s.w fa0, zero
    fcvt.s.w fa1, zero
    fcvt.s.w fa2, zero
    fcvt.s.w fa3, zero
    fcvt.s.w fa4, zero
    fcvt.s.w fa5, zero
    fcvt.s.w fa6, zero
    fcvt.s.w fa7, zero
    fcvt.s.w fs2, zero
    fcvt.s.w fs3, zero
    fcvt.s.w fs4, zero
    fcvt.s.w fs5, zero
    fcvt.s.w fs6, zero
    fcvt.s.w fs7, zero
    fcvt.s.w fs8, zero
    fcvt.s.w fs9, zero
    fcvt.s.w fs10, zero
    fcvt.s.w fs11, zero
    fcvt.s.w ft8, zero
    fcvt.s.w ft9, zero
    fcvt.s.w ft10, zero
    fcvt.s.w ft11, zero
    li zero, 15
    mova.m.x zero

    csrr a0, mhartid
    andi a0, a0, 0x1

    bne a0, zero, skip_cache_lock

# Enable the scratchpad and allocate ways 0, 1 and 2 for all sets
# This is done in M-mode
# volatile char __attribute__((aligned(4096))) scp[4096];
   li a1, 0x20000000
# Lock S/W function (0x0)
   addi a3, zero, 16
   li   a4, 0x080000000001000
   li   a5, 0x100000000002000
loop_lock:
# Lock way 0
   csrw 0x7FD, a1
   add  a1, a1, a4
# Lock way 1
   csrw 0x7FD, a1
   add  a1, a1, a4
# Lock way 2
   csrw 0x7FD, a1
# Move to next CL, way 0
   addi a1, a1, 64
   addi a3, a3, -1
   xor  a1, a1, a5
   bne  a3, zero, loop_lock

skip_cache_lock:

# load init and trap entry addresses
# immediately evict address from caches as it will be rewrited in future
    li a0, LAUNCH_PARAMS_AREA_BASE
    ld a1, 0(a0) # init
    ld a2, 8(a0) # trap

# enable shadow registers for hartid and sleep txfma
    csrwi 0x7d2, 0x3
# configure trap vector and move to user mode
    csrw mtvec, a2
    li t0, 0x1800
    csrrc x0, mstatus, t0 # clear mstatus.mpp
    csrw mepc, a1 # set mepc to user-mode entry point
    mret # go to user mode
