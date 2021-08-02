#ifndef TEST_TRACE_H
#define TEST_TRACE_H

#include <stdlib.h>

#include "device_trace.h"

struct trace_buffer_std_header_t *test_trace_create(struct trace_control_block_t *cb, size_t size)
{
    struct trace_buffer_std_header_t *buf = malloc(size);
    if (!buf) {
        return NULL;
    }

    struct trace_init_info_t info;
    info.shire_mask = 0xFFFFFFFF;
    info.thread_mask = 0XFFFFFFFF;
    info.event_mask = TRACE_EVENT_ENABLE_ALL;
    info.filter_mask = TRACE_FILTER_ENABLE_ALL;
    info.threshold = size;

    cb->size_per_hart = size;
    cb->base_per_hart = (uint64_t)buf;
    Trace_Init(&info, cb, TRACE_STD_HEADER);

    buf->magic_header = TRACE_MAGIC_HEADER;
    buf->data_size = sizeof(struct trace_buffer_std_header_t);
#ifdef MASTER_MINION
    buf->type = TRACE_MM_BUFFER;
#else
    buf->type = TRACE_CM_BUFFER;
#endif

    return buf;
}

void test_trace_evict(struct trace_buffer_std_header_t *buf, struct trace_control_block_t *cb)
{
    buf->data_size = cb->offset_per_hart;
}

void test_trace_destroy(struct trace_buffer_std_header_t *buf)
{
    free(buf);
}

#endif
