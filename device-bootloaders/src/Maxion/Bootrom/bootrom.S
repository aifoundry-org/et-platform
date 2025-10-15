//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

//========================================================================
// cosim bootrom
//========================================================================
// BootROM code used for the purposes of cosimulation.  Currently, the RTL
// implementation includes a cosimulation configuration which sets the
// processor in debug mode.  The bootrom code essentially sets the debug
// pc and the debug csr register values accordingly to reset the processor
// to the start of the test programs.

        .section .text.init
        .option  norvc
        .globl   _start

_start: csrr     a0, mhartid
        beqz     a0, 1f           // We only allow hart0 to proceed

0:      wfi
        j        0b

1:      la       a1, _start + 256 // DTB is at boot start + 256

        li       s0, 0x603
        csrw     dcsr, s0

        li       s0, 0x8000000000 // Start kernel and exit debug
        csrw     dpc, s0

        
        li t0, 0xDEADBEEF          // Write magic for for SP to check
        li t1, 0x0030000000
        sw t0, 0(t1)
        j 0b
        dret
