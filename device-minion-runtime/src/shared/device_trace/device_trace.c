#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "device_trace.h"

static inline void flush_va(uint64_t dst, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((dst       & 0x3                   ) << 58 ) |
                       ((addr      & 0xFFFFFFFFFFC0ULL     )       ) |
                       ((num_lines & 0xF                   )       ) ;
    uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__(
        "fence\n"
        "li   x31, %[x31_enc]\n"
        "csrw flush_va, %[csr_enc]\n"
        :
        : [x31_enc] "i" (x31_enc),
          [csr_enc] "r" (csr_enc)
        : "x31"
    );
}

static inline void flush_va_range(uint64_t dst, uint64_t addr, uint64_t size, uint64_t id)
{
    flush_va(dst, addr, size / 64, 64, id);
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

    if (cb->offset_per_hart + size > cb->threshold) {
        /* Flush buffer to L3 (dst = 2), id = 0 */
        flush_va_range(2, cb->base_per_hart, cb->offset_per_hart, 0);

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

void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
                uint64_t hart_id)
{
    cb->base_per_hart = init_info->buffer + (hart_id * init_info->size_per_hart);
    cb->size_per_hart = init_info->size_per_hart;
    cb->offset_per_hart = 0;
    cb->threshold = init_info->threshold;
}

void Trace_String(struct trace_control_block_t *cb, const char *str)
{
    struct trace_string_t *entry =
        TraceBuffer_Reserve(cb, sizeof(*entry));

    entry->header.type = TRACE_TYPE_STRING;
    entry->header.cycle = PMU_Get_hpmcounter3();
    strlcpy(entry->string, str, sizeof(entry->string));
}

void Trace_Format_String(struct trace_control_block_t *cb, const char *format, ...)
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

void Trace_PMC_All_Counters(struct trace_control_block_t *cb)
{
    for (enum pmc_counter_e counter = PMC_COUNTER_HPMCOUNTER4;
         counter <= PMC_COUNTER_MEMSHIRE_FOO;
         counter++) {
        Trace_PMC_Counter(cb, counter);
    }
}

void Trace_PMC_Counter(struct trace_control_block_t *cb, enum pmc_counter_e counter)
{
    struct trace_pmc_counter_t *entry =
        TraceBuffer_Reserve(cb, sizeof(*entry));

    *entry = (struct trace_pmc_counter_t){
        .header.type = TRACE_TYPE_PMC_COUNTER,
        .header.cycle = PMU_Get_hpmcounter3(),
        .value = PMU_Get_Counter(counter)
    };
}

#define Trace_Value_scalar_def(suffix, trace_type, c_type)                                    \
void Trace_Value_##suffix(struct trace_control_block_t *cb, uint64_t tag, c_type value)       \
{                                                                                             \
    struct trace_value_##suffix##_t *entry =                                                  \
        TraceBuffer_Reserve(cb, sizeof(*entry));                                              \
                                                                                              \
    *entry = (struct trace_value_##suffix##_t){                                               \
        .header.type = trace_type,                                                            \
        .header.cycle = PMU_Get_hpmcounter3(),                                                \
        .tag = tag,                                                                           \
        .value = value                                                                        \
    };                                                                                        \
}

Trace_Value_scalar_def(u64, TRACE_TYPE_VALUE_U64, uint64_t)
Trace_Value_scalar_def(u32, TRACE_TYPE_VALUE_U32, uint32_t)
Trace_Value_scalar_def(u16, TRACE_TYPE_VALUE_U16, uint16_t)
Trace_Value_scalar_def(u8, TRACE_TYPE_VALUE_U8, uint8_t)
Trace_Value_scalar_def(float, TRACE_TYPE_VALUE_FLOAT, float)
