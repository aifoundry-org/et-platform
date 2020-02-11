/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "softfloat/softfloat.h"

float32_t f32_maxNum( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool signA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signB;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if ( isSubnormalF32UI( uiA ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA = softfloat_zeroExpSigF32UI( uiA );
    }
    if ( isSubnormalF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiB = softfloat_zeroExpSigF32UI( uiB );
    }
#endif
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        if (
                softfloat_isSigNaNF32UI( uiA )
             || softfloat_isSigNaNF32UI( uiB )
        ) {
            softfloat_raiseFlags( softfloat_flag_invalid );
            uiZ = defaultNaNF32UI;
            goto uiZ;
        }
        uiZ = isNaNF32UI( uiA )
            ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB ) : uiA;
        goto uiZ;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );
    uiZ = ( signA != signB ) ? ( signB ? uiA : uiB )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? uiA : uiB );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}
