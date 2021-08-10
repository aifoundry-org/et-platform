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
/*! \file device_trace_access_primitives_mm.h
    \brief A C header that implements trace memory access primitives for
           Master Minion. For Master Minion all Trace data is based on L2
           Cache, that means we need to perform all updates in L2.
           On other hand, for Service Processor and Compute Minion, Trace
           data is in L1 Cache, and updates are done by normal load/store
           operations. So, all memory access operations are Atomic
           operations in L2 Cache
*/
/***********************************************************************/

#ifndef TRACE_MM_PRIMITIVES_H
#define TRACE_MM_PRIMITIVES_H

#include "device-common/atomic.h"
#include "etsoc_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

union data_u32_f
{
    uint32_t value_u32;
    float value_f;
};

#ifdef __cplusplus
}
#endif

#define ET_TRACE_READ(size, addr)            atomic_load_local_##size(&addr)
#define ET_TRACE_WRITE(size, addr, value)    atomic_store_local_##size(&addr, value)

/* TODO: Use atomic float store, when its implementation is available. */
#define ET_TRACE_WRITE_F(addr, value)       ({ union data_u32_f data; data.value_f = value;                  \
                                            atomic_store_local_32((void *)&addr, data.value_u32);})
#define ET_TRACE_MEM_CPY(dest, src, size)   ETSOC_Memory_Write_Local_Atomic(src, dest, size)

#define ET_TRACE_MESSAGE_HEADER(msg, id)    {ET_TRACE_WRITE(64, msg->header.mm.cycle, PMU_Get_hpmcounter3()); \
                                            ET_TRACE_WRITE(32, msg->header.mm.hart_id, get_hart_id());        \
                                            ET_TRACE_WRITE(16, msg->header.mm.type, id);}

/* Get PC counter value from counter register using RISCV ISA. */
#define ET_TRACE_GET_TIMESTAMP(ret_val)         {__asm__ __volatile__("csrr %0, hpmcounter3\n" : "=r"(ret_val));}
#define ET_TRACE_GET_HPM_COUNTER(counter_index, ret_val) {__asm__ __volatile__("csrr %0, hpmcounter"#counter_index"\n" : "=r"(ret_val));}


#endif