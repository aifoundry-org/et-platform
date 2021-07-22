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
/*! \file device_trace.h
    \brief A C header that implements the Trace services for device side.
*/
/***********************************************************************/

#ifndef DEVICE_TRACE_H
#define DEVICE_TRACE_H

#include <stdbool.h>
#include "device_trace_types.h"

/*
 * Get the Shire mask form Hart ID using Harts per Shire information.
 */
#define GET_SHIRE_MASK(hart_id)     (1UL << ((hart_id) / 64U))

/*
 * Get Hart mask form Hart ID using Harts per Shire information.
 */
#define GET_HART_MASK(hart_id)      (1UL << ((hart_id) % 64U))

/*
 * Trace event masks.
 * Breakdown of sections in struct trace_init_info_t::event_mask
 * 0,       Type: Single bit    Desc: Trace String Event
 * 1,       Type: Single bit    Desc: Trace PMC Event
 * 2,       Type: Single bit    Desc: Trace MARKER Event
 * 3-31,    TBD
 */
#define TRACE_EVENT_STRING              (1U << 0)
#define TRACE_EVENT_PMC                 (1U << 1)
#define TRACE_EVENT_MARKER              (1U << 2)
#define TRACE_EVENT_ENABLE_ALL          0XFFFFFFFF

/*
 * Trace event filters masks.
 * Breakdown of sections in struct trace_init_info_t::filter_mask
 * 0-7,     Type: uint8_t,      Desc: String log levels.
 * 8-15,    Type: bit mask,     Desc: PMC Filters
 * 16-31,   Type: bit mask,     Desc: Markers Filters
 * 32-61,   TBD
 */
#define TRACE_FILTER_STRING_MASK        0x000000FFUL
#define TRACE_FILTER_PMC_MASK           0x0000FF00UL
#define TRACE_FILTER_MARKERS_MASK       0x00FF0000UL
#define TRACE_FILTER_ENABLE_ALL         0XFFFFFFFFU

typedef uint8_t trace_enable_e;

/*
 * Trace string events filters.
 */
enum trace_enable {
    TRACE_ENABLE = 0,
    TRACE_DISABLE = 1
};

/*
 * Trace string events filters.
 */
enum trace_string_event {
    TRACE_EVENT_STRING_CRITICAL = 0,
    TRACE_EVENT_STRING_ERROR    = 1,
    TRACE_EVENT_STRING_WARNING  = 2,
    TRACE_EVENT_STRING_INFO     = 3,
    TRACE_EVENT_STRING_DEBUG    = 4
};

/*! \def CHECK_STRING_FILTER
    \brief This checks if trace string log level is enabled to log the given level.
*/
#if defined(MASTER_MINION)
#define CHECK_STRING_FILTER(cb, log_level) ((atomic_load_local_32(&cb->filter_mask) & TRACE_FILTER_STRING_MASK) >= log_level)
#else
#define CHECK_STRING_FILTER(cb, log_level) ((cb->filter_mask & TRACE_FILTER_STRING_MASK) >= log_level)
#endif
/*
 * Global tracing information used to initialize Trace.
 */
struct trace_init_info_t {
    uint64_t buffer;            /*!< Base address for Trace buffer. */
    uint32_t buffer_size;       /*!< Total size of the Trace buffer. */
    uint32_t threshold;         /*!< Threshold for free memory in the buffer for each hart. */
    uint64_t shire_mask;        /*!< Bit Mask of Shire to enable Trace Capture. */
    uint64_t thread_mask;       /*!< Bit Mask of Thread within a Shire to enable Trace Capture. */
    uint32_t event_mask;        /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
    uint32_t filter_mask;       /*!< This is a bit mask representing a list of filters for a given event to trace. */
};

/*
 * Per-thread tracing book-keeping information.
 */
struct trace_control_block_t {
    uint64_t base_per_hart;     /*!< Base address for Trace buffer. User have to populate this. */
    uint32_t size_per_hart;     /*!< Size of Trace buffer. User have to populate this. */
    uint32_t offset_per_hart;   /*!< Head pointer within the chunk of struct trace_init_info_t::buffer for each hart */
    uint32_t threshold;         /*!< Threshold for free memory in the buffer for each hart. */
    uint32_t event_mask;        /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
    uint32_t filter_mask;       /*!< This is a bit mask representing a list of filters for a given event to trace. */
    uint8_t  enable;            /*!< Enable/Disable Trace. */
    uint8_t  header;            /*!< Buffer header type of value trace_header_type_e */
} __attribute__((aligned(64)));

void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
    trace_header_type_e);
void Trace_String(enum trace_string_event log_level, struct trace_control_block_t *cb, const char *str);
void Trace_Format_String(enum trace_string_event log_level, struct trace_control_block_t *cb, const char *format, ...);
void Trace_PMC_All_Counters(struct trace_control_block_t *cb);
void Trace_PMC_Counter(struct trace_control_block_t *cb, enum pmc_counter_e counter);
void Trace_Value_u64(struct trace_control_block_t *cb, uint32_t tag, uint64_t value);
void Trace_Value_u32(struct trace_control_block_t *cb, uint32_t tag, uint32_t value);
void Trace_Value_u16(struct trace_control_block_t *cb, uint32_t tag, uint16_t value);
void Trace_Value_u8(struct trace_control_block_t *cb, uint32_t tag, uint8_t value);
void Trace_Value_float(struct trace_control_block_t *cb, uint32_t tag, float value);
void Trace_Memory(struct trace_control_block_t *cb, const uint8_t *src, uint16_t num_cache_line);
void *Trace_Buffer_Reserve(struct trace_control_block_t *cb, uint64_t size);
void Trace_Cmd_Status(struct trace_control_block_t *cb,
        const struct trace_cmd_status_internal_t *cmd_data);
void Trace_Power_Status(struct trace_control_block_t *cb,
    const struct trace_power_event_status_t *cmd_data);
#endif
