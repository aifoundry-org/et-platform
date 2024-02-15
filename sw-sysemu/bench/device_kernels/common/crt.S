/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the 
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with 
* the written permission of Esperanto Technologies and 
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#=========================================================================
# crt.S : Entry point for RISC-V user programs
#=========================================================================

  .text
  .global _start
_start:
  # Initialize global pointer; don't use relaxed relocation because it will
  # generate a "mv gp, gp" which sets gp to zero.
  .option push
  .option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
  .option pop
  call    main
  wfi

