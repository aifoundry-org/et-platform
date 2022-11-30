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
/*! \file trace_umode.h
    \brief A C header that implements the Trace services for CM UMode.
*/
/***********************************************************************/

#ifndef TRACE_UMODE_H
#define TRACE_UMODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "trace/trace_umode_cb.h"
#include "etsoc/isa/hart.h"

/*! \def et_trace_pmc_compute(hart_id)
    \brief A macro used to dump Minion and Neighborhood PMCs in trace buffer.
*/
#define et_trace_pmc_compute(hart_id)                                                     \
    {                                                                                     \
        uint32_t cur_hart_id = get_hart_id();                                             \
        if (hart_id == cur_hart_id)                                                       \
        {                                                                                 \
            Trace_PMC_Counters_Compute(&CM_UMODE_TRACE_CB[GET_CB_INDEX(cur_hart_id)].cb); \
        }                                                                                 \
    }

/*! \def et_trace_pmc_sc(hart_id)
    \brief A macro used to dump Shire-Cache PMCs in trace buffer.
    \warning Must only we called from a single hart in a kernel.
*/
#define et_trace_pmc_sc(hart_id)                                                     \
    {                                                                                \
        uint32_t cur_hart_id = get_hart_id();                                        \
        if (hart_id == cur_hart_id)                                                  \
        {                                                                            \
            Trace_PMC_Counters_SC(&CM_UMODE_TRACE_CB[GET_CB_INDEX(cur_hart_id)].cb); \
        }                                                                            \
    }

/*! \def et_trace_pmc_ms(hart_id, ms_id)
    \brief A macro used to dump Mem-Shire PMCs in trace buffer.
    \warning Must only we called from a single hart in a kernel.
*/
#define et_trace_pmc_ms(hart_id, ms_id)                                                     \
    {                                                                                       \
        uint32_t cur_hart_id = get_hart_id();                                               \
        if (hart_id == cur_hart_id)                                                         \
        {                                                                                   \
            Trace_PMC_Counters_MS(&CM_UMODE_TRACE_CB[GET_CB_INDEX(cur_hart_id)].cb, ms_id); \
        }                                                                                   \
    }

/*! \def et_trace_memory(src_ptr, size)
    \brief A macro used to dump the number of data bytes from the source address provided.
*/
#define et_trace_memory(src_ptr, size)                                                   \
    {                                                                                    \
        Trace_Memory(&CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb, src_ptr, size); \
    }

/*! \def et_trace_register()
    \brief A macro used to log file, line and funtion info along with GPRs dump to the trace buffer.
*/
#define et_trace_register()                                                                    \
    {                                                                                          \
        et_printf("Stack dump in file \"%s\", line %d%s%s\n", __FILE__, __LINE__,              \
            __FUNCTION__ ? ", function: " : "", __FUNCTION__ ? __FUNCTION__ : "");             \
        struct dev_context_registers_t *regs;                                                  \
        asm volatile(                                                                          \
            "addi  sp, sp, -(35 * 8)   \n" /* Make space for trace context registers struct */ \
            "sd    x0,  0  * 8( sp )   \n" /* Zeroize the top 4 values */                      \
            "sd    x0,  1  * 8( sp )   \n"                                                     \
            "sd    x0,  2  * 8( sp )   \n"                                                     \
            "sd    x0,  3  * 8( sp )   \n"                                                     \
            "sd    x1,  4  * 8( sp )   \n" /* Start dumping GPRs */                            \
            "sd    x2,  5  * 8( sp )   \n"                                                     \
            "sd    x3,  6  * 8( sp )   \n"                                                     \
            "sd    x4,  7  * 8( sp )   \n"                                                     \
            "sd    x5,  8  * 8( sp )   \n"                                                     \
            "sd    x6,  9  * 8( sp )   \n"                                                     \
            "sd    x7,  10 * 8( sp )   \n"                                                     \
            "sd    x8,  11 * 8( sp )   \n"                                                     \
            "sd    x9,  12 * 8( sp )   \n"                                                     \
            "sd    x10, 13 * 8( sp )   \n"                                                     \
            "sd    x11, 14 * 8( sp )   \n"                                                     \
            "sd    x12, 15 * 8( sp )   \n"                                                     \
            "sd    x13, 16 * 8( sp )   \n"                                                     \
            "sd    x14, 17 * 8( sp )   \n"                                                     \
            "sd    x15, 18 * 8( sp )   \n"                                                     \
            "sd    x16, 19 * 8( sp )   \n"                                                     \
            "sd    x17, 20 * 8( sp )   \n"                                                     \
            "sd    x18, 21 * 8( sp )   \n"                                                     \
            "sd    x19, 22 * 8( sp )   \n"                                                     \
            "sd    x20, 23 * 8( sp )   \n"                                                     \
            "sd    x21, 24 * 8( sp )   \n"                                                     \
            "sd    x22, 25 * 8( sp )   \n"                                                     \
            "sd    x23, 26 * 8( sp )   \n"                                                     \
            "sd    x24, 27 * 8( sp )   \n"                                                     \
            "sd    x25, 28 * 8( sp )   \n"                                                     \
            "sd    x26, 39 * 8( sp )   \n"                                                     \
            "sd    x27, 30 * 8( sp )   \n"                                                     \
            "sd    x28, 31 * 8( sp )   \n"                                                     \
            "sd    x29, 32 * 8( sp )   \n"                                                     \
            "sd    x30, 33 * 8( sp )   \n"                                                     \
            "sd    x31, 34 * 8( sp )   \n"                                                     \
            "mv    %[regs], sp         \n"                                                     \
            : [regs] "=r"(regs));                                                              \
                                                                                               \
        Trace_Execution_Stack(&CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb, regs);       \
                                                                                               \
        asm volatile("addi  sp, sp, (35 * 8)\n"); /* Restore SP */                             \
    }

/*! \fn void et_trace_flush_buffer(void)
    \brief This function is used to flush the trace buffer of the calling hart.
    \return None
*/
void et_trace_flush_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* TRACE_UMODE_H */