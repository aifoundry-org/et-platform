/* 
 * This file is largely copies the implementation from device-minion-runtime,
 * with a few modifications to make it compile for non-minions.
 */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "device_trace.h"

#include "mock_atomics.h"
#include "mock_etsoc.h"

/* For Master Minion all Trace data is based on L2 Cache, that means we need to perform all updates in L2.
   On other hand, for Service Processor and Compute Minion, Trace data is in L1 Cache, and updates are
   done by normal load/store operations. */
#if defined(MASTER_MINION)
/* All Trace CB operations are Atomic operations in L2 Cache. */

union data_u32_f
{
    uint32_t value_u32;
    float value_f;
};

/* Utilities for trace data updates in L2 Cache. */
#define READ_U64(addr)                  atomic_load_local_64(&addr)
#define READ_U32(addr)                  atomic_load_local_32(&addr)
#define READ_U16(addr)                  atomic_load_local_16(&addr)
#define READ_U8(addr)                   atomic_load_local_8(&addr)

#define WRITE_U64(addr, value)          atomic_store_local_64(&addr, value)
#define WRITE_U32(addr, value)          atomic_store_local_32(&addr, value)
#define WRITE_U16(addr, value)          atomic_store_local_16(&addr, value)
#define WRITE_U8(addr, value)           atomic_store_local_8(&addr, value)
/* TODO: Use atomic float store, when its implementation is vailable. */
#define WRITE_F(addr, value)            ({ union data_u32_f data; data.value_f = value;                 \
                                         atomic_store_local_32((void *)&addr, data.value_u32);})
#define MEM_CPY(dest, src, size)        ETSOC_Memory_Write_Local_Atomic(src, dest, size)

#define IS_TRACE_ENABLED(cb)            (READ_U8(cb->enable) == TRACE_ENABLE)
#define IS_TRACE_STR_ENABLED(cb, log)   (IS_TRACE_ENABLED(cb) &&                                        \
                                        (atomic_load_local_32(&cb->event_mask) & TRACE_EVENT_STRING) && \
                                        (CHECK_STRING_FILTER(cb, log)))

#define ADD_MESSAGE_HEADER(msg, id)     {WRITE_U64(msg->header.cycle, PMU_Get_hpmcounter3());   \
                                         WRITE_U32(msg->header.hart_id, get_hart_id());        \
                                         WRITE_U16(msg->header.type, id);}

#else
/* All Trace CB operations are normal operations in L1 Cache. */

/* Utilities for trace data updates in L1 Cache. */
#define READ_U64(var)                   (var)
#define READ_U32(var)                   (var)
#define READ_U16(var)                   (var)
#define READ_U8(var)                    (var)

#define WRITE_U64(var, value)           (var = value)
#define WRITE_U32(var, value)           (var = value)
#define WRITE_U16(var, value)           (var = value)
#define WRITE_U8(var, value)            (var = value)
#define WRITE_F(var, value)             (var = value)
#define MEM_CPY(dest, src, size)        memcpy(dest, src, size)

#define IS_TRACE_ENABLED(cb)            (cb->enable == TRACE_ENABLE)
#define IS_TRACE_STR_ENABLED(cb, log)   (IS_TRACE_ENABLED(cb) &&                    \
                                        (cb->event_mask & TRACE_EVENT_STRING) &&    \
                                        (CHECK_STRING_FILTER(cb, log)))

#define ADD_MESSAGE_HEADER(msg, id)     {WRITE_U64(msg->header.cycle, PMU_Get_hpmcounter3());   \
                                         WRITE_U16(msg->header.type, id);}
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
    return reg_hpmcounter3;
}

/* Mock: Since we don't have pmu counters, we simply return random values */
static inline uint64_t PMU_Get_Counter(enum pmc_counter_e counter)
{
    uint64_t val;
    switch (counter) {
    case PMC_COUNTER_HPMCOUNTER4:
        val = reg_hpmcounter4;
        break;
    case PMC_COUNTER_HPMCOUNTER5:
        val = reg_hpmcounter5;
        break;
    case PMC_COUNTER_HPMCOUNTER6:
        val = reg_hpmcounter6;
        break;
    case PMC_COUNTER_HPMCOUNTER7:
        val = reg_hpmcounter7;
        break;
    case PMC_COUNTER_HPMCOUNTER8:
        val = reg_hpmcounter8;
        break;
    case PMC_COUNTER_SHIRE_CACHE_FOO:
        val = 0; // syscall
        break;
    case PMC_COUNTER_MEMSHIRE_FOO:
        val = 0; // syscall
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
    if((READ_U32(cb->offset_per_hart) + size) > READ_U32(cb->size_per_hart))
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
    *current_offset = READ_U32(cb->offset_per_hart);
    if((*current_offset + size) > READ_U32(cb->threshold))
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
        if (trace_check_buffer_full(cb, size))//NOSONAR Do not merge this "if" with enclosing because of task pending above
        {
            /* Reset buffer. */
            (READ_U8(cb->header) == TRACE_STD_HEADER) ?
                WRITE_U32(cb->offset_per_hart, sizeof(struct trace_buffer_std_header_t)) :
                WRITE_U32(cb->offset_per_hart, sizeof(struct trace_buffer_size_header_t));

            head = (void *)(uintptr_t)(READ_U64(cb->base_per_hart));

            return head;
        }
    }

    /* Update offset. */
    WRITE_U32(cb->offset_per_hart, (uint32_t)(current_offset + size));
    head = (void *) (READ_U64(cb->base_per_hart) + current_offset);

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
void Trace_String(enum trace_string_event log_level, struct trace_control_block_t *cb, const char *str)
{
    if (IS_TRACE_STR_ENABLED(cb, log_level))
    {
        struct trace_string_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_STRING)

        MEM_CPY(entry->string, str, sizeof(entry->string));
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
void Trace_Format_String(enum trace_string_event log_level, struct trace_control_block_t *cb, const char *format, ...)
{
    if (IS_TRACE_STR_ENABLED(cb, log_level))
    {
        va_list args;
        struct trace_string_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_STRING)

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
*       trace_control_block_t        Trace control block of logging Hart.
*       trace_cmd_status_internal_t  Command status data.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Cmd_Status(struct trace_control_block_t *cb,
    const struct trace_cmd_status_internal_t *cmd_data)
{
    struct trace_cmd_status_t *entry =
        Trace_Buffer_Reserve(cb, sizeof(*entry));

    ADD_MESSAGE_HEADER(entry, TRACE_TYPE_CMD_STATUS)
    WRITE_U64(entry->cmd.raw_cmd, cmd_data->raw_cmd);
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
*       trace_control_block_t        Trace control block of logging Hart.
*       trace_power_event_status_t   Power status data.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Power_Status(struct trace_control_block_t *cb,
    const struct trace_power_event_status_t *pwr_data)
{
    struct trace_power_status_t *entry =
        Trace_Buffer_Reserve(cb, sizeof(*entry));

    ADD_MESSAGE_HEADER(entry, TRACE_TYPE_POWER_STATUS)
    WRITE_U64(entry->cmd.raw_cmd, pwr_data->raw_cmd);
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
        for (enum pmc_counter_e counter = PMC_COUNTER_HPMCOUNTER4;
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
void Trace_PMC_Counter(struct trace_control_block_t *cb, enum pmc_counter_e counter)
{
    if(IS_TRACE_ENABLED(cb))
    {
        struct trace_pmc_counter_t *entry =
            Trace_Buffer_Reserve(cb, sizeof(*entry));

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_PMC_COUNTER)

        WRITE_U64(entry->value, PMU_Get_Counter(counter));
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U64)

        WRITE_U32(entry->tag, tag);
        WRITE_U64(entry->value, value);
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U32)

        WRITE_U32(entry->tag, tag);
        WRITE_U32(entry->value, value);
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U16)

        WRITE_U32(entry->tag, tag);
        WRITE_U16(entry->value, value);
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_U8)

        WRITE_U32(entry->tag, tag);
        WRITE_U8(entry->value, value);
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_VALUE_FLOAT)

        WRITE_U32(entry->tag, tag);
        WRITE_F(entry->value, value);
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

        ADD_MESSAGE_HEADER(entry, TRACE_TYPE_MEMORY)

        WRITE_U64(entry->src_addr, (uint64_t)(src));
        WRITE_U64(entry->size, (uint64_t)(num_cache_line*8));

        for(uint16_t index=0; index < num_cache_line ; index++) {
           MEM_CPY(&entry->data[index*64], src, 64);
           src += 64;
        }
    }
}
