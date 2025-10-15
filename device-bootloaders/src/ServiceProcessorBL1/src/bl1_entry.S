/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#=========================================================================
# Reset handler code for Service Processor core
#=========================================================================

.global bl1_entry

.align 3
bl1_entry:
    # initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    # initialize trap vector
    la    t0, mtrap_vector
    csrw  mtvec, t0

    la a1, _init_handler
    jalr x0, a1
