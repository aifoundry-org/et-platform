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

/***********************************************************************
 * et-trace/decoder.h
 * Decode interface for Esperanto device traces.
 *
 *
 * USAGE
 *
 * In a *single* source file, put:
 *
 *     #define ET_TRACE_DECODER_IMPL
 *     #include <et-trace/decoder.h>
 *
 * Other source files can include et-trace/decoder.h as normal.
 *
 *
 * ABOUT
 *
 * This file provides an interface to decoder device traces
 * stored in a linear memory buffer. The trace buffer should
 * be well formatted, according to the layout in et-trace/layout.h.
 *
 * The main way to access the contents of a given trace buffer
 * is by iterating over its entries:
 *
 *     struct trace_buffer_std_header_t* trace_buf = ...;
 *     struct trace_entry_header_t* entry = NULL;
 *     while ((entry = Trace_Decode(trace_buf, entry))) {
 *        .. process entry here ..
 *     }
 *
 ***********************************************************************/

#ifndef ET_TRACE_DECODER_H
#define ET_TRACE_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

struct trace_buffer_std_header_t;

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Trace_Decode
 *
 *   DESCRIPTION
 *
 *       This function decodes a trace sub buffer (with trace standard header)
 *       one entry at a time.
 *
 *   INPUTS
 *
 *       tb     Pointer to the trace buffer standard header.
 *       prev   Pointer to the last entry, or
 *              NULL if this is the first time this function is called.
 *
 *   OUTPUTS
 *
 *       void*  Pointer to the next entry in the trace buffer, or
 *              NULL if the end of the trace buffer has been reached.
 *              Note: This is a void* to support both trace_entry_ext_t
 *              and trace_entry_t types depending on the trace type.
 *              On decoder errors or wrong inputs, the function returns NULL.
 *
 ***********************************************************************/
const struct trace_entry_header_t *Trace_Decode(const struct trace_buffer_std_header_t *tb,
                const struct trace_entry_header_t *prev);

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Trace_Decode_Sub
 *
 *   DESCRIPTION
 *
 *       This function decodes a trace sub buffer (with trace size header)
 *       one entry at a time.
 *
 *   INPUTS
 *
 *       tb     Pointer to the trace buffer size header.
 *       prev   Pointer to the last entry, or
 *              NULL if this is the first time this function is called.
 *
 *   OUTPUTS
 *
 *       void*  Pointer to the next entry in the trace buffer, or
 *              NULL if the end of the trace buffer has been reached.
 *              Note: This is a void* to support both trace_entry_ext_t
 *              and trace_entry_t types depending on the trace type.
 *              On decoder errors or wrong inputs, the function returns NULL.
 *
 ***********************************************************************/
const struct trace_entry_header_t *Trace_Decode_Sub(const struct trace_buffer_size_header_t *tb,
                const struct trace_entry_header_t *prev);

#ifdef ET_TRACE_DECODER_IMPL

#include "layout.h"

#include <stdlib.h>

static inline const struct trace_entry_header_t *get_next_trace_event(const struct trace_entry_header_t *packet)
{
    size_t payload_size = 0;
#define ET_TRACE_PAYLOAD_SIZE(E, S) \
    case TRACE_TYPE_##E:            \
        payload_size = S;           \
        break;

    /* Get payload size depending on entry type */
    switch (packet->type) {
        ET_TRACE_PAYLOAD_SIZE(VALUE_U8, sizeof(struct trace_value_u8_t))
        ET_TRACE_PAYLOAD_SIZE(VALUE_U16, sizeof(struct trace_value_u16_t))
        ET_TRACE_PAYLOAD_SIZE(VALUE_U32, sizeof(struct trace_value_u32_t))
        ET_TRACE_PAYLOAD_SIZE(VALUE_U64, sizeof(struct trace_value_u64_t))
        ET_TRACE_PAYLOAD_SIZE(VALUE_FLOAT, sizeof(struct trace_value_float_t))
        ET_TRACE_PAYLOAD_SIZE(STRING, sizeof(struct trace_string_t))
        ET_TRACE_PAYLOAD_SIZE(PMC_COUNTER, sizeof(struct trace_pmc_counter_t))
        ET_TRACE_PAYLOAD_SIZE(PMC_ALL_COUNTERS, sizeof(struct trace_pmc_counter_t) * 7)
    case TRACE_TYPE_MEMORY: {
        payload_size = sizeof(struct trace_entry_header_t)      /* header*/
                       + sizeof(uint64_t)                       /* src_addr */
                       + sizeof(uint64_t)                       /* size */
                       + ((struct trace_memory_t *)packet)->size; /* data */
        break;
    }
        ET_TRACE_PAYLOAD_SIZE(EXCEPTION, 0)
        ET_TRACE_PAYLOAD_SIZE(CMD_STATUS, sizeof(struct trace_cmd_status_t))
        ET_TRACE_PAYLOAD_SIZE(POWER_STATUS, sizeof(struct trace_power_status_t))
    default:
        return NULL;
    }

#undef ET_TRACE_PAYLOAD_SIZE

    if (payload_size == 0)
        return NULL;

    return ((struct trace_entry_header_t *)((const uint8_t *)packet + payload_size));
}

const struct trace_entry_header_t *Trace_Decode(const struct trace_buffer_std_header_t *tb,
                const struct trace_entry_header_t *prev)
{
    if (tb == NULL)
        return NULL;

    const size_t buffer_size = tb->data_size;

    if (prev == NULL) {
        /* Check if valid trace buffer */
        if (tb->magic_header != TRACE_MAGIC_HEADER)
            return NULL;
        if (buffer_size <= sizeof(struct trace_buffer_std_header_t))
            return NULL;
        /* First entry */
        return (struct trace_entry_header_t *)(tb + 1);
    }

    /* Invalid prev entry */
    if (prev < (const void *)tb)
        return NULL;

    const struct trace_entry_header_t *next = get_next_trace_event(prev);;

    /* End of buffer? */
    const size_t cur_size = (const uint8_t *)next - (const uint8_t *)tb;
    if (cur_size >= buffer_size)
        return NULL;

    return next;
}


const struct trace_entry_header_t *Trace_Decode_Sub(const struct trace_buffer_size_header_t *tb,
                const struct trace_entry_header_t *prev)
{
    if (tb == NULL)
        return NULL;

    const size_t buffer_size = tb->data_size;

    if (prev == NULL) {
        /* Check if valid trace buffer */
        if (buffer_size <= sizeof(struct trace_buffer_size_header_t))
            return NULL;
        /* First entry */
        return (struct trace_entry_header_t *)(tb + 1);
    }

    /* Invalid prev entry */
    if (prev < (const void *)tb)
        return NULL;

    const struct trace_entry_header_t *next = get_next_trace_event(prev);;

    /* End of buffer? */
    const size_t cur_size = (const uint8_t *)next - (const uint8_t *)tb;
    if (cur_size >= buffer_size)
        return NULL;

    return next;
}

#endif /* ET_TRACE_DECODER_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* ET_TRACE_DECODER_H */
