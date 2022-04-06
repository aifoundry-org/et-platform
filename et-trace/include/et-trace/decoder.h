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
 *       This function decodes a trace buffer (with trace standard header)
 *       one entry at a time. If buffer has has sub-buffer partitions,
 *       this function decode those sub-buffers as well.
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

static inline bool check_trace_layout_version(const struct trace_version_t *buf_version)
{
    /* Check if Trace layout version of given buffer is supported by decoder or not. */
    return (buf_version->major == TRACE_VERSION_MAJOR) &&
           (buf_version->minor == TRACE_VERSION_MINOR) &&
           (buf_version->patch == TRACE_VERSION_PATCH);
}

static inline const struct trace_entry_header_t *get_next_trace_event(const struct trace_entry_header_t *packet)
{
    size_t payload_size = (sizeof(struct trace_entry_header_t) + packet->payload_size);

    if (payload_size == 0)
        return NULL;

    return ((struct trace_entry_header_t *)((const uint8_t *)packet + payload_size));
}

/*! \fn decode_next_valid_sub_buffer
    \brief This function traverse sub-buffer after given buffer index.
           It return first entry if of first sub-buffer which has valid data.
           If no processding sub-buffers has valid data, then it returns NULL.
*/
static inline const struct trace_entry_header_t *decode_next_valid_sub_buffer(
    const struct trace_buffer_std_header_t *tb,
    uint16_t buf_index)
{
    struct trace_buffer_size_header_t *base = NULL;
    struct trace_entry_header_t *next = NULL;
    /* */
    for (size_t i = (buf_index + 1); i < tb->sub_buffer_count; i++)
    {
        /* Get the base of Nth sub buffer */
        base = (struct trace_buffer_size_header_t *)((uint64_t)tb +
                (uint64_t)(i * tb->sub_buffer_size));

        /* Check if this buffer has any trace data in it. */
        if (base->data_size > sizeof(struct trace_buffer_size_header_t))
        {
            /* Found data in this sub-buffer, return first packet of this sub-buffer. */
            next = (struct trace_entry_header_t *)(base+1);
            break;
        }
    }

    return next;
}

const struct trace_entry_header_t *Trace_Decode(const struct trace_buffer_std_header_t *tb,
                const struct trace_entry_header_t *prev)
{
    if (tb == NULL)
        return NULL;

    size_t payload_size = tb->data_size;
    uint64_t base_addr = (uint64_t) tb;
    uint16_t buf_index = 0;

    if (prev == NULL) {
        /* Check if valid trace buffer */
        if ((tb->magic_header != TRACE_MAGIC_HEADER) || !(check_trace_layout_version(&tb->version)))
        {
            return NULL;
        }
        else if (payload_size > sizeof(struct trace_buffer_std_header_t))
        {
            /* First entry */
            return (struct trace_entry_header_t *)(tb + 1);
        }
        else if (tb->sub_buffer_count > 1)
        {
            /* First buffer do not have any data, check data in next sub-buffers */
            return decode_next_valid_sub_buffer(tb, buf_index);
        }
        else
        {
            return NULL;
        }
    }
    else if (prev < (const void *)tb)
    {
        /* Invalid prev entry */
        return NULL;
    }
    else if (tb->sub_buffer_count > 1)
    {
        buf_index = ((uint64_t)prev - (uint64_t)tb)  / tb->sub_buffer_size;
        /* Previous event node lies in sub buffer? */
        if ((buf_index > 0) && (buf_index < tb->sub_buffer_count))
        {
            struct trace_buffer_size_header_t *base = (struct trace_buffer_size_header_t *)((uint64_t)tb +
                        (uint64_t)(buf_index * tb->sub_buffer_size));
            payload_size = base->data_size;
            base_addr = (uint64_t) base;
        }
    }

    /* Get next trace event. */
    const struct trace_entry_header_t *next = get_next_trace_event(prev);

    /* End of buffer current buffer? */
    const size_t cur_size = (uint64_t)next - base_addr;
    if (cur_size >= payload_size)
    {
        if (tb->sub_buffer_count > 1)
        {
            /* There is no more data in current buffer, get next sub-buffer which has data. */
            next = decode_next_valid_sub_buffer(tb, buf_index);
        }
        else
        {
            return NULL;
        }
    }

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
