#include <stdint.h>

#include "mock_etsoc.h"

static uint64_t et_trace_get_counter(int id)
{
    switch (id) {
    case 3:
        return reg_hpmcounter3;
    case 4:
        return reg_hpmcounter4;
    case 5:
        return reg_hpmcounter5;
    case 6:
        return reg_hpmcounter6;
    case 7:
        return reg_hpmcounter7;
    case 8:
        return reg_hpmcounter8;
    default:
        return 0;
    }
}

#ifdef MASTER_MINION
#define ET_TRACE_MESSAGE_HEADER(msg, id)                                 \
    {                                                                    \
        ET_TRACE_WRITE(64, msg->header.mm.cycle, PMU_Get_hpmcounter3()); \
        ET_TRACE_WRITE(32, msg->header.mm.hart_id, get_hart_id());       \
        ET_TRACE_WRITE(16, msg->header.mm.type, id);                     \
    }
#endif

// Extranous semicolon because of reasons
#define ET_TRACE_GET_TIMESTAMP(out)       (out = reg_hpmcounter3);
#define ET_TRACE_GET_HPM_COUNTER(id, out) (out = et_trace_get_counter(id));

#define ET_TRACE_ENCODER_IMPL
#include <device-trace/et_trace.h>
