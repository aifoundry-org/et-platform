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

#include <stdint.h>
static inline uint64_t et_trace_get_timestamp(void);

/* TODO: Provide timer API for SP to get timestamp. */
#define ET_TRACE_GET_TIMESTAMP() et_trace_get_timestamp()

#define ET_TRACE_ENCODER_IMPL
#include <et-trace/encoder.h>

/* Increment a static variable */
static inline uint64_t et_trace_get_timestamp()
{
    static uint64_t time = 0;
    return ++time;
}
