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
/*! \file cm_mm_defines.h
    \brief A C header that defines common configurations for MM and CM.
*/
/***********************************************************************/

#ifndef CM_TO_MM_DEFS_H
#define CM_TO_MM_DEFS_H

#include <etsoc/isa/sync.h>
#include <stdio.h>

/*! \def CM_MM_MASTER_HART_DISPATCHER_IDX
    \brief A macro that provides the index of the hart within master shire
    used for dispatcher.
*/
#define CM_MM_MASTER_HART_DISPATCHER_IDX 0U

/*! \def CM_MM_MASTER_HART_UNICAST_BUFF_IDX
    \brief A macro that provides the index of the unicast buffer associated
    with Master HART within the master shire.
*/
#define CM_MM_MASTER_HART_UNICAST_BUFF_IDX 0U

/*! \def CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX
    \brief A macro that provides the base index of the unicast buffer
    associated with Kernel Workers.
*/
#define CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX 1U

/* Error Codes */
/*! \def CM_ERROR_KERNEL_RETURN
    \brief A macro that provides the error code for compute minion kernel return error
*/
#define CM_ERROR_KERNEL_RETURN -1

/*! \def CM_ERROR_KERNEL_LAUNCH
    \brief A macro that provides the error code for compute minion kernel launch error
*/
#define CM_ERROR_KERNEL_LAUNCH -2

/*! \def CM_DEFAULT_TRACE_THREAD_MASK
    \brief Default masks to enable Trace for all hart in CM Shires.
*/
#define CM_DEFAULT_TRACE_THREAD_MASK 0xFFFFFFFFFFFFFFFFULL

/*! \def CM_DEFAULT_TRACE_SHIRE_MASK
    \brief Default masks to enable Trace for all CM shires.
*/
#define CM_DEFAULT_TRACE_SHIRE_MASK 0x1FFFFFFFFULL

/*! \def MM_SHIRE_MASK
    \brief Master Minion Shire mask
*/
#define MM_SHIRE_MASK (1ULL << 32)

/*! \def CM_SHIRE_MASK
    \brief Shire mask of Compute Minions Shires.
*/
#define CM_SHIRE_MASK 0xFFFFFFFFUL

/*! \def CM_MM_SHIRE_MASK
    \brief Shire mask of Compute Minions and Master Shire.
*/
#define CM_MM_SHIRE_MASK 0x1FFFFFFFFULL

/*! \def MM_HART_MASK
    \brief Master Minion Hart mask
*/
#define MM_HART_MASK 0xFFFFFFFFUL

/*! \def CM_HART_MASK
    \brief Compute Minion Hart mask
*/
#define CM_HART_MASK 0xFFFFFFFFFFFFFFFFULL

/*! \def CW_IN_MM_SHIRE
    \brief Computer worker HART index in MM Shire.
*/
#define CW_IN_MM_SHIRE 0xFFFFFFFF00000000ULL

/*! \def MM_HART_COUNT
    \brief Number of Harts running MMFW.
*/
#define MM_HART_COUNT 32U

/*! \enum cm_context_type_e
    \brief An enum that provides the types of CM execution contexts that can be saved.
*/
typedef enum {
    CM_CONTEXT_TYPE_HANG = 0,
    CM_CONTEXT_TYPE_UMODE_EXCEPTION,
    CM_CONTEXT_TYPE_SYSTEM_ABORT,
    CM_CONTEXT_TYPE_SELF_ABORT,
    CM_CONTEXT_TYPE_USER_KERNEL_ERROR,
    CM_CONTEXT_TYPE_TENSOR_ERROR
} cm_context_type_e;

/*! \struct execution_context_t
    \brief A structure that is used to store the execution context of hart
    when a exception occurs or abort is initiated.
*/
typedef struct {
    uint64_t type;
    uint64_t cycles;
    uint64_t hart_id;
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t scause;
    int64_t user_error;
    uint64_t gpr[31];
} __attribute__((packed, aligned(64))) execution_context_t;

/*! \struct cm_kernel_launched_flag_t
    \brief A structure that is used to store the kernel launch status on Compute
    Minion side.
*/
typedef struct {
    union {
        uint32_t flag;
        uint8_t raw[64];
    };
} __attribute__((aligned(64))) cm_kernel_launched_flag_t;

#endif
