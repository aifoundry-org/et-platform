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
/*! \file device_trace.c
    \brief A C module that implements the Trace services for device side.

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
*/
/***********************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "device_trace.h"

enum cop_dest {
   to_L1  = 0x0ULL,
   to_L2  = 0x1ULL,
   to_L3  = 0x2ULL,
   to_Mem = 0x3ULL
};

static inline void __attribute__((always_inline)) evict_va(uint64_t use_tmask, uint64_t dst, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id)
{
   uint64_t csr_enc = ((use_tmask & 1                     ) << 63 ) |
                      ((dst       & 0x3                   ) << 58 ) | //00=L1, 01=L2, 10=L3, 11=MEM
                      ((addr      & 0xFFFFFFFFFFC0ULL     )       ) |
                      ((num_lines & 0xF                   )       ) ;

   register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "csrw 0x89f, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
   );
}

static inline void __attribute__((always_inline)) evict(enum cop_dest dest, uint64_t address, uint64_t size)
{
    evict_va(0, dest, address, ((address & 0x3F) + size) >> 6, 64, 0);
}

static inline uint64_t PMU_Get_hpmcounter3(void)
{
    uint64_t val;
    __asm__ __volatile__("csrr %0, hpmcounter3\n" : "=r"(val));
    return val;
}

static inline uint64_t PMU_Get_Counter(enum pmc_counter_e counter)
{
    uint64_t val;
    switch (counter) {
    case PMC_COUNTER_HPMCOUNTER4:
        __asm__ __volatile__("csrr %0, hpmcounter4\n" : "=r"(val));
        break;
    case PMC_COUNTER_HPMCOUNTER5:
        __asm__ __volatile__("csrr %0, hpmcounter5\n" : "=r"(val));
        break;
    case PMC_COUNTER_HPMCOUNTER6:
        __asm__ __volatile__("csrr %0, hpmcounter6\n" : "=r"(val));
        break;
    case PMC_COUNTER_HPMCOUNTER7:
        __asm__ __volatile__("csrr %0, hpmcounter7\n" : "=r"(val));
        break;
    case PMC_COUNTER_HPMCOUNTER8:
        __asm__ __volatile__("csrr %0, hpmcounter8\n" : "=r"(val));
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

static inline void *TraceBuffer_Reserve(struct trace_control_block_t *cb, uint64_t size)
{
    void *head;

    if ((cb->offset_per_hart + size) > cb->threshold) 
    {    
        /* Flush buffer to L3 (dst = 2) */
        asm volatile("fence");
        evict(0x2ULL, cb->base_per_hart, cb->offset_per_hart);      
        __asm__ __volatile__ ( "csrwi tensor_wait, 6\n" : : );

        // TODO: Should we CacheOps wait?
        // TODO: Notify that we reached the notification threshold
        //       syscall(SYSCALL_TRACE_BUFFER_THRESHOLD_HIT);

        /* Reset the head pointer */
        head = (void *)cb->base_per_hart;
        cb->offset_per_hart = 0;
    } else {
        head = (void *)(cb->base_per_hart + cb->offset_per_hart);
        cb->offset_per_hart += size;
    }

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
*       uint64_t                Hart ID for which user needs Trace control block.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb, uint64_t hart_id)
{
    /* Check if Trace is enabled for current HART ID. */
    if (!((init_info->shire_mask & GET_SHIRE_MASK(hart_id)) && (init_info->thread_mask & GET_HART_MASK(hart_id))))
    {
        cb->enable = false;
        return;
    }

    /* Check if it is a shortcut to enable all Trace events and filters. */ 
    cb->filter_mask = (init_info->event_mask == TRACE_EVENT_ENABLE_ALL) ? 
                     TRACE_FILTER_ENABLE_ALL : init_info->filter_mask;

    cb->event_mask = init_info->event_mask;
    cb->offset_per_hart = 0;
    cb->threshold = init_info->threshold;
    cb->enable = true;
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
    if ((cb->enable == true) && (cb->event_mask & TRACE_EVENT_STRING) && (CHECK_STRING_FILTER(cb, log_level)))
    {
        struct trace_string_t *entry =
            TraceBuffer_Reserve(cb, sizeof(*entry));

        entry->header.type = TRACE_TYPE_STRING;
        entry->header.cycle = PMU_Get_hpmcounter3();
        strlcpy(entry->string, str, sizeof(entry->string));
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
    if ((cb->enable == true) && (cb->event_mask & TRACE_EVENT_STRING) && (CHECK_STRING_FILTER(cb, log_level)))
    {
        va_list args;
        struct trace_string_t *entry =
            TraceBuffer_Reserve(cb, sizeof(*entry));

        entry->header.type = TRACE_TYPE_STRING;
        entry->header.cycle = PMU_Get_hpmcounter3();

        va_start(args, format);
        vsnprintf(entry->string, sizeof(entry->string), format, args);
        va_end(args);
    }
}

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

void Trace_PMC_Counter(struct trace_control_block_t *cb, enum pmc_counter_e counter)
{
    if(cb->enable == true)
    {
        struct trace_pmc_counter_t *entry =
            TraceBuffer_Reserve(cb, sizeof(*entry));

        *entry = (struct trace_pmc_counter_t){
            .header.type = TRACE_TYPE_PMC_COUNTER,
            .header.cycle = PMU_Get_hpmcounter3(),
            .value = PMU_Get_Counter(counter)
        };
    }
}

#define Trace_Value_scalar_def(suffix, trace_type, c_type)                                    \
void Trace_Value_##suffix(struct trace_control_block_t *cb, uint64_t tag, c_type value)       \
{                                                                                             \
    if(cb->enable == true)                                                                    \
    {                                                                                         \
        struct trace_value_##suffix##_t *entry =                                              \
            TraceBuffer_Reserve(cb, sizeof(*entry));                                          \
                                                                                              \
        *entry = (struct trace_value_##suffix##_t){                                           \
            .header.type = trace_type,                                                        \
            .header.cycle = PMU_Get_hpmcounter3(),                                            \
            .tag = tag,                                                                       \
            .value = value                                                                    \
        };                                                                                    \
    }                                                                                         \
}

Trace_Value_scalar_def(u64, TRACE_TYPE_VALUE_U64, uint64_t)
Trace_Value_scalar_def(u32, TRACE_TYPE_VALUE_U32, uint32_t)
Trace_Value_scalar_def(u16, TRACE_TYPE_VALUE_U16, uint16_t)
Trace_Value_scalar_def(u8, TRACE_TYPE_VALUE_U8, uint8_t)
Trace_Value_scalar_def(float, TRACE_TYPE_VALUE_FLOAT, float)
