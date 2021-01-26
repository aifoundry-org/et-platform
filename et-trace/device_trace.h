#ifndef DEVICE_TRACE_H
#define DEVICE_TRACE_H

#include <stdint.h>
#include "device_trace_types.h"

/*
 * Global tracing information used to initialize per-thread information.
 */
struct trace_init_info_t {
    uint64_t buffer;
    uint64_t size_per_hart;
    uint64_t threshold;
};

/*
 * Per-thread tracing book-keeping information.
 */
struct trace_control_block_t {
    uint64_t base_per_hart;     /*!< Base address within struct trace_init_info_t::buffer for each hart */
    uint64_t size_per_hart;     /*!< Size of the chunk of struct trace_init_info_t::buffer for each hart */
    uint64_t offset_per_hart;   /*!< Head pointer within the chunk of struct trace_init_info_t::buffer for each hart */
    uint64_t threshold;
};

void Trace_Init(const struct trace_init_info_t *init_info, struct trace_control_block_t *cb,
                uint64_t hart_id);
void Trace_String(struct trace_control_block_t *cb, const char *str);
void Trace_Format_String(struct trace_control_block_t *cb, const char *format, ...);
void Trace_PMC_All_Counters(struct trace_control_block_t *cb);
void Trace_PMC_Counter(struct trace_control_block_t *cb, enum pmc_counter_e counter);
void Trace_Value_u64(struct trace_control_block_t *cb, uint64_t tag, uint64_t value);
void Trace_Value_u32(struct trace_control_block_t *cb, uint64_t tag, uint32_t value);
void Trace_Value_u16(struct trace_control_block_t *cb, uint64_t tag, uint16_t value);
void Trace_Value_u8(struct trace_control_block_t *cb, uint64_t tag, uint8_t value);
void Trace_Value_float(struct trace_control_block_t *cb, uint64_t tag, float value);

#endif
