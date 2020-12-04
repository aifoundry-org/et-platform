/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
.extern freertos_risc_v_trap_handler

    .text

    .align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */

    .global mtrap_vector
mtrap_vector:
    // Jump directly to FreeRTOS trap handler
    j freertos_risc_v_trap_handler
