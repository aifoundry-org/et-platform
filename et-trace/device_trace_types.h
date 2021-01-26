#ifndef DEVICE_TRACE_TYPES_H
#define DEVICE_TRACE_TYPES_H

#include <stdint.h>

// Note: This header should be auto-generated

#define TRACE_STRING_MAX_SIZE 64

enum trace_type_e {
    TRACE_TYPE_STRING,
    TRACE_TYPE_PMC_COUNTER,
    TRACE_TYPE_PMC_ALL_COUNTERS,
    TRACE_TYPE_VALUE_U64,
    TRACE_TYPE_VALUE_U32,
    TRACE_TYPE_VALUE_U16,
    TRACE_TYPE_VALUE_U8,
    TRACE_TYPE_VALUE_FLOAT
};

enum pmc_counter_e {
    PMC_COUNTER_HPMCOUNTER4,
    PMC_COUNTER_HPMCOUNTER5,
    PMC_COUNTER_HPMCOUNTER6,
    PMC_COUNTER_HPMCOUNTER7,
    PMC_COUNTER_HPMCOUNTER8,
    PMC_COUNTER_SHIRE_CACHE_FOO,
    PMC_COUNTER_MEMSHIRE_FOO
};

struct trace_entry_header_t {
    uint8_t type;   // One of enum trace_type_e
    uint64_t cycle; // Current cycle
} __attribute__((packed));

struct trace_pmc_counter_t {
    struct trace_entry_header_t header;
    uint8_t counter; // One of enum pmc_counter_e
    uint64_t value;
} __attribute__((packed));

struct trace_string_t {
    struct trace_entry_header_t header;
    char string[TRACE_STRING_MAX_SIZE];
} __attribute__((packed));

#define trace_scalar_type_def(suffix, c_type) \
struct trace_value_##suffix##_t {             \
    struct trace_entry_header_t header;       \
    uint64_t tag;                             \
    c_type value;                             \
} __attribute__((packed));

trace_scalar_type_def(u64, uint64_t)
trace_scalar_type_def(u32, uint32_t)
trace_scalar_type_def(u16, uint16_t)
trace_scalar_type_def(u8, uint8_t)
trace_scalar_type_def(float, float)

#endif
