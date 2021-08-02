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
 ***********************************************************************/

#ifndef DEVICE_TRACE_DECODE_H
#define DEVICE_TRACE_DECODE_H

#include "device_trace_types.h"

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Trace_Decode
 *
 *   DESCRIPTION
 *
 *       This function decodes a trace buffer one entry at a time.
 *
 *   INPUTS
 *
 *       tb     Pointer to the trace buffer header.
 *       prev   Pointer to the last entry, or
 *              NULL if this is the first time this function is called.
 *
 *   OUTPUTS
 *
 *       void*  Pointer to the next entry in the trace buffer, or
 *              NULL if the end of the trace buffer has been reached.
 *              Note: This is a void* to support both trace_entry_ext_t
 *              and trace_entry_t types depending on the trace type.
 *
 ***********************************************************************/
void* Trace_Decode(struct trace_buffer_std_header_t* tb, void* prev);

/*
 * Implement Trace_Decode by defining DEVICE_TRACE_DECODE_IMPL
 * in a *single* source file before including the header, i.e.:
 *
 * #define DEVICE_TRACE_DECODE_IMPL
 * #include <device_trace_decode.h>
 */

#ifdef DEVICE_TRACE_DECODE_IMPL

void* Trace_Decode(struct trace_buffer_std_header_t* tb, void* prev)
{
    if (tb == NULL) return NULL;

    const size_t buffer_size = tb->data_size;

    if (prev == NULL)
    {
        /* Check if valid trace buffer */
        if (tb->magic_header != TRACE_MAGIC_HEADER) return NULL;
        if (buffer_size <= sizeof(struct trace_buffer_std_header_t)) return NULL;
        /* First entry */
        return tb + 1;
    }

    const uint16_t entry_type = ((struct trace_entry_header_t*)prev)->type;
    size_t payload_size;

#define DEVICE_TRACE_PAYLOAD_SIZE(E, S)  \
    case TRACE_TYPE_ ## E:               \
        payload_size = S;                \
        break;

    switch (entry_type) {
    DEVICE_TRACE_PAYLOAD_SIZE(VALUE_U8, sizeof(struct trace_value_u8_t))
    DEVICE_TRACE_PAYLOAD_SIZE(VALUE_U16, sizeof(struct trace_value_u16_t))
    DEVICE_TRACE_PAYLOAD_SIZE(VALUE_U32, sizeof(struct trace_value_u32_t))
    DEVICE_TRACE_PAYLOAD_SIZE(VALUE_U64, sizeof(struct trace_value_u64_t))
    DEVICE_TRACE_PAYLOAD_SIZE(VALUE_FLOAT, sizeof(struct trace_value_float_t))
    DEVICE_TRACE_PAYLOAD_SIZE(STRING, sizeof(struct trace_string_t))
    DEVICE_TRACE_PAYLOAD_SIZE(PMC_COUNTER, sizeof(struct trace_pmc_counter_t))
    DEVICE_TRACE_PAYLOAD_SIZE(PMC_ALL_COUNTERS, sizeof(struct trace_pmc_counter_t) * 7)
    case TRACE_TYPE_MEMORY: {
        payload_size = sizeof(struct trace_entry_header_t)
                     + sizeof(uint64_t) /* src_addr */
                     + sizeof(uint64_t) /* size */
                     + ((struct trace_memory_t*)prev)->size;
        break;
    }
    DEVICE_TRACE_PAYLOAD_SIZE(EXCEPTION, 0)
    DEVICE_TRACE_PAYLOAD_SIZE(CMD_STATUS, 0)
    default:
        payload_size = 0;
    }

#undef DEVICE_TRACE_PAYLOAD_SIZE

    if (payload_size == 0) return NULL;

    void* next = (char*)prev + payload_size;

    /* End of buffer? */
    const size_t cur_size = (char*)next - (char*)tb;
    if (cur_size >= buffer_size) return NULL;

    return next;
}

#endif /* DEVICE_TRACE_DECODE_IMPL */

#endif /* DEVICE_TRACE_TYPES_H */
