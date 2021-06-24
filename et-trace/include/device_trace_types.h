#ifndef DEVICE_TRACE_TYPES_H
#define DEVICE_TRACE_TYPES_H

#include <stdint.h>

// Note: This header should be auto-generated

/*! \def TRACE_STRING_MAX_SIZE
    \brief Max size of string to log into Trace string message.
        Longer strings are truncated.
*/
#define TRACE_STRING_MAX_SIZE 64

/*! \def TRACE_MAGIC_HEADER
    \brief This is Trace header magic number. This is just temproray number as of now.
*/
#define TRACE_MAGIC_HEADER        0x76543210

/*! \enum trace_buffer_type_e
    \brief Trace buffer type either MM, CM, or SP.
*/
enum trace_buffer_type_e {
    TRACE_MM_BUFFER,
    TRACE_CM_BUFFER,
    TRACE_SP_BUFFER
};

/*! \typedef trace_header_type_e
    \brief Trace buffer type either trace_buffer_std_header_t or trace_buffer_size_header_t.
*/
typedef uint8_t trace_header_type_e;

/*! \trace_header_type_e
    \brief Trace buffer header type.
*/
enum trace_header_type {
    TRACE_STD_HEADER,   /**< Standard trace buffer header of type struct trace_buffer_std_header_t */
    TRACE_SIZE_HEADER   /**< Size only trace buffer header of type struct trace_buffer_size_header_t */
};

enum trace_type_e {
    TRACE_TYPE_STRING,
    TRACE_TYPE_PMC_COUNTER,
    TRACE_TYPE_PMC_ALL_COUNTERS,
    TRACE_TYPE_VALUE_U64,
    TRACE_TYPE_VALUE_U32,
    TRACE_TYPE_VALUE_U16,
    TRACE_TYPE_VALUE_U8,
    TRACE_TYPE_VALUE_FLOAT,
    TRACE_TYPE_MEMORY,
    TRACE_TYPE_EXCEPTION
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

struct trace_buffer_size_header_t {
    uint32_t data_size;     /**< Data in the buffer. */
} __attribute__((packed));

struct trace_buffer_std_header_t {
    uint32_t magic_header;  /**< Esperanto magic header. */
    uint32_t data_size;     /**< Data in the buffer. */
    uint16_t type;          /**< Buffer type one of enum trace_buffer_type_e. */
    uint8_t  pad[6];        /**< Padding for Cache alignment. */
} __attribute__((packed));

struct trace_entry_header_t {
    uint64_t cycle;   /**< Current cycle */
#if defined(MASTER_MINION)
    uint32_t hart_id; /**< Hart ID of the Hart which is logging Trace */
    uint16_t type;    /**< One of enum trace_type_e */
    uint8_t  pad[2];  /**< Cache Alignment for MM as it used atomic operations. */
#else
    uint16_t type;    /**< One of enum trace_type_e */
#endif
} __attribute__((packed));

struct trace_pmc_counter_t {
    struct trace_entry_header_t header;
    uint64_t value;
    uint8_t  counter; // One of enum pmc_counter_e
    uint8_t  pad[7];
} __attribute__((packed));

struct trace_string_t {
    struct trace_entry_header_t header;
    char string[TRACE_STRING_MAX_SIZE];
} __attribute__((packed));

struct trace_memory_t {
    struct trace_entry_header_t header;
    uint64_t src_addr;
    uint64_t size;
    uint8_t data[];
} __attribute__((packed));

#define trace_scalar_type_def(suffix, c_type, pad_count)    \
struct trace_value_##suffix##_t {                           \
    struct trace_entry_header_t header;                     \
    uint32_t tag;                                           \
    c_type value;                                           \
    uint8_t pad[pad_count];                                 \
} __attribute__((packed));

trace_scalar_type_def(u64, uint64_t, 4)
trace_scalar_type_def(u32, uint32_t, 0)
trace_scalar_type_def(u16, uint16_t, 2)
trace_scalar_type_def(u8, uint8_t, 3)
trace_scalar_type_def(float, float, 0)

#endif
