/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/softfloat.h"

float32_t f32_mulSub( float32_t a, float32_t b, float32_t c )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    union ui32_f32 uC;
    uint_fast32_t uiC;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    uC.f = c;
    uiC = uC.ui ^ 0x80000000;
    return softfloat_mulAddF32( uiA, uiB, uiC, 0 );
}
