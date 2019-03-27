/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include "fpu_types.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

#ifdef FPU_DEBUG
#undef FPU_DEBUG
#endif

static uint_fast32_t
softfloat_ignoreNaNF32UI( uint_fast32_t uiA, uint_fast32_t uiB )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    return isNaNF32UI( uiA )
        ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB )
        : uiA;
}


static uint_fast32_t
softfloat_ignoreQNaNF32UI( uint_fast32_t uiA, uint_fast32_t uiB )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        return defaultNaNF32UI;
    }
    return isNaNF32UI( uiA )
        ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB )
        : uiA;
}


float32_t f32_minimumNumber( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_ignoreNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signA ? a : b )
        : ( ((uiA != uiB) && (signA ^ (uiA < uiB))) ? a : b );
}


float32_t f32_maximumNumber( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_ignoreNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signB ? a : b )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? a : b );
}


float32_t f32_minNum( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_ignoreQNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signA ? a : b )
        : ( ((uiA != uiB) && (signA ^ (uiA < uiB))) ? a : b );
}


float32_t f32_maxNum( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_ignoreQNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signB ? a : b )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? a : b );
}
