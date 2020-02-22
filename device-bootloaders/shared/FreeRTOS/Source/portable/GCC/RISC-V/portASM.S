/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#if __riscv_xlen == 64
	#define portWORD_SIZE 8
#elif __riscv_xlen == 32
	#error Not implemented yet - change ld to lw, and sd to sw.
	#define portWORD_SIZE 4
#else
	#error Assembler did not define __riscv_xlen
#endif

/*
 * The FreeRTOS kernel's RISC-V port is split between the the code that is
 * common across all currently supported RISC-V chips (implementations of the
 * RISC-V ISA), and code which tailors the port to a specific RISC-V chip:
 *
 * + The code that is common to all RISC-V chips is implemented in
 *   FreeRTOS\Source\portable\GCC\RISC-V-RV32\portASM.S.  There is only one
 *   portASM.S file because the same file is used no matter which RISC-V chip is
 *   in use.
 *
 * + The code that tailors the kernel's RISC-V port to a specific RISC-V
 *   chip is implemented in freertos_risc_v_chip_specific_extensions.h.  There
 *   is one freertos_risc_v_chip_specific_extensions.h that can be used with any
 *   RISC-V chip that both includes a standard CLINT and does not add to the
 *   base set of RISC-V registers.  There are additional
 *   freertos_risc_v_chip_specific_extensions.h files for RISC-V implementations
 *   that do not include a standard CLINT or do add to the base set of RISC-V
 *   registers.
 *
 * CARE MUST BE TAKEN TO INCLDUE THE CORRECT
 * freertos_risc_v_chip_specific_extensions.h HEADER FILE FOR THE CHIP
 * IN USE.  To include the correct freertos_risc_v_chip_specific_extensions.h
 * header file ensure the path to the correct header file is in the assembler's
 * include path.
 *
 * This freertos_risc_v_chip_specific_extensions.h is for use on RISC-V chips
 * that include a standard CLINT and do not add to the base set of RISC-V
 * registers.
 *
 */
#include "freertos_risc_v_chip_specific_extensions.h"

/* Check the freertos_risc_v_chip_specific_extensions.h and/or command line
definitions. */
#ifndef portasmHAS_CLINT
	#error freertos_risc_v_chip_specific_extensions.h must define portasmHAS_CLINT to either 1 (CLINT present) or 0 (clint not present).
#endif

/* Only the standard core registers are stored by default.  Any additional
registers must be saved by the portasmSAVE_ADDITIONAL_REGISTERS and
portasmRESTORE_ADDITIONAL_REGISTERS macros - which can be defined in a chip
specific version of freertos_risc_v_chip_specific_extensions.h.  See the notes
at the top of this file. */
#define portCONTEXT_SIZE ( 30 * portWORD_SIZE )

.global xPortStartFirstTask
.global freertos_risc_v_trap_handler
.global pxPortInitialiseStack
.extern pxCurrentTCB
.extern ulPortTrapHandler
.extern vTaskSwitchContext
.extern Timer_IRQHandler
.extern pullMachineTimerCompareRegister
.extern pullNextTime
.extern ullTimerIncrementsForOneTick
.extern xISRStackTop
.extern pulMaxID
.extern pullVectorTable

/*-----------------------------------------------------------*/

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func
freertos_risc_v_trap_handler:
	addi sp, sp, -portCONTEXT_SIZE
	sd x1, 1 * portWORD_SIZE( sp )
	sd x5, 2 * portWORD_SIZE( sp )
	sd x6, 3 * portWORD_SIZE( sp )
	sd x7, 4 * portWORD_SIZE( sp )
	sd x8, 5 * portWORD_SIZE( sp )
	sd x9, 6 * portWORD_SIZE( sp )
	sd x10, 7 * portWORD_SIZE( sp )
	sd x11, 8 * portWORD_SIZE( sp )
	sd x12, 9 * portWORD_SIZE( sp )
	sd x13, 10 * portWORD_SIZE( sp )
	sd x14, 11 * portWORD_SIZE( sp )
	sd x15, 12 * portWORD_SIZE( sp )
	sd x16, 13 * portWORD_SIZE( sp )
	sd x17, 14 * portWORD_SIZE( sp )
	sd x18, 15 * portWORD_SIZE( sp )
	sd x19, 16 * portWORD_SIZE( sp )
	sd x20, 17 * portWORD_SIZE( sp )
	sd x21, 18 * portWORD_SIZE( sp )
	sd x22, 19 * portWORD_SIZE( sp )
	sd x23, 20 * portWORD_SIZE( sp )
	sd x24, 21 * portWORD_SIZE( sp )
	sd x25, 22 * portWORD_SIZE( sp )
	sd x26, 23 * portWORD_SIZE( sp )
	sd x27, 24 * portWORD_SIZE( sp )
	sd x28, 25 * portWORD_SIZE( sp )
	sd x29, 26 * portWORD_SIZE( sp )
	sd x30, 27 * portWORD_SIZE( sp )
	sd x31, 28 * portWORD_SIZE( sp )

	csrr t0, mstatus					/* Required for MPIE bit. */
	sd t0, 29 * portWORD_SIZE( sp )

	portasmSAVE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to save any registers unique to the RISC-V implementation. */

	ld  t0, pxCurrentTCB				/* Load pxCurrentTCB. */
	sd  sp, 0( t0 )				 		/* Write sp to first TCB member. */

	csrr a0, mcause
	csrr a1, mepc

test_if_asynchronous:
	srli a2, a0, 0x3f					/* MSB of mcause is 1 if handing an asynchronous interrupt - shift to LSB to clear other bits. */
	beq a2, x0, handle_synchronous		/* Branch past interrupt handing if not asynchronous. */
	sd a1, 0( sp )						/* Asynch so save unmodified exception return address. */

handle_asynchronous:

#if( portasmHAS_CLINT != 0 )

	test_if_mtimer:							/* If there is a CLINT then the mtimer is used to generate the tick interrupt. */
		lui t0, 0x80000                     /* set bit 31 */
		slli t0, t0, 0x20                   /* shift bit 31 to bit 63 */
		addi t1, t0, 7						/* 0x8000000000000007 == machine timer interrupt. */
		bne a0, t1, test_if_external_interrupt

		ld t0, pullMachineTimerCompareRegister	/* Load address of compare register into t0. */
		ld t1, pullNextTime						/* Load the address of ullNextTime into t1. */
		ld t2, 0(t1)							/* Load ullNextTime into t2. */
		sd t2, 0(t0)							/* Store ullNextTime into compare register. */
		ld t0, ullTimerIncrementsForOneTick		/* Load the value of ullTimerIncrementForOneTick into t0 (could this be optimized by storing in an array next to pullNextTime?). */
		add t4, t0, t2							/* Add ullNextTime to the timer increments for one tick */
		sd t4, 0(t1)							/* Store new ullNextTime. */
		ld sp, xISRStackTop						/* Switch to ISR stack before function call. */
		jal xTaskIncrementTick
		beqz a0, processed_source				/* Don't switch context if incrementing tick didn't unblock a task. */
		jal vTaskSwitchContext
		j processed_source

	test_if_external_interrupt:			/* If there is a CLINT and the mtimer interrupt is not pending then check to see if an external interrupt is pending. */
		addi t1, t1, 4					/* 0x8000000000000007 + 4 = 0x800000000000000b == Machine external interrupt. */
		bne a0, t1, as_yet_unhandled	/* Something as yet unhandled. */

#endif /* portasmHAS_CLINT */

handle_external_interupt:
	ld s0, pulMaxID						/* Load the address of ulMaxID into s0. */
	ld t0, pullVectorTable				/* Load base address of vector table into t0. */
	ld sp, xISRStackTop					/* Switch to ISR stack before function call. */
	lwu s1, 0(s0)						/* Load ulMaxID into s1 to claim the interrupt */
	beqz s1, processed_source       	/* If ulMaxID is zero, the interrupt has already been claimed (unlikely) */
	slli t1, s1, 3						/* Shift ulMaxID to 8-byte aligned index into vector table in t1 */
	add t0, t0, t1 						/* Add 8-byte aligned index to base address of vector table in t0 */
	ld t1, 0(t0)						/* load vector from vector table into t1 */
	jalr t1								/* jump to interrupt vector */
	sw s1, 0(s0)						/* write ulMaxID to indicate the interrupt has been serviced */
	j processed_source

handle_synchronous:
	addi a1, a1, 4						/* Synchronous so updated exception return address to the instruction after the instruction that generated the exeption. */
	sd a1, 0( sp )						/* Save updated exception return address. */

test_if_environment_call:
	li t0, 11 							/* 11 == environment call. */
	bne a0, t0, is_exception			/* Not an M environment call, so some other exception. */
	ld sp, xISRStackTop					/* Switch to ISR stack before function call. */
	jal vTaskSwitchContext
	j processed_source

is_exception:
	ebreak
	j is_exception

as_yet_unhandled:
	ebreak
	j as_yet_unhandled

processed_source:
	ld  sp, pxCurrentTCB				/* Load pxCurrentTCB. */
	ld  sp, 0( sp )				 		/* Read sp from first TCB member. */


	ld t0, 0( sp ) 						/* Load mepc with the address of the next instruction in the task to run next. */
	csrw mepc, t0

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */

	/* Load mstatus with the interrupt enable bits used by the task. */
	ld  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0					/* Required for MPIE bit. */

	ld  x1, 1 * portWORD_SIZE( sp )
	ld  x5, 2 * portWORD_SIZE( sp )		/* t0 */
	ld  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	ld  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	ld  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	ld  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	ld  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	ld  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	ld  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	ld  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	ld  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	ld  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	ld  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	ld  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	ld  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	ld  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	ld  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	ld  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	ld  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	ld  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	ld  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	ld  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	ld  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	ld  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	ld  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	ld  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	ld  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	ld  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	mret /*  mstatus.mpp => privilege mode, mstatus.mpie -> mstatus.mie, mepc => pc */
	.endfunc
/*-----------------------------------------------------------*/

.align 8
.func
xPortStartFirstTask:

#if( portasmHAS_CLINT != 0 )
	/* If there is a clint then interrupts can branch directly to the FreeRTOS
	trap handler.  Otherwise the interrupt controller will need to be configured
	outside of this file. */
	la t0, freertos_risc_v_trap_handler
	csrw mtvec, t0
#endif /* portasmHAS_CLILNT */

	ld  sp, pxCurrentTCB				/* Load pxCurrentTCB. */
	ld  sp, 0( sp )				 		/* Read sp from first TCB member. */

	ld  x1, 0( sp )						/* Load mepc with the address of the next instruction in the task to run next. */
	csrw mepc, x1

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */

    ld  t0, 29 * portWORD_SIZE( sp )    /* mstatus */
    csrrw  x0, mstatus, t0              /* Load mstatus with the interrupt enable bits used by the task: mret will restore them */

	ld  x5, 2 * portWORD_SIZE( sp )		/* t0 */
	ld  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	ld  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	ld  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	ld  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	ld  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	ld  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	ld  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	ld  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	ld  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	ld  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	ld  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	ld  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	ld  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	ld  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	ld  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	ld  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	ld  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	ld  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	ld  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	ld  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	ld  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	ld  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	ld  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	ld  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	ld  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	ld  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi	sp, sp, portCONTEXT_SIZE
	mret /*  mstatus.mpp => privilege mode, mstatus.mpie -> mstatus.mie, mepc => pc */
	.endfunc
/*-----------------------------------------------------------*/

/*
 * Unlike other ports pxPortInitialiseStack() is written in assembly code as it
 * needs access to the portasmADDITIONAL_CONTEXT_SIZE constant.  The prototype
 * for the function is as per the other ports:
 * StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
 *
 * As per the standard RISC-V ABI pxTopOfStack is passed in in a0, pxCode in
 * a1, and pvParameters in a2.  The new top of stack is passed out in a0.
 *
 * RISC-V maps registers to ABI names as follows (X1 to X31 integer registers
 * for the 'I' profile, X1 to X15 for the 'E' profile, currently I assumed).
 *
 * Register		ABI Name	Description						Saver
 * x0			zero		Hard-wired zero					-
 * x1			ra			Return address					Caller
 * x2			sp			Stack pointer					Callee
 * x3			gp			Global pointer					-
 * x4			tp			Thread pointer					-
 * x5-7			t0-2		Temporaries						Caller
 * x8			s0/fp		Saved register/Frame pointer	Callee
 * x9			s1			Saved register					Callee
 * x10-11		a0-1		Function Arguments/return values Caller
 * x12-17		a2-7		Function arguments				Caller
 * x18-27		s2-11		Saved registers					Callee
 * x28-31		t3-6		Temporaries						Caller
 *
 * The RISC-V context is saved t FreeRTOS tasks in the following stack frame,
 * where the global and thread pointers are currently assumed to be constant so
 * are not saved:
 *
 * mstatus
 * x31
 * x30
 * x29
 * x28
 * x27
 * x26
 * x25
 * x24
 * x23
 * x22
 * x21
 * x20
 * x19
 * x18
 * x17
 * x16
 * x15
 * x14
 * x13
 * x12
 * x11
 * pvParameters
 * x9
 * x8
 * x7
 * x6
 * x5
 * portTASK_RETURN_ADDRESS
 * [chip specific registers go here]
 * pxCode
 */
.align 8
.func
pxPortInitialiseStack:

	csrr t0, mstatus					/* Obtain current mstatus value. */
	addi t1, x0, 0x188					/* Generate the value 0x1880, which are the MPIE and MPP bits to set in mstatus. */
	slli t1, t1, 4
	or t0, t0, t1						/* Set MPIE and MPP bits in mstatus value. */

	addi a0, a0, -portWORD_SIZE
	sd t0, 0(a0)						/* mstatus onto the stack. */
	addi a0, a0, -(22 * portWORD_SIZE)	/* Space for registers x11-x31. */
	sd a2, 0(a0)						/* Task parameters (pvParameters parameter) goes into register X10/a0 on the stack. */
	addi a0, a0, -(6 * portWORD_SIZE)	/* Space for registers x5-x9. */
	sd x0, 0(a0)						/* Return address onto the stack, could be portTASK_RETURN_ADDRESS */
	addi t0, x0, portasmADDITIONAL_CONTEXT_SIZE /* The number of chip specific additional registers. */
chip_specific_stack_frame:				/* First add any chip specific registers to the stack frame being created. */
	beq t0, x0, 1f						/* No more chip specific registers to save. */
	addi a0, a0, -portWORD_SIZE			/* Make space for chip specific register. */
	sd x0, 0(a0)						/* Give the chip specific register an initial value of zero. */
	addi t0, t0, -1						/* Decrement the count of chip specific registers remaining. */
	j chip_specific_stack_frame			/* Until no more chip specific registers. */
1:
	addi a0, a0, -portWORD_SIZE
	sd a1, 0(a0)						/* mret value (pxCode parameter) onto the stack. */
	ret
	.endfunc
/*-----------------------------------------------------------*/
