#ifndef ET_TRACE_LAYOUT_H
#define ET_TRACE_LAYOUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Note: This header should be auto-generated

/*! \def TRACE_STRING_MAX_SIZE
    \brief Max size of string to log into Trace string message.
        Longer strings are truncated.
*/
#define TRACE_STRING_MAX_SIZE 64

/*! \def TRACE_MAGIC_HEADER
    \brief This is Trace header magic number. This is just temproray number as of now.
*/
#define TRACE_MAGIC_HEADER 0x76543210

/*! \typedef trace_buffer_type_e
    \brief Trace buffer type either MM, CM, SP etc.
*/
typedef uint8_t trace_buffer_type_e;

/*! \enum trace_buffer_type
    \brief Trace buffer type either MM, CM, or SP etc.
*/
enum trace_buffer_type { TRACE_MM_BUFFER, TRACE_CM_BUFFER, TRACE_SP_BUFFER, TRACE_CM_UMODE_BUFFER };

/*! \typedef trace_header_type_e
    \brief Trace buffer type either trace_buffer_std_header_t or trace_buffer_size_header_t.
*/
typedef uint8_t trace_header_type_e;

/*! \enum trace_header_type
    \brief Trace buffer header type.
*/
enum trace_header_type {
    TRACE_STD_HEADER, /**< Standard trace buffer header of type struct trace_buffer_std_header_t */
    TRACE_SIZE_HEADER /**< Size only trace buffer header of type struct trace_buffer_size_header_t */
};

/*! \typedef trace_type_e
    \brief Trace packet type.
*/
typedef uint16_t trace_type_e;

/*! \enum trace_type
    \brief Trace packet types.
*/
enum trace_type {
    TRACE_TYPE_STRING,
    TRACE_TYPE_PMC_COUNTER,
    TRACE_TYPE_PMC_ALL_COUNTERS,
    TRACE_TYPE_VALUE_U64,
    TRACE_TYPE_VALUE_U32,
    TRACE_TYPE_VALUE_U16,
    TRACE_TYPE_VALUE_U8,
    TRACE_TYPE_VALUE_FLOAT,
    TRACE_TYPE_MEMORY,
    TRACE_TYPE_EXCEPTION,
    TRACE_TYPE_CMD_STATUS,
    TRACE_TYPE_POWER_STATUS
};

/*! \typedef trace_cmd_status_e
    \brief Trace command statuses. Refers to enum trace_cmd_status.
*/
typedef uint8_t trace_cmd_status_e;

/*! \enum trace_cmd_status
    \brief Trace command statuses.
*/
enum trace_cmd_status {
    CMD_STATUS_WAIT_BARRIER = 1, /**< Conditional: Command is awaiting for Barrier to be released */
    CMD_STATUS_RECEIVED,         /**< Command is popped/received from submission Queue. */
    CMD_STATUS_EXECUTING, /**< Command is submitted for execution to respective component of device. */
    CMD_STATUS_FAILED,    /**< Command completed with a failure. */
    CMD_STATUS_ABORTED,   /**< Command is aborted. */
    CMD_STATUS_SUCCEEDED, /**< Command completed successfully. */
};

/*! \typedef pmc_counter_e
    \brief Counter type of log timestamp. Refers to enum pmc_counter.
*/
typedef uint8_t pmc_counter_e;

/*! \enum pmc_counter
    \brief Counter type of log timestamp.
*/
enum pmc_counter {
    PMC_COUNTER_HPMCOUNTER4,
    PMC_COUNTER_HPMCOUNTER5,
    PMC_COUNTER_HPMCOUNTER6,
    PMC_COUNTER_HPMCOUNTER7,
    PMC_COUNTER_HPMCOUNTER8,
    PMC_COUNTER_SHIRE_CACHE_FOO,
    PMC_COUNTER_MEMSHIRE_FOO
};

/*! \struct trace_buffer_size_header_t
    \brief Trace buffer header (resides at the beggining of the buffer).
           It contains size of valid data in that buffer.
           This header is used for sub-buffers when a buffer is partitioned into sub-bufers.
*/
struct trace_buffer_size_header_t {
    uint32_t data_size; /**< Data in the buffer. */
} __attribute__((packed));

/*! \struct trace_buffer_std_header_t
    \brief Trace buffer header (resides at the beggining of the buffer).
           It contains size of valid data, type of buffer and Esperanto magic
           number to validate buffer. It is used for once for the whole buffer
           not matter if it divided into sub-buffers or not
*/
struct trace_buffer_std_header_t {
    uint32_t magic_header; /**< Esperanto magic header. */
    uint32_t data_size;    /**< Data in the buffer. */
    uint16_t type;         /**< Buffer type one of enum trace_buffer_type_e. */
    uint8_t pad[6];        /**< Padding for Cache alignment. */
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief Information common for different entry types.
*/
struct trace_entry_header_t {
    uint64_t cycle;   /**< Current cycle */
    uint32_t hart_id; /**< (optional) Hart ID of the Hart which is logging Trace */
    uint16_t type;    /**< One of enum trace_type_e */
    uint8_t pad[2];   /**< Cache Alignment for MM as it uses atomic operations. */
} __attribute__((packed));

/*! \struct trace_pmc_counter_t
    \brief A Trace packet strucure for a single PMC counter.
*/
struct trace_pmc_counter_t {
    struct trace_entry_header_t header;
    uint64_t value;
    uint8_t counter; // One of enum pmc_counter_e
    uint8_t pad[7];
} __attribute__((packed));

/*! \struct trace_event_cmd_status_t
    \brief A Trace event strucure for a command execution status.
*/
struct trace_event_cmd_status_t {
    union {
        struct {
            uint16_t mesg_id; /**< Command message ID */
            trace_cmd_status_e
                cmd_status;        /**< Command execution status, One of enum trace_cmd_status */
            uint8_t queue_slot_id; /**< Submission Queue ID from which command is popped */
            uint16_t trans_id;     /**< transaction ID of command e.g. Tag ID. */
            uint8_t reserved[2];   /**< Reserved */
        } __attribute__((packed));
        uint64_t raw_cmd;
    };
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief A Trace packet strucure for a command execution status.
*/
struct trace_cmd_status_t {
    struct trace_entry_header_t header;
    struct trace_event_cmd_status_t cmd;
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief A Trace event strucure for a power status.
*/
struct trace_event_power_status_t {
    union {
        struct {
            uint8_t throttle_state; /**< Power Throttle State: UP, Down */
            uint8_t power_state;    /**< Power State: Max, Managed, Safe, Low */
            uint8_t current_power;  /**< Current Power in mW */
            uint8_t current_temp;   /**< Current Temperature in C */
            uint16_t tgt_freq;      /**< Target Frequency in Mhz */
            uint16_t tgt_voltage;   /**< Target Voltage in mV */
        } __attribute__((packed));
        uint64_t raw_cmd;
    };
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief A Trace packet strucure for a power status.
*/
struct trace_power_status_t {
    struct trace_entry_header_t header;
    struct trace_event_power_status_t power;
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief A Trace packet strucure for a string message.
*/
struct trace_string_t {
    struct trace_entry_header_t header;
    char string[TRACE_STRING_MAX_SIZE];
} __attribute__((packed));

/*! \struct trace_entry_header_t
    \brief A Trace packet strucure for a memory dump of variable size.
*/
struct trace_memory_t {
    struct trace_entry_header_t header;
    uint64_t src_addr;
    uint64_t size;
    uint8_t data[];
} __attribute__((packed));

#define TRACE_SCALAR_TYPE_DEF(suffix, c_type, pad_count) \
    struct trace_value_##suffix##_t {                    \
        struct trace_entry_header_t header;              \
        uint32_t tag;                                    \
        c_type value;                                    \
        uint8_t pad[pad_count];                          \
    } __attribute__((packed));

TRACE_SCALAR_TYPE_DEF(u64, uint64_t, 4)
TRACE_SCALAR_TYPE_DEF(u32, uint32_t, 0)
TRACE_SCALAR_TYPE_DEF(u16, uint16_t, 2)
TRACE_SCALAR_TYPE_DEF(u8, uint8_t, 3)
TRACE_SCALAR_TYPE_DEF(float, float, 0)

#ifdef __cplusplus
}
#endif

#endif /* ET_TRACE_LAYOUT_H */
