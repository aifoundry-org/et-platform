/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */

#include "ring_buffer.h"
#include "esperanto/device-api/tracing_types_non_privileged.h"
#include "layout.h"

#include <string.h>

extern const size_t event_size_array[];

// Returns the remaining space in ring buffer
static size_t ring_buffer_avail_space(struct buffer_header_t *rbuffer,
                                      size_t buffer_size)
{
    return (rbuffer->head < rbuffer->tail) ?
               rbuffer->tail - rbuffer->head - 1 :
               buffer_size - rbuffer->head + rbuffer->tail - 1;
}

// Ring buffer utility functions
static trace_status_e ring_buffer_check_space(struct buffer_header_t *rbuffer,
                                              size_t buffer_size,
                                              size_t event_size)
{
    size_t rem = ring_buffer_avail_space(rbuffer, buffer_size);
    if (rem < event_size) {
        return TRACE_STATUS_BUFFER_FULL;
    }

    return TRACE_STATUS_SUCCESS;
}

// Allocate space for event
void *ring_buffer_alloc_space(uint16_t hart_id, size_t size)
{
    void *ret;
    struct trace_control_t *cntrl = (void *)DEVICE_MRT_TRACE_BASE;
    struct buffer_header_t *rbuffer_header =
        DEVICE_MRT_BUFFER_BASE(hart_id, cntrl->buffer_size);
    trace_status_e status = ring_buffer_check_space(
        rbuffer_header, DEVICE_MRT_BUFFER_LENGTH(cntrl->buffer_size), size);

    // Check if we have space
    if (TRACE_STATUS_SUCCESS != status) {
        // Current implementation:
        // On overflow, fill remaining space with zeros. avoiding event break.
        // Start writing from start of the buffer after tail adjustments.
        size_t crnt_event_size;
        struct message_header_t *msg_hdr;
        size_t avail_size = ring_buffer_avail_space(
            rbuffer_header, DEVICE_MRT_BUFFER_LENGTH(cntrl->buffer_size));

        // head needs to overflow, and available size is not enough
        if (rbuffer_header->head > rbuffer_header->tail) {
            // +1 to also include the unused byte to differentiate
            // between full and empty condition
            memset((void *)(rbuffer_header->buffer + rbuffer_header->head),
                   TRACE_EVENT_ID_NONE, avail_size + 1);

            // wrap head pointer
            rbuffer_header->head = 0;
            avail_size = 0;
        }

        // Check if maximum creatable space is enough or not?,
        // account for consecutive overflow
        if (size > (DEVICE_MRT_BUFFER_LENGTH(cntrl->buffer_size) -
                    rbuffer_header->tail + avail_size)) {
            // Okay its not enough, head and tail both needs to overflow,
            // Mark the remaining space as invalid
            memset((void *)(rbuffer_header->buffer + rbuffer_header->head),
                   TRACE_EVENT_ID_NONE,
                   (DEVICE_MRT_BUFFER_LENGTH(cntrl->buffer_size) -
                    rbuffer_header->tail + avail_size + 1));

            rbuffer_header->head = 0;
            rbuffer_header->tail = 0;
            avail_size = 0;
        }

        // Compute the size to advance the tail pointer in the ring buffer
        while (avail_size <= size) {
            msg_hdr = ((void *)(rbuffer_header->buffer + rbuffer_header->tail));

            // Retrieve the event size from the message header
            crnt_event_size = event_size_array[msg_hdr->event_id];
            if (msg_hdr->event_id == TRACE_EVENT_ID_TEXT_STRING) {
                crnt_event_size += ((struct trace_string_t *)msg_hdr)->size;
                crnt_event_size = ALIGN(crnt_event_size, 8UL);
            } else if (msg_hdr->event_id == TRACE_EVENT_ID_NONE) {
                // Reached the unused space from previous overflow. As tested
                // there is enough space creatable, just wrap tail and break
                rbuffer_header->tail = 0;
                break;
            }

            // Adjust tail to point to oldest event in buffer
            // freeing space for new events
            rbuffer_header->tail = (rbuffer_header->tail + crnt_event_size) %
                                   DEVICE_MRT_BUFFER_LENGTH(cntrl->buffer_size);

            avail_size += crnt_event_size;
        }
    }

    // Get the address to return
    ret = (void *)(rbuffer_header->buffer + rbuffer_header->head);

    // Move the head pointer forward
    rbuffer_header->head += size;

    return ret;
}
