/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#=========================================================================
# Reset handler code for Service Processor core
#=========================================================================

.global bl1_entry

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
