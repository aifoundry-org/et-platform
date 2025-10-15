/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "softfloat/softfloat.h"

float32_t f32_copySign( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    uiZ = packToF32UI( signF32UI( uiB ), expF32UI( uiA ), fracF32UI( uiA ) );
    uZ.ui = uiZ;
    return uZ.f;
}
