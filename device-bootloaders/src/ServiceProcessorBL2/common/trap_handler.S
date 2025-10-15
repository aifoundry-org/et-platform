/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/
.extern freertos_risc_v_trap_handler

    .text

    .align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */

    .global mtrap_vector
mtrap_vector:
    // Jump directly to FreeRTOS trap handler
    j freertos_risc_v_trap_handler
