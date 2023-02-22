#include <stdint.h>

#include "mock_etsoc.h"

#include <et-trace/layout.h>

static uint64_t et_trace_get_counter(pmc_counter_e id)
{
    switch (id) {
    case PMC_COUNTER_HPMCOUNTER4:
        return reg_hpmcounter4;
    case PMC_COUNTER_HPMCOUNTER5:
        return reg_hpmcounter5;
    case PMC_COUNTER_HPMCOUNTER6:
        return reg_hpmcounter6;
    case PMC_COUNTER_HPMCOUNTER7:
        return reg_hpmcounter7;
    case PMC_COUNTER_HPMCOUNTER8:
        return reg_hpmcounter8;
    default:
        return 0;
    }
}

#ifdef MASTER_MINION
#define ET_TRACE_WITH_HART_ID
#endif

#define ET_TRACE_GET_TIMESTAMP()     reg_hpmcounter3
#define ET_TRACE_GET_TIMESTAMP_SAFE()     reg_hpmcounter3
#define ET_TRACE_GET_HPM_COUNTER(id) et_trace_get_counter(id)
#define ET_TRACE_GET_HPM_COUNTER_SAFE(id) et_trace_get_counter(id)
#define ET_TRACE_GET_HART_ID()       reg_mhartid

#define ET_TRACE_ENCODER_IMPL
#include <et-trace/encoder.h>

#define ET_TRACE_DECODER_IMPL
#include <et-trace/decoder.h>
