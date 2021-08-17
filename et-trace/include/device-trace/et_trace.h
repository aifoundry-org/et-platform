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

    Public interfaces:
        Trace_Init
        Trace_String
        Trace_Format_String
        Trace_PMC_All_Counters
        Trace_PMC_Counter
        Trace_Value_u64
        Trace_Value_u32
        Trace_Value_u16
        Trace_Value_u8
        Trace_Value_float
        Trace_Memory
*/
/***********************************************************************/

#ifndef DEVICE_TRACE_H
#define DEVICE_TRACE_H

#include <stdbool.h>
#include "et_trace_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get the Shire mask form Hart ID using Harts per Shire information.
 */
#define TRACE_SHIRE_MASK(hart_id)     (1UL << ((hart_id) / 64U))

/*
 * Get Hart mask form Hart ID using Harts per Shire information.
 */
#define TRACE_HART_MASK(hart_id)      (1UL << ((hart_id) % 64U))

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
typedef uint8_t trace_string_event_e;

/*
 * Trace string events filters.
 */
enum trace_string_event_e {
    TRACE_EVENT_STRING_CRITICAL = 0,
    TRACE_EVENT_STRING_ERROR    = 1,
    TRACE_EVENT_STRING_WARNING  = 2,
    TRACE_EVENT_STRING_INFO     = 3,
    TRACE_EVENT_STRING_DEBUG    = 4
};

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

void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb, uint8_t buff_header);
void Trace_String(trace_string_event_e log_level, struct trace_control_block_t *cb, const char *str);
void Trace_Format_String(trace_string_event_e log_level, struct trace_control_block_t *cb, const char *format, ...);
void Trace_PMC_All_Counters(struct trace_control_block_t *cb);
void Trace_PMC_Counter(struct trace_control_block_t *cb, pmc_counter_e counter);
void Trace_Value_u64(struct trace_control_block_t *cb, uint32_t tag, uint64_t value);
void Trace_Value_u32(struct trace_control_block_t *cb, uint32_t tag, uint32_t value);
void Trace_Value_u16(struct trace_control_block_t *cb, uint32_t tag, uint16_t value);
void Trace_Value_u8(struct trace_control_block_t *cb, uint32_t tag, uint8_t value);
void Trace_Value_float(struct trace_control_block_t *cb, uint32_t tag, float value);
void Trace_Memory(struct trace_control_block_t *cb, const uint8_t *src, uint16_t num_cache_line);
void *Trace_Buffer_Reserve(struct trace_control_block_t *cb, uint64_t size);
void Trace_Cmd_Status(struct trace_control_block_t *cb,
        const struct trace_event_cmd_status_t *cmd_data);
void Trace_Power_Status(struct trace_control_block_t *cb,
    const struct trace_event_power_status_t *cmd_data);

/*
 * Implement Trace encode functions by defining ET_TRACE_ENCODER_IMPL
 * in a *single* source file before including the header, i.e.:
 *
 *     #define ET_TRACE_ENCODER_IMPL
 *     #include "et_trace.h>
 *
 * Note: The implementation of Trace_Decode changes depending on
 *       whether `MASTER_MINION` is set or not (see device_trace_types.h)
 */

#ifdef ET_TRACE_ENCODER_IMPL

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ET_TRACE_GENERIC_HEADER(msg, id)    {ET_TRACE_WRITE(64, msg->header.cycle, PMU_Get_hpmcounter3());\
                                            ET_TRACE_WRITE(16, msg->header.type, id);}

/* Check if Trace is enabled for given control block. */
#define IS_TRACE_ENABLED(cb)            (ET_TRACE_READ(8, cb->enable) == TRACE_ENABLE)
/* Check if Trace String log level is enabled for given control block. */
#define IS_TRACE_STR_ENABLED(cb, log)   (IS_TRACE_ENABLED(cb) &&                           \
                                        (ET_TRACE_READ(32, cb->event_mask) & TRACE_EVENT_STRING) && \
                                        ((ET_TRACE_READ(32, cb->filter_mask) & TRACE_FILTER_STRING_MASK) >= log_level))

/* Check if user has provided implementation of optional its own primitives, if not then use default. */
#ifndef ET_TRACE_READ
#define ET_TRACE_READ(size, addr)               (addr)
#endif

#ifndef ET_TRACE_WRITE
#define ET_TRACE_WRITE(size, addr, val)         (addr = val)
#endif

#ifndef ET_TRACE_WRITE_F
#define ET_TRACE_WRITE_F(addr, val)             (addr = val)
#endif

#ifndef ET_TRACE_MEM_CPY
#define ET_TRACE_MEM_CPY(dest, src, size)       memcpy(dest, src, size)
#endif

#ifndef ET_TRACE_MESSAGE_HEADER
#define ET_TRACE_MESSAGE_HEADER(msg, id)        ET_TRACE_GENERIC_HEADER(msg, id)
#endif

#ifndef ET_TRACE_GET_TIMESTAMP
#define ET_TRACE_GET_TIMESTAMP(time)            (time+=1)
#endif

#ifndef ET_TRACE_GET_HPM_COUNTER
#define ET_TRACE_GET_HPM_COUNTER(counter_index, ret_val) (ret_val = counter_index)
#endif

union trace_header_u {
    struct
    {
        uint32_t hart_id; // Hart ID of the Hart which is logging Trace
        uint16_t type;    // One of enum trace_type_e
        uint8_t  pad[2];
    };
    uint64_t header_raw;
};

/* Mock: This counter should return the current cycle time.
 * Here we simply increment a static value. */
static inline uint64_t PMU_Get_hpmcounter3(void)
{
    uint64_t val = 0;
    ET_TRACE_GET_TIMESTAMP(val);
    return val;
}

static inline uint64_t PMU_Get_Counter(pmc_counter_e counter)
{
    uint64_t val;
    switch (counter) {
    case PMC_COUNTER_HPMCOUNTER4:
        ET_TRACE_GET_HPM_COUNTER(4, val);
        break;
    case PMC_COUNTER_HPMCOUNTER5:
        ET_TRACE_GET_HPM_COUNTER(5, val);
        break;
    case PMC_COUNTER_HPMCOUNTER6:
        ET_TRACE_GET_HPM_COUNTER(6, val);
        break;
    case PMC_COUNTER_HPMCOUNTER7:
        ET_TRACE_GET_HPM_COUNTER(7, val);
        break;
    case PMC_COUNTER_HPMCOUNTER8:
        ET_TRACE_GET_HPM_COUNTER(8, val);
        break;
    default:
        val = 0;
        break;
    }
    return val;
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
*
*   OUTPUTS
*
*       bool                    True: Buffer Is full, and reset is done.
*                               False: Buffer is not full yet.
*
***********************************************************************/
inline static bool trace_check_buffer_full(const struct trace_control_block_t *cb,
    uint64_t size)
{
    if((ET_TRACE_READ(32, cb->offset_per_hart) + size) > ET_TRACE_READ(32, cb->size_per_hart))
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
*       uint32_t                Pointer to return current offest in buffer.
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
    uint64_t size, uint32_t *current_offset)
{
    *current_offset = ET_TRACE_READ(32, cb->offset_per_hart);
    if((*current_offset + size) > ET_TRACE_READ(32, cb->threshold))
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
*       Trace_Buffer_Reserve
*
*   DESCRIPTION
*
*       This function reserves buffer for given size of data.
*       And if buffer threshold reached it notifies the Host,
*       but if buffer is full upto maximum size then it resets the buffer.
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
void *Trace_Buffer_Reserve(struct trace_control_block_t *cb, uint64_t size)
{
    void *head;
    uint32_t current_offset = 0;

    /* Check if Trace buffer is filled upto threshold. */
    if (trace_check_buffer_threshold(cb, size, &current_offset))
    {
        /* Check if host needs to be notified about reaching buffer threshold limit.
           This notification is only needed once when it reaches threshold for the first time,
           so this checks if we just reached threshold by including current data size.
           TODO: If "current_offset" is less than threshold then
           Notify the Host that we reached the notification threshold
           syscall(SYSCALL_TRACE_BUFFER_THRESHOLD_HIT) */

        /* Check if Trace buffer is filled upto threshold. Then do reset the buffer. */
        if (trace_check_buffer_full(cb, size))
        {
            /* Reset buffer. */
            current_offset = (ET_TRACE_READ(8, cb->header) == TRACE_STD_HEADER) ?
                sizeof(struct trace_buffer_std_header_t) : sizeof(struct trace_buffer_size_header_t);
        }
    }

    /* Update offset. */
    ET_TRACE_WRITE(32, cb->offset_per_hart, (uint32_t)(current_offset + size));
    head = (void *) (ET_TRACE_READ(64, cb->base_per_hart) + current_offset);

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
*       None
*
***********************************************************************/
void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
    uint8_t buff_header)
{
    if (!init_info || !cb)
        return;

    /* Check if it is a shortcut to enable all Trace events and filters. */
    cb->filter_mask = (init_info->event_mask == TRACE_EVENT_ENABLE_ALL) ?
                     TRACE_FILTER_ENABLE_ALL : init_info->filter_mask;

    cb->offset_per_hart = (buff_header == TRACE_STD_HEADER) ?
        sizeof(struct trace_buffer_std_header_t): sizeof(struct trace_buffer_size_header_t);

    cb->event_mask = init_info->event_mask;
    cb->threshold = init_info->threshold;
    cb->header = buff_header;
    cb->enable = TRACE_ENABLE;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_String
*
*   DESCRIPTION
*
*       A function to log Trace string message.
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
    if (IS_TRACE_STR_ENABLED(cb, log_level))
    {
        struct trace_string_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_STRING)

        ET_TRACE_MEM_CPY(entry->string, str,
            ((strlen(str) < TRACE_STRING_MAX_SIZE)? (strlen(str)+1): TRACE_STRING_MAX_SIZE));
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
void Trace_Format_String(trace_string_event_e log_level, struct trace_control_block_t *cb, const char *format, ...)
{
    if (IS_TRACE_STR_ENABLED(cb, log_level))
    {
        va_list args;
        struct trace_string_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_STRING)

        va_start(args, format);
        vsnprintf(entry->string, sizeof(entry->string), format, args);
        va_end(args);
    }
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
    struct trace_cmd_status_t *entry =
        Trace_Buffer_Reserve(cb, sizeof(*entry));

    ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_CMD_STATUS)
    ET_TRACE_WRITE(64, entry->cmd.raw_cmd, cmd_data->raw_cmd);
}

/* NB: This is still missing from the device-minion-runtime */
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
    struct trace_power_status_t *entry =
        Trace_Buffer_Reserve(cb, sizeof(*entry));

    ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_POWER_STATUS)
    ET_TRACE_WRITE(64, entry->power.raw_cmd, pwr_data->raw_cmd);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_PMC_All_Counters
*
*   DESCRIPTION
*
*       A function to log all PMC counters.
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
void Trace_PMC_All_Counters(struct trace_control_block_t *cb)
{
    if(cb->enable == true)
    {
        for (pmc_counter_e counter = PMC_COUNTER_HPMCOUNTER4;
            counter <= PMC_COUNTER_MEMSHIRE_FOO;
            counter++) {
            Trace_PMC_Counter(cb, counter);
        }
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_pmc_counter_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_PMC_COUNTER)

        ET_TRACE_WRITE(64, entry->value, PMU_Get_Counter(counter));
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_value_u64_t *entry =
                Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U64)

        ET_TRACE_WRITE(32, entry->tag, tag);
        ET_TRACE_WRITE(64, entry->value, value);
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_value_u32_t *entry =
                Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U32)

        ET_TRACE_WRITE(32, entry->tag, tag);
        ET_TRACE_WRITE(32, entry->value, value);
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_value_u16_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U16)

        ET_TRACE_WRITE(32, entry->tag, tag);
        ET_TRACE_WRITE(16, entry->value, value);
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_value_u8_t *entry =
                Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U8)

        ET_TRACE_WRITE(32, entry->tag, tag);
        ET_TRACE_WRITE(8, entry->value, value);
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
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_value_float_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_FLOAT)

        ET_TRACE_WRITE(32, entry->tag, tag);
        ET_TRACE_WRITE_F(entry->value, value);
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
*       trace_control_block_t     Trace control block of logging Thread/Hart.
*       uint8_t                   Pointer to memory.
*       uint16_t                  Size of memory in term of cache lines..
*
*   OUTPUTS
*
*       None
*
************************************************************************/
void Trace_Memory(struct trace_control_block_t *cb, const uint8_t *src,
                  uint16_t num_cache_line)
{
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_memory_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry) + (uint32_t)(num_cache_line*8));

        ET_TRACE_MESSAGE_HEADER(entry, TRACE_TYPE_MEMORY)

        ET_TRACE_WRITE(64, entry->src_addr, (uint64_t)(src));
        ET_TRACE_WRITE(64, entry->size, (uint64_t)(num_cache_line*8));

        for(uint16_t index=0; index < num_cache_line ; index++) {
           ET_TRACE_MEM_CPY(&entry->data[index*64], src, 64);
           src += 64;
        }
    }
}

#endif

#ifdef __cplusplus
}
#endif


#endif
