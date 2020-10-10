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

#ifndef VPU_H
#define VPU_H

//-------------------------------------------------------------------------------------------------
//
// FUNCTION setM0MaskFF
//
//   Set Mask0 to 1 for all lanes in VPU, i.e. enable all lanes
//
static void setM0MaskFF(void);
static inline void setM0MaskFF()
{
    __asm__ __volatile__("mov.m.x m0, zero, 0xff\n" : : : "memory");
}

//-------------------------------------------------------------------------------------------------
//
// FUNCTION setFRegs_2
//
//   Set all FP regs (lanes will be set as enabled by mask) to input param value
//
inline __attribute__((always_inline)) void setFRegs_2(unsigned long value)
{
    __asm__ __volatile__("add x5, zero, %[value]\n"
                         "fbcx.ps f0,x5\n"
                         "fbcx.ps f1,x5\n"
                         "fbcx.ps f2,x5\n"
                         "fbcx.ps f3,x5\n"
                         "fbcx.ps f4,x5\n"
                         "fbcx.ps f5,x5\n"
                         "fbcx.ps f6,x5\n"
                         "fbcx.ps f7,x5\n"
                         "fbcx.ps f8,x5\n"
                         "fbcx.ps f9,x5\n"
                         "fbcx.ps f10,x5\n"
                         "fbcx.ps f11,x5\n"
                         "fbcx.ps f12,x5\n"
                         "fbcx.ps f13,x5\n"
                         "fbcx.ps f14,x5\n"
                         "fbcx.ps f15,x5\n"
                         "fbcx.ps f16,x5\n"
                         "fbcx.ps f17,x5\n"
                         "fbcx.ps f18,x5\n"
                         "fbcx.ps f19,x5\n"
                         "fbcx.ps f20,x5\n"
                         "fbcx.ps f21,x5\n"
                         "fbcx.ps f22,x5\n"
                         "fbcx.ps f23,x5\n"
                         "fbcx.ps f24,x5\n"
                         "fbcx.ps f25,x5\n"
                         "fbcx.ps f26,x5\n"
                         "fbcx.ps f27,x5\n"
                         "fbcx.ps f28,x5\n"
                         "fbcx.ps f29,x5\n"
                         "fbcx.ps f30,x5\n"
                         "fbcx.ps f31,x5\n"
                         :
                         : [value] "r"(value)
                         : "memory");
}

#endif
