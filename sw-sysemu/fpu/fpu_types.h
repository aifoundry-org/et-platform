/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _FPU_TYPES_H
#define _FPU_TYPES_H

#include "softfloat/platform.h"
#include "softfloat/softfloat_types.h"

// Arithmetic types
typedef struct { uint16_t v; } float11_t;
typedef struct { uint16_t v; } float10_t;

#define isSubnormalF11UI( a ) ((((a) & 0x7C0) == 0) && ((a) & 0x03F))
#define softfloat_zeroExpSigF11UI( a ) (0)

#define isSubnormalF10UI( a ) ((((a) & 0x3E0) == 0) && ((a) & 0x01F))
#define softfloat_zeroExpSigF10UI( a ) (0)

#endif // _FPU_TYPES_H
