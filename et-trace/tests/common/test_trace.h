#ifndef TEST_TRACE_H
#define TEST_TRACE_H

#include <stdlib.h>

#include <et-trace/layout.h>

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
    cb->offset_per_hart = sizeof(struct trace_buffer_std_header_t);
    cb->buffer_lock_acquire = NULL;
    cb->buffer_lock_release = NULL;
    Trace_Init(&info, cb, TRACE_STD_HEADER);

    buf->magic_header = TRACE_MAGIC_HEADER;
    buf->data_size = sizeof(struct trace_buffer_std_header_t);
    buf->sub_buffer_count = 1;
    buf->sub_buffer_size = size;
    buf->version.major = TRACE_VERSION_MAJOR;
    buf->version.minor = TRACE_VERSION_MINOR;
    buf->version.patch = TRACE_VERSION_PATCH;
#ifdef MASTER_MINION
    buf->type = TRACE_MM_BUFFER;
#else
    buf->type = TRACE_CM_BUFFER;
#endif

    return buf;
}

struct trace_buffer_std_header_t *test_cm_trace_create(struct trace_control_block_t *cb,
                                                       size_t size, size_t sub_buffer_count)
{
    struct trace_buffer_std_header_t *buf = malloc(size * sub_buffer_count);
    if (!buf) {
        return NULL;
    }
    struct trace_init_info_t info;
    info.shire_mask = 0xFFFFFFFF;
    info.thread_mask = 0XFFFFFFFF;
    info.event_mask = TRACE_EVENT_ENABLE_ALL;
    info.filter_mask = TRACE_FILTER_ENABLE_ALL;
    info.threshold = size;

    for (uint32_t i = 0; i < sub_buffer_count; i++) {
        cb->size_per_hart = size;
        cb->base_per_hart = (uint64_t)buf + (i * size);
        cb->offset_per_hart = sizeof(struct trace_buffer_std_header_t);
        cb->buffer_lock_acquire = NULL;
        cb->buffer_lock_release = NULL;
        if (i == 0) {
            Trace_Init(&info, cb, TRACE_STD_HEADER);
            buf->data_size = sizeof(struct trace_buffer_std_header_t);
        } else {
            Trace_Init(&info, cb, TRACE_SIZE_HEADER);
            buf->data_size = sizeof(struct trace_buffer_size_header_t);
        }
        cb++;
    }

    buf->magic_header = TRACE_MAGIC_HEADER;
    buf->sub_buffer_count = sub_buffer_count;
    buf->sub_buffer_size = size;
    buf->version.major = TRACE_VERSION_MAJOR;
    buf->version.minor = TRACE_VERSION_MINOR;
    buf->version.patch = TRACE_VERSION_PATCH;
    buf->type = TRACE_CM_BUFFER;

    return buf;
}

void test_cm_trace_evict(struct trace_control_block_t *cb)
{
    if (cb->header == TRACE_STD_HEADER) {
        struct trace_buffer_std_header_t *std_header_ptr =
            (struct trace_buffer_std_header_t *)cb->base_per_hart;
        std_header_ptr->data_size = cb->offset_per_hart;
    } else {
        struct trace_buffer_size_header_t *size_header_ptr =
            (struct trace_buffer_size_header_t *)cb->base_per_hart;
        size_header_ptr->data_size = cb->offset_per_hart;
    }
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
