/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file trace_primitives.c
    \brief A C file that has generic implemenation of Trace memory
           access primitives. All memory operations are normal operations
           without any special access primitive.
*/
/***********************************************************************/

#include "bl2_timer.h"

#define ET_TRACE_GET_TIMESTAMP() timer_get_ticks_count()

#define ET_TRACE_ENCODER_IMPL
#include <et-trace/encoder.h>

