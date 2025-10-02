/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

.global memory_copy_8x32

# void memory_copy_8x32(uint32_t * dst, uint32_t * src, uint32_t count)
# dst - 32-bit aligned destination address
# src - 32-bit aligned source address
# src_end - 32-bit aligned source end address.  Copy size must be a multiple of 8, so (src_end - src) % 8 must be 0.

memory_copy_8x32:
    # read 8x32-bit values
    lw t0, 0(a1)
    lw t1, 4(a1)
    lw t2, 8(a1)
    lw t3, 12(a1)
    lw t4, 16(a1)
    lw t5, 20(a1)
    lw t6, 24(a1)
    lw a3, 28(a1)
    
    # write 8x32-bit values
    sw t0, 0(a0)
    sw t1, 4(a0)
    sw t2, 8(a0)
    sw t3, 12(a0)
    sw t4, 16(a0)
    sw t5, 20(a0)
    sw t6, 24(a0)
    sw a3, 28(a0)
    
    # increment src and dst address
    addi a0, a0, 32
    addi a1, a1, 32
    
    # repeat if src < src_end
    bltu a1, a2, memory_copy_8x32
    
    # return
    ret
