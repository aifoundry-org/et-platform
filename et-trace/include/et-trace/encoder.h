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
 * et-trace/encoder.h
 * Encode interface for Esperanto device traces.
 *
 *
 * USAGE
 *
 * In a *single* source file, put:
 *
 *     #define ET_TRACE_ENCODER_IMPL
 *     #include <et-trace/encoder.h>
 *
 * Other source files can include et-trace/encoder.h as normal.
 *
 * Most of the underlying primitives can be overwritten
 * with custom implementation (see CUSTOMIZING)
 *
 *
 * ABOUT
 *
 * This file provides the basic interface for primitive
 * data types that can be saved to a device trace.
 *
 * Public interfaces:
 *     Trace_Init
 *     Trace_String
 *     Trace_Format_String
 *     Trace_PMC_Counters_Compute
 *     Trace_PMC_Counters_Memory
 *     Trace_PMC_Counter
 *     Trace_Value_u64
 *     Trace_Value_u32
 *     Trace_Value_u16
 *     Trace_Value_u8
 *     Trace_Value_float
 *     Trace_Memory
 *     Trace_Cmd_Status
 *     Trace_Power_Status
 *     Trace_Execution_Stack
 *     Trace_Custom_Event
 *
 * The trace buffer itself is accessed via a control block.
 * This data structure has to be filled with a pointer to the
 * memory buffer that allocates the trace buffer.
 *
 * This is the basic way of initializing the trace buffer:
 *
 *     // This depends on your use case
 *     struct trace_buffer_std_header_t* buf = ...;
 *     size_t trace_size       = ...;
 *     trace_type_e trace_type = ...;
 *
 *     struct trace_control_block_t* cb = ...;
 *
 *     // Basic settings (in this case enable everything)
 *     struct trace_init_info_t info;
 *     info.shire_mask  = 0xFFFFFFFF;
 *     info.thread_mask = 0xFFFFFFFF;
 *     info.event_mask  = TRACE_EVENT_ENABLE_ALL;
 *     info.filter_mask = TRACE_FILTER_ENABLE_ALL;
 *     info.threshold   = trace_size;
 *
 *     // Initialize the control block
 *     cb->size_per_hart = trace_size;
 *     cb->base_per_hart = (uint64_t)trace_buf;
 *     cb->offset_per_hart = sizeof(struct trace_buffer_std_header_t);
 *     Trace_Init(&info, cb, TRACE_STD_HEADER);
 *
 *     // Setup the trace buffer itself
 *     ET_TRACE_WRITE_U32(buf->magic_header, TRACE_MAGIC_HEADER);
 *     ET_TRACE_WRITE_U32(buf->data_size, sizeof(struct trace_buffer_std_header_t));
 *     ET_TRACE_WRITE_U16(buf->type, trace_type);
 *
 *
 * After performing writes to the trace buffer, you'll need
 * to update its size, i.e. the number of valid bytes it contains.
 * This is usually done when this memory region is evicted:
 *
 *     ET_TRACE_WRITE_U32(buf->data_size, cb->offset_per_hart);
 *
 *
 * CUSTOMIZING
 *
 * The following are primitives that are used by the encoder implementation.
 * You can define custom implementations, for instance to perform reads/writes
 * via atomic operations, or to provide different implementations for underlying
 * hardware features depending on their availability.
 *
 *     // Memory operations
 *     ET_TRACE_READ_U8(Location)             Reads unsigned char from variable <location>
 *     ET_TRACE_READ_U16(Location)            Reads unsigned short int from variable <location>
 *     ET_TRACE_READ_U32(Location)            Reads unsigned int from variable <location>
 *     ET_TRACE_READ_U64(Location)            Reads unsigned long int  from variable <location>
 *     ET_TRACE_WRITE_U8(Location, Value)     Writes unsigned char to <location>
 *     ET_TRACE_WRITE_U16(Location, Value)    Writes unsigned short int to <location>
 *     ET_TRACE_WRITE_U32(Location, Value)    Writes unsigned int to <location>
 *     ET_TRACE_WRITE_U64(Location, Value)    Writes unsigned long int to <location>
 *     ET_TRACE_WRITE_FLOAT(Location, Value)  Writes FP <value> to variable <location>
 *
 *     // Buffer locking (must be defined in pair)
 *     ET_TRACE_BUFFER_LOCK_ACQUIRE       Acquires the lock for a shared trace buffer
 *     ET_TRACE_BUFFER_LOCK_RELEASE       Releases the lock for a shared trace bufer
 *
 *     // Hardware features
 *     ET_TRACE_GET_TIMESTAMP()           Returns current cycle time
 *     ET_TRACE_GET_HART_ID()             Returns ID of executing hart
 *     ET_TRACE_GET_HPM_COUNTER(Counter)  Returns sampled value of <counter>
 *
 * You can also customize the information that is written to trace entry headers.
 * This allows you to omit unused values for optimization purposes.
 *
 *     ET_TRACE_MESSAGE_HEADER(Entry, Type)
 *       Write information to header of <entry> that contains payload of <type>
 *
 * By default the hart_id is not written to the trace entry header.
 * You can change this behavior by defining `ET_TRACE_WITH_HART_ID`:
 *
 *     #define ET_TRACE_ENCODER_IMPL
 *     #define ET_TRACE_WITH_HART_ID
 *     #include <et-trace/encoder.h>
 *
 ***********************************************************************/

#ifndef ET_TRACE_ENCODER_H
#define ET_TRACE_ENCODER_H

#include <stdbool.h>
#include "layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get the Shire mask form Hart ID using Harts per Shire information.
 */
#define TRACE_SHIRE_MASK(hart_id) (1UL << ((hart_id) / 64U))

/*
 * Get Hart mask form Hart ID using Harts per Shire information.
 */
#define TRACE_HART_MASK(hart_id) (1UL << ((hart_id) % 64U))

/*
 * Trace event masks.
 * Breakdown of sections in struct trace_init_info_t::event_mask
 * 0,       Type: Single bit    Desc: Trace String Event
 * 1,       Type: Single bit    Desc: Trace PMC Event
 * 2,       Type: Single bit    Desc: Trace MARKER Event
 * 3-31,    TBD
 */
#define TRACE_EVENT_STRING     (1U << 0)
#define TRACE_EVENT_PMC        (1U << 1)
#define TRACE_EVENT_MARKER     (1U << 2)
#define TRACE_EVENT_ENABLE_ALL 0XFFFFFFFF

/*
 * Trace event filters masks.
 * Breakdown of sections in struct trace_init_info_t::filter_mask
 * 0-7,     Type: uint8_t,      Desc: String log levels.
 * 8-15,    Type: bit mask,     Desc: PMC Filters
 * 16-31,   Type: bit mask,     Desc: Markers Filters
 * 32-61,   TBD
 */
#define TRACE_FILTER_STRING_MASK  0x000000FFUL
#define TRACE_FILTER_PMC_MASK     0x0000FF00UL
#define TRACE_FILTER_MARKERS_MASK 0x00FF0000UL
#define TRACE_FILTER_ENABLE_ALL   0XFFFFFFFFU

/*
 * Trace error codes.
 */
#define TRACE_STATUS_SUCCESS    -0
#define TRACE_INVALID_CB        -1
#define TRACE_INVALID_INIT_INFO -2
#define TRACE_INVALID_THRESHOLD -3
#define TRACE_INVALID_BUF_SIZE  -4

typedef uint8_t trace_enable_e;

/*
 * Trace string events filters.
 */
enum trace_enable { TRACE_ENABLE = 0, TRACE_DISABLE = 1 };

/*
 * Trace string events filters.
 */
typedef uint8_t trace_string_event_e;

/*
 * Trace string events filters.
 */
enum trace_string_event {
    TRACE_EVENT_STRING_CRITICAL = 0,
    TRACE_EVENT_STRING_ERROR = 1,
    TRACE_EVENT_STRING_WARNING = 2,
    TRACE_EVENT_STRING_INFO = 3,
    TRACE_EVENT_STRING_DEBUG = 4
};

/*
 * Per-thread tracing book-keeping information.
 */
struct trace_control_block_t {
    void (*buffer_lock_acquire)(void); /*!< Function pointer for acquiring trace buffer lock */
    void (*buffer_lock_release)(void); /*!< Function pointer for releasing trace buffer lock */
    uint64_t base_per_hart; /*!< Base address for Trace buffer. User have to populate this. */
    uint32_t size_per_hart; /*!< Size of Trace buffer. User have to populate this. */
    uint32_t
        offset_per_hart; /*!< Head pointer within the chunk of struct trace_init_info_t::buffer for each hart */
    uint32_t threshold; /*!< Threshold for free memory in the buffer for each hart. */
    uint32_t
        event_mask; /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
    uint32_t
        filter_mask; /*!< This is a bit mask representing a list of filters for a given event to trace. */
    uint8_t enable; /*!< Enable/Disable Trace. */
    uint8_t header; /*!< Buffer header type of value trace_header_type_e */
} __attribute__((aligned(64)));

int32_t Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
                uint8_t buff_header);
int32_t Trace_Config(const struct trace_config_info_t *config_info, struct trace_control_block_t *cb);
void Trace_String(trace_string_event_e log_level, struct trace_control_block_t *cb,
                  const char *str);
void Trace_Format_String(trace_string_event_e log_level, struct trace_control_block_t *cb,
                         const char *format, ...);
void Trace_PMC_Counters_Compute(struct trace_control_block_t *cb);
void Trace_PMC_Counters_Memory(struct trace_control_block_t *cb);
void Trace_PMC_Counter(struct trace_control_block_t *cb, pmc_counter_e counter);
void Trace_Value_u64(struct trace_control_block_t *cb, uint32_t tag, uint64_t value);
void Trace_Value_u32(struct trace_control_block_t *cb, uint32_t tag, uint32_t value);
void Trace_Value_u16(struct trace_control_block_t *cb, uint32_t tag, uint16_t value);
void Trace_Value_u8(struct trace_control_block_t *cb, uint32_t tag, uint8_t value);
void Trace_Value_float(struct trace_control_block_t *cb, uint32_t tag, float value);
void *Trace_Memory(struct trace_control_block_t *cb, const uint8_t *src, uint32_t size);
void Trace_Cmd_Status(struct trace_control_block_t *cb,
                      const struct trace_event_cmd_status_t *cmd_data);
void Trace_Power_Status(struct trace_control_block_t *cb,
                        const struct trace_event_power_status_t *cmd_data);
void *Trace_Execution_Stack(struct trace_control_block_t *cb,
                            const struct dev_context_registers_t *regs);
void *Trace_Custom_Event(struct trace_control_block_t *cb, uint32_t custom_type,
                         const uint8_t *payload, uint32_t payload_size);

#ifdef ET_TRACE_ENCODER_IMPL

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Check if user has provided implementation of optional its own primitives, if not then use default. */
#ifndef ET_TRACE_READ_U8
#define ET_TRACE_READ_U8(var)          (var)
#endif

#ifndef ET_TRACE_READ_U16
#define ET_TRACE_READ_U16(var)         (var)
#endif

#ifndef ET_TRACE_READ_U32
#define ET_TRACE_READ_U32(var)         (var)
#endif

#ifndef ET_TRACE_READ_U64
#define ET_TRACE_READ_U64(var)         (var)
#endif

#ifndef ET_TRACE_READ_U64_PTR
#define ET_TRACE_READ_U64_PTR(var) (var)
#endif

#ifndef ET_TRACE_WRITE_U8
#define ET_TRACE_WRITE_U8(var, val)    (var = val)
#endif

#ifndef ET_TRACE_WRITE_U16
#define ET_TRACE_WRITE_U16(var, val)   (var = val)
#endif

#ifndef ET_TRACE_WRITE_U32
#define ET_TRACE_WRITE_U32(var, val)   (var = val)
#endif

#ifndef ET_TRACE_WRITE_U64
#define ET_TRACE_WRITE_U64(var, val)   (var = val)
#endif

#ifndef ET_TRACE_WRITE_FLOAT
#define ET_TRACE_WRITE_FLOAT(loc, val) (loc = val)
#endif

#ifndef ET_TRACE_MEM_CPY
#define ET_TRACE_MEM_CPY(dest, src, size) memcpy(dest, src, size)
#endif

#ifndef ET_TRACE_STRLEN
#define ET_TRACE_STRLEN(str) strlen(str)
#endif

#ifndef ET_TRACE_STRING_MAX_SIZE
#define ET_TRACE_STRING_MAX_SIZE 512
#endif

#ifndef ET_TRACE_GET_TIMESTAMP
#define ET_TRACE_GET_TIMESTAMP() 0
#endif

#ifndef ET_TRACE_GET_HPM_COUNTER
#define ET_TRACE_GET_HPM_COUNTER(counter) 0
#endif

#ifndef ET_TRACE_GET_SHIRE_CACHE_COUNTER
#define ET_TRACE_GET_SHIRE_CACHE_COUNTER(counter) 0
#endif

#ifndef ET_TRACE_GET_MEM_SHIRE_COUNTER
#define ET_TRACE_GET_MEM_SHIRE_COUNTER(counter) 0
#endif

#ifndef ET_TRACE_GET_HART_ID
#define ET_TRACE_WRITE_HART_ID(msg)
#else
#define ET_TRACE_WRITE_HART_ID(msg) ET_TRACE_WRITE_U16(msg->header.hart_id, (uint16_t)ET_TRACE_GET_HART_ID())
#endif

#define ET_TRACE_GET_PAYLOAD_SIZE(packet_size)  (packet_size - sizeof(struct trace_entry_header_t))

#ifndef ET_TRACE_MESSAGE_HEADER
#define ET_TRACE_MESSAGE_HEADER(msg, size, id)                           \
    {                                                                    \
        ET_TRACE_WRITE_U64(msg->header.cycle, ET_TRACE_GET_TIMESTAMP()); \
        ET_TRACE_WRITE_U32(msg->header.payload_size, size);              \
        ET_TRACE_WRITE_HART_ID(msg);                                     \
        ET_TRACE_WRITE_U16(msg->header.type, id);                        \
    }
#endif

/* Check if Trace is enabled for given control block. */
inline static bool trace_is_enabled(const struct trace_control_block_t *cb)
{
    return ET_TRACE_READ_U8(cb->enable) == TRACE_ENABLE;
}

/* Check if Trace String log level is enabled for given control block. */
inline static bool trace_is_str_enabled(const struct trace_control_block_t *cb,
                                        trace_string_event_e log_level)
{
    return (trace_is_enabled(cb) && (ET_TRACE_READ_U32(cb->event_mask) & TRACE_EVENT_STRING) &&
            ((ET_TRACE_READ_U32(cb->filter_mask) & TRACE_FILTER_STRING_MASK) >= log_level));
}

inline static uint32_t trace_get_header_size(const struct trace_control_block_t *cb)
{
    return (ET_TRACE_READ_U8(cb->header) == TRACE_STD_HEADER) ?
                        sizeof(struct trace_buffer_std_header_t) :
                        sizeof(struct trace_buffer_size_header_t);
}

/************************************************************************
*
*   FUNCTION
*
*       trace_check_buffer_min_size
*
*   DESCRIPTION
*
*       This function checks Trace buffer's minimum size requirement.
*       It checks that if buffer size is enough to log a (max sized)
*       fixed length Trace event.
*       NOTE: It is not thread safe, should only be called from Trace_Init()
*
*   INPUTS
*
*       trace_control_block_t   Trace control block.
*       uint32_t                Buffer header size.
*
*   OUTPUTS
*
*       bool                    True: Buffer fullfil min size requirement.
*                               False: Buffer size is less then minimum size required.
*
***********************************************************************/
inline static bool trace_check_buffer_min_size(const struct trace_control_block_t *cb, uint32_t hdr_size)
{
    /* Get the maximum size for fixed sized trace events.
       This union contains all fixed sized trace events, sp the size
       to union is equal to the maximum sized event.
       NOTE: For new fixed sized event this union should be updated to include that new event. */
    union max_event {
        struct trace_cmd_status_t event1;
        struct trace_power_status_t event2;
        struct trace_string_t event3;
        struct trace_value_float_t event4;
        struct trace_value_u64_t event5;
        struct trace_pmc_counter_t event6;
        struct trace_execution_stack_t event7;
    };

    size_t max_event_size = (sizeof(union max_event) > (ET_TRACE_STRING_MAX_SIZE + hdr_size)) ?
                            sizeof(union max_event) : (ET_TRACE_STRING_MAX_SIZE + hdr_size);

    if ((cb->size_per_hart >= hdr_size) &&
        (max_event_size < (cb->size_per_hart - hdr_size)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       trace_check_buffer_full
*
*   DESCRIPTION
*
*       This function checks if buffer is completely filled upto maximum size.
*
*   INPUTS
*
*       trace_control_block_t   Trace control block.
*       uint64_t                Size of buffer to be reserved.
*       uint32_t                Value of current offset of trace.
*
*   OUTPUTS
*
*       bool                    True: Buffer Is full, and reset is done.
*                               False: Buffer is not full yet.
*
***********************************************************************/
inline static bool trace_check_buffer_full(const struct trace_control_block_t *cb, uint64_t size,
                                           uint32_t current_offset)
{
    if ((current_offset + size) > ET_TRACE_READ_U32(cb->size_per_hart)) {
        return true;
    } else {
        return false;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       trace_check_buffer_threshold
*
*   DESCRIPTION
*
*       This function checks if buffer is filled upto threshold limit.
*
*   INPUTS
*
*       trace_control_block_t   Trace control block.
*       uint64_t                Size of buffer to be reserved.
*       uint32_t                Value of current offset of trace.
*                               NOTE: this offset does not include new
*                               reserved buffer size.
*
*   OUTPUTS
*
*       bool                    True: Buffer crossed threshold
*                               False: Buffer is not filled upto threshold.
*
***********************************************************************/
inline static bool trace_check_buffer_threshold(const struct trace_control_block_t *cb,
                                                uint64_t size, uint32_t current_offset)
{
    if ((current_offset + size) > ET_TRACE_READ_U32(cb->threshold)) {
        return true;
    } else {
        return false;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       trace_buffer_reserve
*
*   DESCRIPTION
*
*       This function reserves buffer for given size of data.
*       And if buffer threshold reached it notifies the Host,
*       but if buffer is full upto maximum size then it resets the buffer.
*       NOTE: For variable sized Trace packets, user must make sure that
*       data size does not exceeds total buffer size . For fixed sized packets,
*       Trace_Init make sure that size is enough to log largest Trace packet.
*
*   INPUTS
*
*       trace_control_block_t   Trace control block.
*       uint64_t                Size of buffer to be reserved.
*
*   OUTPUTS
*
*       void*                   Pointer to buffer head.
*
***********************************************************************/
static inline void *trace_buffer_reserve(struct trace_control_block_t *cb, uint64_t size)
{
    void *head;
    uint32_t current_offset;

    void (*lock_acquire)(void) = NULL;
    void (*lock_release)(void) = NULL;

    /* Load function ptr for buffer lock acquire */
    lock_acquire = (void (*)(void))(uintptr_t)ET_TRACE_READ_U64_PTR(cb->buffer_lock_acquire);

    /* Acquire the lock and load release lock function ptr if lock acquire function is defined */
    if (lock_acquire != NULL) {
        lock_release = (void (*)(void))(uintptr_t)ET_TRACE_READ_U64_PTR(cb->buffer_lock_release);
        lock_acquire();
    }

    /* Read the current offset value of trace buffer */
    current_offset = ET_TRACE_READ_U32(cb->offset_per_hart);

    /* Check if Trace buffer is filled upto threshold. */
    if (trace_check_buffer_threshold(cb, size, current_offset)) {
        /* Check if host needs to be notified about reaching buffer threshold limit.
           This notification is only needed once when it reaches threshold for the first time,
           so this checks if we just reached threshold by including current data size.
           TODO: If "current_offset" is less than size then
           Notify the Host that we reached the notification threshold
           syscall(SYSCALL_TRACE_BUFFER_THRESHOLD_HIT) */

        /* Check if Trace buffer is filled upto threshold. Then do reset the buffer. */
        if (trace_check_buffer_full(cb, size, current_offset)) {
            /* Reset buffer. */
            current_offset = trace_get_header_size(cb);
        }
    }

    /* Update offset. */
    ET_TRACE_WRITE_U32(cb->offset_per_hart, (uint32_t)(current_offset + size));

    /* Release the lock */
    if (lock_release != NULL) {
        lock_release();
    }

    /* Update the head pointer to write to */
    head = (void *)(ET_TRACE_READ_U64(cb->base_per_hart) + current_offset);

    return head;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init
*
*   DESCRIPTION
*
*       This function initializes the Trace. The caller function must
*       populate size and base of the buffer in trace control block.
*
*   INPUTS
*
*       trace_init_info_t       Global tracing information used to initialize Trace.
*       trace_control_block_t   A return pointer to hold intialized Trace control block for
*                               given Hart ID.
*       trace_header_type_e     Device Buffer header type. MM and only use TRACE_STD_HEADER type.
*                               CM use both types.
*
*   OUTPUTS
*
*       int32_t                  Successful status or error code.
*
***********************************************************************/
int32_t Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
                uint8_t buff_header)
{
    uint32_t header_size = (buff_header == TRACE_STD_HEADER) ?
                              sizeof(struct trace_buffer_std_header_t) :
                              sizeof(struct trace_buffer_size_header_t);

    if (!cb)
    {
        return TRACE_INVALID_CB;
    }
    else if (!init_info)
    {
        cb->enable = TRACE_DISABLE;
        return TRACE_INVALID_INIT_INFO;
    }
    else if (!trace_check_buffer_min_size(cb, header_size))
    {
        cb->enable = TRACE_DISABLE;
        return TRACE_INVALID_BUF_SIZE;
    }
    else if (((cb->buffer_lock_acquire != NULL) && (cb->buffer_lock_release == NULL)) ||
             ((cb->buffer_lock_acquire == NULL) && (cb->buffer_lock_release != NULL)))
    {
        cb->enable = TRACE_DISABLE;
        return TRACE_INVALID_INIT_INFO;
    }

    /* Check if it is a shortcut to enable all Trace events and filters. */
    cb->filter_mask = (init_info->event_mask == TRACE_EVENT_ENABLE_ALL) ? TRACE_FILTER_ENABLE_ALL :
                                                                          init_info->filter_mask;

    /* Update offset as per Trace buffer header size. */
    cb->offset_per_hart = header_size;

    cb->event_mask = init_info->event_mask;
    cb->threshold = ((init_info->threshold > 0) || (init_info->threshold < cb->size_per_hart)) ?
                      init_info->threshold : cb->size_per_hart;
    cb->header = buff_header;
    cb->enable = TRACE_ENABLE;

    return TRACE_STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Config
*
*   DESCRIPTION
*
*       This function configures the Trace.
*       NOTE:Trace must be initialized using Trace_init() before this
*       function.
*
*   INPUTS
*
*       trace_config_info_t     Global tracing information used to
*                               configure Trace.
*       trace_control_block_t   Pointer to Trace control block.
*
*   OUTPUTS
*
*       int32_t                  Successful status or error code.
*
***********************************************************************/
int32_t Trace_Config(const struct trace_config_info_t *config_info, struct trace_control_block_t *cb)
{
    if (!cb)
    {
        return TRACE_INVALID_CB;
    }
    else if (!config_info)
    {
        return TRACE_INVALID_INIT_INFO;
    }
    else if (config_info->threshold > ET_TRACE_READ_U32(cb->size_per_hart))
    {
        return TRACE_INVALID_THRESHOLD;
    }

    /* Check if it is a shortcut to enable all Trace events and filters. */
    ET_TRACE_WRITE_U32(cb->filter_mask, (config_info->event_mask == TRACE_EVENT_ENABLE_ALL) ?
        TRACE_FILTER_ENABLE_ALL : config_info->filter_mask);
    ET_TRACE_WRITE_U32(cb->event_mask, config_info->event_mask);
    ET_TRACE_WRITE_U32(cb->threshold, config_info->threshold);

    return TRACE_STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_String
*
*   DESCRIPTION
*
*       A function to log Trace string message. Strings longer than
*       ET_TRACE_STRING_MAX_SIZE will not be logged into Trace.
*
*   INPUTS
*
*       trace_string_event        Trace String event type.
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       const char                Log Message.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_String(trace_string_event_e log_level, struct trace_control_block_t *cb, const char *str)
{
    /* Get string message size plus one null termination character.*/
    size_t str_length = TRACE_STRING_SIZE_ALIGN(ET_TRACE_STRLEN(str) + 1);
    str_length = (str_length < ET_TRACE_STRING_MAX_SIZE) ? str_length: ET_TRACE_STRING_MAX_SIZE;

    if (trace_is_str_enabled(cb, log_level)) {
        struct trace_string_t *entry =
            (struct trace_string_t *)trace_buffer_reserve(cb, (sizeof(*entry) + str_length));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)str_length, TRACE_TYPE_STRING)
        ET_TRACE_MEM_CPY(entry->string, str, str_length);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Format_String
*
*   DESCRIPTION
*
*       A function to log Trace string message with given formatting.
*       This is not supported yet.
*
*   INPUTS
*
*       trace_string_event        Trace String event type.
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       const char                Log Message.
*       const char                Log message string format.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Format_String(trace_string_event_e log_level, struct trace_control_block_t *cb,
                         const char *format, ...)
{
    va_list args;
    va_start(args, format);

    /* Get string message size plus one null termination character.*/
    size_t str_length = TRACE_STRING_SIZE_ALIGN((size_t)vsnprintf(0, 0, format, args) + 1);
    str_length = (str_length < ET_TRACE_STRING_MAX_SIZE) ? str_length: ET_TRACE_STRING_MAX_SIZE;

    if (trace_is_str_enabled(cb, log_level)) {
        struct trace_string_t *entry =
            (struct trace_string_t *)trace_buffer_reserve(cb, (sizeof(*entry) + str_length));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)str_length, TRACE_TYPE_STRING)
        va_start(args, format);
        vsnprintf(entry->string, str_length, format, args);
    }
    va_end(args);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Cmd_Status
*
*   DESCRIPTION
*
*       A function to log Trace command status message.
*
*   INPUTS
*
*       trace_control_block_t       Trace control block of logging Hart.
*       trace_event_cmd_status_t    Command status data.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Cmd_Status(struct trace_control_block_t *cb,
                      const struct trace_event_cmd_status_t *cmd_data)
{
    if (trace_is_enabled(cb)) {
        struct trace_cmd_status_t *entry =
            (struct trace_cmd_status_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_CMD_STATUS)
        ET_TRACE_WRITE_U64(entry->cmd.raw_cmd, cmd_data->raw_cmd);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Power_Status
*
*   DESCRIPTION
*
*       A function to log Trace power status message.
*
*   INPUTS
*
*       trace_control_block_t       Trace control block of logging Hart
*       trace_event_power_status_t  Power status data.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Power_Status(struct trace_control_block_t *cb,
                        const struct trace_event_power_status_t *pwr_data)
{
    if (trace_is_enabled(cb)) {
        struct trace_power_status_t *entry =
            (struct trace_power_status_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_POWER_STATUS)
        ET_TRACE_WRITE_U64(entry->power.raw_cmd, pwr_data->raw_cmd);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_PMC_Counters_Compute
*
*   DESCRIPTION
*
*       A function to log all Minion and Neighborhood PMC counters.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_PMC_Counters_Compute(struct trace_control_block_t *cb)
{
    if (trace_is_enabled(cb)) {
        struct trace_pmc_counters_compute_t *entry =
            (struct trace_pmc_counters_compute_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_PMC_COUNTERS_COMPUTE)
        ET_TRACE_WRITE_U64(entry->hpmcounter3, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER3));
        ET_TRACE_WRITE_U64(entry->hpmcounter4, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER4));
        ET_TRACE_WRITE_U64(entry->hpmcounter5, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER5));
        ET_TRACE_WRITE_U64(entry->hpmcounter6, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER6));
        ET_TRACE_WRITE_U64(entry->hpmcounter7, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER7));
        ET_TRACE_WRITE_U64(entry->hpmcounter8, ET_TRACE_GET_HPM_COUNTER(PMC_COUNTER_HPMCOUNTER8));
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_PMC_Counters_Memory
*
*   DESCRIPTION
*
*       A function to log all Shire-cache and Mem-shire PMC counters.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_PMC_Counters_Memory(struct trace_control_block_t *cb)
{
    if (trace_is_enabled(cb)) {
        struct trace_pmc_counters_memory_t *entry =
            (struct trace_pmc_counters_memory_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_PMC_COUNTERS_MEMORY)
        ET_TRACE_WRITE_U64(entry->sc_pmc0,
            ET_TRACE_GET_SHIRE_CACHE_COUNTER(PMC_COUNTER_SHIRE_CACHE_1 - PMC_COUNTER_SHIRE_CACHE_CYCLE));
        ET_TRACE_WRITE_U64(entry->sc_pmc1,
            ET_TRACE_GET_SHIRE_CACHE_COUNTER(PMC_COUNTER_SHIRE_CACHE_2 - PMC_COUNTER_SHIRE_CACHE_CYCLE));
        ET_TRACE_WRITE_U64(entry->ms_pmc0,
            ET_TRACE_GET_MEM_SHIRE_COUNTER(PMC_COUNTER_MEMSHIRE_1 - PMC_COUNTER_MEMSHIRE_CYCLE));
        ET_TRACE_WRITE_U64(entry->ms_pmc1,
            ET_TRACE_GET_MEM_SHIRE_COUNTER(PMC_COUNTER_MEMSHIRE_2 - PMC_COUNTER_MEMSHIRE_CYCLE));
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_PMC_Counter
*
*   DESCRIPTION
*
*       A function to log given PMC counter only.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       pmc_counter_e             PMC counter number to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_PMC_Counter(struct trace_control_block_t *cb, pmc_counter_e counter)
{
    if (trace_is_enabled(cb)) {
        struct trace_pmc_counter_t *entry =
            (struct trace_pmc_counter_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_PMC_COUNTER)
        ET_TRACE_WRITE_U8(entry->counter, counter);

        switch(counter)
        {
            case PMC_COUNTER_SHIRE_CACHE_CYCLE:
            case PMC_COUNTER_SHIRE_CACHE_1:
            case PMC_COUNTER_SHIRE_CACHE_2:
                /* Sample SC PMCs */
                ET_TRACE_WRITE_U64(entry->value, ET_TRACE_GET_SHIRE_CACHE_COUNTER(counter - PMC_COUNTER_SHIRE_CACHE_CYCLE));
                break;
            case PMC_COUNTER_MEMSHIRE_CYCLE:
            case PMC_COUNTER_MEMSHIRE_1:
            case PMC_COUNTER_MEMSHIRE_2:
                /* Sample memshire PMCs */
                ET_TRACE_WRITE_U64(entry->value, ET_TRACE_GET_MEM_SHIRE_COUNTER(counter - PMC_COUNTER_MEMSHIRE_CYCLE));
                break;
            default:
                /* Sample and log Minion and Neighborhood PMCs */
                ET_TRACE_WRITE_U64(entry->value, ET_TRACE_GET_HPM_COUNTER(counter));
                break;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Value_u64
*
*   DESCRIPTION
*
*       A function to log Trace scalar 64 bit value.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint64_t                  Tag for scalar value.
*       uint64_t                  Value to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Value_u64(struct trace_control_block_t *cb, uint32_t tag, uint64_t value)
{
    if (trace_is_enabled(cb)) {
        struct trace_value_u64_t *entry =
            (struct trace_value_u64_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_VALUE_U64)

        ET_TRACE_WRITE_U32(entry->tag, tag);
        ET_TRACE_WRITE_U64(entry->value, value);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Value_u32
*
*   DESCRIPTION
*
*       A function to log Trace scalar 32 bit value.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint64_t                  Tag for scalar value.
*       uint64_t                  Value to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Value_u32(struct trace_control_block_t *cb, uint32_t tag, uint32_t value)
{
    if (trace_is_enabled(cb)) {
        struct trace_value_u32_t *entry =
            (struct trace_value_u32_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_VALUE_U32)

        ET_TRACE_WRITE_U32(entry->tag, tag);
        ET_TRACE_WRITE_U32(entry->value, value);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Value_u16
*
*   DESCRIPTION
*
*       A function to log Trace scalar 16 bit value.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint64_t                  Tag for scalar value.
*       uint64_t                  Value to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Value_u16(struct trace_control_block_t *cb, uint32_t tag, uint16_t value)
{
    if (trace_is_enabled(cb)) {
        struct trace_value_u16_t *entry =
            (struct trace_value_u16_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_VALUE_U16)

        ET_TRACE_WRITE_U32(entry->tag, tag);
        ET_TRACE_WRITE_U16(entry->value, value);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Value_u8
*
*   DESCRIPTION
*
*       A function to log Trace scalar 8 bit value.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint64_t                  Tag for scalar value.
*       uint64_t                  Value to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Value_u8(struct trace_control_block_t *cb, uint32_t tag, uint8_t value)
{
    if (trace_is_enabled(cb)) {
        struct trace_value_u8_t *entry =
            (struct trace_value_u8_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_VALUE_U8)

        ET_TRACE_WRITE_U32(entry->tag, tag);
        ET_TRACE_WRITE_U8(entry->value, value);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Value_float
*
*   DESCRIPTION
*
*       A function to log Trace scalar float (32 bit) value.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint64_t                  Tag for scalar value.
*       uint64_t                  Value to log.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Value_float(struct trace_control_block_t *cb, uint32_t tag, float value)
{
    if (trace_is_enabled(cb)) {
        struct trace_value_float_t *entry =
            (struct trace_value_float_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_VALUE_FLOAT)

        ET_TRACE_WRITE_U32(entry->tag, tag);
        ET_TRACE_WRITE_FLOAT(entry->value, value);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Memory
*
*   DESCRIPTION
*
*       A function to log specified memory into Trace.
*
*   INPUTS
*
*       cb     Trace control block of logging Thread/Hart.
*       src    Pointer to memory.
*       size   Size of memory in bytes.
*
*   OUTPUTS
*
*       Pointer to starting address of event
*
************************************************************************/
void *Trace_Memory(struct trace_control_block_t *cb, const uint8_t *src, uint32_t size)
{
    struct trace_memory_t *entry = NULL;

    if (trace_is_enabled(cb) &&
        (size <= (ET_TRACE_READ_U32(cb->size_per_hart) - trace_get_header_size(cb)))) {
        entry = (struct trace_memory_t *)trace_buffer_reserve(cb, sizeof(*entry) + size);

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry) + size), TRACE_TYPE_MEMORY)

        ET_TRACE_WRITE_U64(entry->src_addr, (uint64_t)(src));
        ET_TRACE_WRITE_U64(entry->size, size);
        ET_TRACE_MEM_CPY(entry->data, src, size);
    }

    return (void*)entry;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Execution_Stack
*
*   DESCRIPTION
*
*       A function to log device execution stack in case of runtime exception.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       regs                      Pointer to filled device context registers.
*
*   OUTPUTS
*
*       Pointer to starting address of event
*
************************************************************************/
void *Trace_Execution_Stack(struct trace_control_block_t *cb,
                            const struct dev_context_registers_t *regs)
{
    struct trace_execution_stack_t *entry = NULL;

    if (trace_is_enabled(cb)) {
        entry = (struct trace_execution_stack_t *)trace_buffer_reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry)), TRACE_TYPE_EXCEPTION)
        ET_TRACE_MEM_CPY(&entry->registers, regs, sizeof(struct dev_context_registers_t));
    }

    return (void*)entry;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Custom_Event
*
*   DESCRIPTION
*
*       A function to log a custom event in trace buffer.
*
*   INPUTS
*
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       custom_type               ID of the custom event
*       payload                   Pointer to payload
*       payload_size              Size of the payload of custom event
*
*   OUTPUTS
*
*       Pointer to starting address of event
*
************************************************************************/
void *Trace_Custom_Event(struct trace_control_block_t *cb, uint32_t custom_type,
                         const uint8_t *payload, uint32_t payload_size)
{
    struct trace_custom_event_t *entry = NULL;

    /* Check if trace is enabled and the payload size is less than the total buffer size */
    if (trace_is_enabled(cb) &&
        (payload_size <= (ET_TRACE_READ_U32(cb->size_per_hart) - trace_get_header_size(cb)))) {
        /* Reserve the trace buffer */
        entry = (struct trace_custom_event_t *)trace_buffer_reserve(cb, sizeof(*entry) + payload_size);

        ET_TRACE_MESSAGE_HEADER(entry, (uint32_t)ET_TRACE_GET_PAYLOAD_SIZE(sizeof(*entry) + payload_size), TRACE_TYPE_CUSTOM_EVENT)
        ET_TRACE_WRITE_U32(entry->custom_type, custom_type);
        ET_TRACE_WRITE_U32(entry->payload_size, payload_size);
        ET_TRACE_MEM_CPY(entry->payload, payload, payload_size);
    }

    return (void*)entry;
}

#endif /* ET_TRACE_ENCODER_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* ET_TRACE_ENCODER_H */
