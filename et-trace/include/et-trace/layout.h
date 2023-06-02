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
 * et-trace/layout.h
 * Describes the basic layout of Esperanto device traces.
 *
 *
 * USAGE
 *
 * Simply include the header file:
 *
 *     #include <et-trace/layout.h>
 *
 * This file only defines data structures and constants.
 *
 *
 * ABOUT
 *
 * From a high-level the layout of a trace buffer is as follows:
 *
 *   +--------------+   +--------+---------+   +--------+---------+
 *   | Trace Header | - | Header | Payload | - | Header | Payload | - ...
 *   +--------------+   +--------+---------+   +--------+---------+
 *                           Entry #1               Entry #2
 *
 * The trace buffer header contains general information about the
 * contents of the buffer. This includes:
 *  - the type of trace buffer (Master Minion, Compute Minion, Service Processor),
 *  - the number of valid bytes in the buffer (i.e.: size), and
 *  - a magic header to identify the validity of the trace.
 *
 * Entries have a variable size, depending on the type of their payload.
 * Each entry consists of a header with common fields, such as:
 *  - the type of payload (string, power_status, ...),
 *  - the timestamp this entry was collected, and
 *  - the hart_id, if applicable.
 *
 * The format of the payload depends on the type of data allocated.
 *
 *
 * NOTES
 *
 *  - All data structures here are defined as packed
 *  - In a few instances extra padding bytes are added to account
 *    for cache alignment / atomic operations
 *
 ***********************************************************************/


#ifndef ET_TRACE_LAYOUT_H
#define ET_TRACE_LAYOUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \def TRACE_STRING_MAX_SIZE
    \brief Max size of string to log into Trace string message.
           Longer strings will not be logged.
    TODO: Make this user configurable option, after
          updating Trace users to remove dependacy on it.
*/
#define TRACE_STRING_MAX_SIZE 512

/*! \def TRACE_STRING_SIZE_ALIGN
    \brief Trace string message alignment. This will keep string packet
           cache line (i.e. 8 bytes) aligned.
*/
#define TRACE_STRING_SIZE_ALIGN(size)     ((((size) + 7) / 8) * 8)

/*! \def TRACE_MAGIC_HEADER
    \brief This is Trace header magic number. This is just temproray number as of now.
*/
#define TRACE_MAGIC_HEADER 0x76543210

/*! \def TRACE_VERSION_MAJOR
    \brief This is Trace layout version (major).
*/
#define TRACE_VERSION_MAJOR 0

/*! \def TRACE_VERSION_MINOR
    \brief This is Trace layout version (minor).
*/
#define TRACE_VERSION_MINOR 6

/*! \def TRACE_VERSION_PATCH
    \brief This is Trace layout version (patch).
*/
#define TRACE_VERSION_PATCH 0

/*! \def TRACE_DEV_CONTEXT_GPRS
    \brief Macro that represents the total number of GPRs in device context.
*/
#define TRACE_DEV_CONTEXT_GPRS 31

/*! \typedef trace_buffer_type_e
    \brief Trace buffer type either MM, CM, SP etc.
*/
typedef uint8_t trace_buffer_type_e;

/*! \enum trace_buffer_type
    \brief Trace buffer type either MM, CM, or SP etc.
*/
enum trace_buffer_type { TRACE_MM_BUFFER, TRACE_CM_BUFFER, TRACE_SP_BUFFER, TRACE_CM_UMODE_BUFFER, TRACE_SP_STATS_BUFFER, TRACE_MM_STATS_BUFFER};

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
    TRACE_TYPE_START,
    TRACE_TYPE_STRING = TRACE_TYPE_START,
    TRACE_TYPE_PMC_COUNTER,
    TRACE_TYPE_PMC_COUNTERS_COMPUTE,
    TRACE_TYPE_PMC_COUNTERS_MEMORY,
    TRACE_TYPE_PMC_COUNTERS_SC,
    TRACE_TYPE_PMC_COUNTERS_MS,
    TRACE_TYPE_VALUE_U64,
    TRACE_TYPE_VALUE_U32,
    TRACE_TYPE_VALUE_U16,
    TRACE_TYPE_VALUE_U8,
    TRACE_TYPE_VALUE_FLOAT,
    TRACE_TYPE_MEMORY,
    TRACE_TYPE_EXCEPTION,
    TRACE_TYPE_CMD_STATUS,
    TRACE_TYPE_POWER_STATUS,
    TRACE_TYPE_CUSTOM_EVENT,
    TRACE_TYPE_USER_PROFILE_EVENT,
    TRACE_TYPE_END
};

/*! \enum trace_custom_type_sp
    \brief Trace custom type IDs for SP.
*/
enum trace_custom_type_sp {
    TRACE_CUSTOM_TYPE_SP_START = 0,
    TRACE_CUSTOM_TYPE_SP_PERF_GLOBALS,
    TRACE_CUSTOM_TYPE_SP_POWER_GLOBALS,
    TRACE_CUSTOM_TYPE_SP_POWER_STATES_GLOBALS,
    TRACE_CUSTOM_TYPE_SP_OP_STATS,
    TRACE_CUSTOM_TYPE_SP_COUNT,
    TRACE_CUSTOM_TYPE_SP_END = 999
};

/*! \enum trace_custom_type_mm
    \brief Trace custom type IDs for MM.
*/
enum trace_custom_type_mm {
    TRACE_CUSTOM_TYPE_MM_START = 1000,
    TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES,
    TRACE_CUSTOM_TYPE_MM_COUNT,
    TRACE_CUSTOM_TYPE_MM_END = 1999
};

/*! \enum trace_custom_type_cm
    \brief Trace custom type IDs for CM.
*/
enum trace_custom_type_cm {
    TRACE_CUSTOM_TYPE_CM_START = 2000,
    TRACE_CUSTOM_TYPE_CM_COUNT,
    TRACE_CUSTOM_TYPE_CM_END = 2999
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
    \brief PMC counters to sample and log in trace.
*/
enum pmc_counter {
    /* Minion and Neighborhood PMCs start. */
    PMC_COUNTER_HPMCOUNTER3 = 0,
    PMC_COUNTER_HPMCOUNTER4,
    PMC_COUNTER_HPMCOUNTER5,
    PMC_COUNTER_HPMCOUNTER6,
    PMC_COUNTER_HPMCOUNTER7,
    PMC_COUNTER_HPMCOUNTER8,
    /* PMCs that are accessible only in priviledged mode */
    /* Base of shire-cache PMCs */
    PMC_COUNTER_SHIRE_CACHE_CYCLE,
    PMC_COUNTER_SHIRE_CACHE_1,
    PMC_COUNTER_SHIRE_CACHE_2,
    /* Deprecated Defines */
    PMC_COUNTER_MEMSHIRE_CYCLE,
    PMC_COUNTER_MEMSHIRE_1,
    PMC_COUNTER_MEMSHIRE_2,
    /* Base of mem-shire PMCs */
    PMC_COUNTER_MEMSHIRE0_CYCLE,
    PMC_COUNTER_MEMSHIRE1_CYCLE,
    PMC_COUNTER_MEMSHIRE2_CYCLE,
    PMC_COUNTER_MEMSHIRE3_CYCLE,
    PMC_COUNTER_MEMSHIRE4_CYCLE,
    PMC_COUNTER_MEMSHIRE5_CYCLE,
    PMC_COUNTER_MEMSHIRE6_CYCLE,
    PMC_COUNTER_MEMSHIRE7_CYCLE,
    PMC_COUNTER_MEMSHIRE0_1,
    PMC_COUNTER_MEMSHIRE1_1,
    PMC_COUNTER_MEMSHIRE2_1,
    PMC_COUNTER_MEMSHIRE3_1,
    PMC_COUNTER_MEMSHIRE4_1,
    PMC_COUNTER_MEMSHIRE5_1,
    PMC_COUNTER_MEMSHIRE6_1,
    PMC_COUNTER_MEMSHIRE7_1,
    PMC_COUNTER_MEMSHIRE0_2,
    PMC_COUNTER_MEMSHIRE1_2,
    PMC_COUNTER_MEMSHIRE2_2,
    PMC_COUNTER_MEMSHIRE3_2,
    PMC_COUNTER_MEMSHIRE4_2,
    PMC_COUNTER_MEMSHIRE5_2,
    PMC_COUNTER_MEMSHIRE6_2,
    PMC_COUNTER_MEMSHIRE7_2
};

/*
 * Trace Layout version.
 */
struct trace_version_t {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} __attribute__((packed));

/*
 * Global tracing information used to initialize Trace.
 */
struct trace_init_info_t {
    uint64_t buffer;      /*!< Base address for Trace buffer. */
    uint32_t buffer_size; /*!< Total size of the Trace buffer. */
    uint32_t threshold;   /*!< Threshold for free memory in the buffer for each hart. */
    uint64_t shire_mask;  /*!< Bit Mask of Shire to enable Trace Capture. */
    uint64_t thread_mask; /*!< Bit Mask of Thread within a Shire to enable Trace Capture. */
    uint32_t
        event_mask; /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
    uint32_t
        filter_mask; /*!< This is a bit mask representing a list of filters for a given event to trace. */
};

/*
 * Global tracing information used to configure the Trace.
 */
struct trace_config_info_t {
    uint32_t
        event_mask; /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
    uint32_t
        filter_mask; /*!< This is a bit mask representing a list of filters for a given event to trace. */
    uint32_t threshold;   /*!< Threshold for free memory in the buffer for each hart. */
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
           Note: This structure must be cache-line aligned, i.e, 64-bytes aligned.
*/
struct trace_buffer_std_header_t {
    uint32_t magic_header;          /**< Esperanto magic header. */
    struct trace_version_t version; /**< Trace Layout version Major, Minor, and Patch */
    uint16_t type;                  /**< Buffer type one of enum trace_buffer_type_e. */
    uint32_t data_size;             /**< Data size after trace buffer standard header. It does not include data
                                         in sub-buffers (if present) */
    uint32_t sub_buffer_size;       /**< Size of one partition on Trace buffer, All partitions must be equal in size. */
    uint16_t sub_buffer_count;      /**< Number of Trace buffer partitions. */
} __attribute__((packed, aligned(64)));

/*! \struct trace_entry_header_t
    \brief Information common for different entry types.
*/
struct trace_entry_header_t {
    uint64_t cycle;   /**< Current cycle */
    uint32_t payload_size; /**< Size of the event payload following the entry header */
    uint16_t hart_id; /**< (optional) Hart ID of the Hart which is logging Trace */
    trace_type_e type; /**< One of enum trace_type */
} __attribute__((packed));

/*! \struct trace_pmc_counters_compute_t
    \brief A Trace packet strucure for all compute PMC counters.
*/
struct trace_pmc_counters_compute_t {
    struct trace_entry_header_t header;
    uint64_t hpmcounter3;
    uint64_t hpmcounter4;
    uint64_t hpmcounter5;
    uint64_t hpmcounter6;
    uint64_t hpmcounter7;
    uint64_t hpmcounter8;
} __attribute__((packed));

/*! \struct trace_pmc_counters_sc_t
    \brief A Trace packet strucure for all shire cache PMC counters.
*/
struct trace_pmc_counters_sc_t {
    struct trace_entry_header_t header;
    uint64_t sc_pmc0;
    uint64_t sc_pmc1;
} __attribute__((packed));

/*! \struct trace_pmc_counters_ms_t
    \brief A Trace packet strucure for all mem shire PMC counters.
*/
struct trace_pmc_counters_ms_t {
    struct trace_entry_header_t header;
    uint64_t ms_pmc0;
    uint64_t ms_pmc1;
    uint8_t ms_id;
    uint8_t pad[7];
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
            uint16_t current_power; /**< Current Power in mW */
            uint8_t throttle_state; /**< Power Throttle State: UP, Down */
            uint8_t power_state;    /**< Power State: Max, Managed, Safe, Low */
            uint8_t current_temp;   /**< Current Temperature in C */
            uint8_t pad_1[3];
        } __attribute__((packed));
        uint64_t raw_bits_64;
    };
    union {
        struct {
            uint16_t tgt_freq;    /**< Target Frequency in Mhz */
            uint16_t tgt_voltage; /**< Target Voltage in mV */
        } __attribute__((packed));
        uint32_t raw_bits_32;
    };
    uint8_t pad_2[4];
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
    char string[];
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

/*! \struct dev_context_registers_t
    \brief A structure containing the device context registers.
*/
struct dev_context_registers_t {
    uint64_t epc;
    uint64_t tval;
    uint64_t status;
    uint64_t cause;
    uint64_t gpr[TRACE_DEV_CONTEXT_GPRS]; /* x1 to x31 */
} __attribute__((packed));

/*! \struct trace_execution_stack_t
    \brief A Trace packet strucure for logging the execution stack of device registers.
*/
struct trace_execution_stack_t {
    struct trace_entry_header_t header;
    struct dev_context_registers_t registers;
} __attribute__((packed));

struct op_value
{
    uint16_t avg;
    uint16_t min;
    uint16_t max;
} __attribute__((packed, aligned(8)));

struct op_module
{
    struct op_value temperature;
    struct op_value power;
} __attribute__((packed, aligned(8)));

/*!
 * @struct struct sp_op_point_stats_
 * @brief Structure to collect SP operating point stats
 */
struct op_stats_t
{
    struct op_module minion;
    struct op_module sram;
    struct op_module noc;
    struct op_module system;
} __attribute__((packed, aligned(8)));

/*! \struct resource_value
    \brief Device statistics sample structure
*/
struct resource_value
{
  uint64_t avg;
  uint64_t min;
  uint64_t max;
} __attribute__((packed, aligned(8)));

/*! \struct compute_resources_sample
    \brief Device computer resource structure
*/
struct compute_resources_sample
{
  struct resource_value cm_utilization;
  struct resource_value cm_bw;
  struct resource_value pcie_dma_read_bw;
  struct resource_value pcie_dma_read_utilization;
  struct resource_value pcie_dma_write_bw;
  struct resource_value pcie_dma_write_utilization;
  struct resource_value ddr_read_bw;
  struct resource_value ddr_write_bw;
  struct resource_value l2_l3_read_bw;
  struct resource_value l2_l3_write_bw;
} __attribute__((packed, aligned(8)));

/*! \struct trace_custom_event_t
    \brief A Trace packet strucure for logging a custom event in trace buffer.
*/
struct trace_custom_event_t {
    struct trace_entry_header_t header;
    uint32_t custom_type;
    uint32_t payload_size;
    uint8_t payload[]; /* flexible array */
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

/*! \struct trace_user_profile_event_t
    \brief A Trace pkt structure for profile events.
*/
struct trace_user_profile_event_t {
    struct trace_entry_header_t header; /**< generic event header */
    uint64_t func;                      /**< ptr to func literal emitting the event */
    uint64_t retiredInsts;              /**< retiredInsts when the event is emitted */
    uint64_t line_region_status;        /**< line(4 bytes) region (2 bytes) status (2 bytes)*/
    uint64_t regionName;                /**< ptr to user region name */
} __attribute__((packed));

#ifdef __cplusplus
}
#endif

#endif /* ET_TRACE_LAYOUT_H */
