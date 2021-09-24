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
/*! \file common_trace_defs.h
    \brief A C header that defines the common defines for device Trace.
*/
/***********************************************************************/

#ifndef COMMON_TRACE_DEFS
#define COMMON_TRACE_DEFS

/*! \def CM_SHIRE_MASK
    \brief Shire mask of Compute Workers.
*/
#define CM_SHIRE_MASK    0xFFFFFFFFULL

/*! \def CW_IN_MM_SHIRE
    \brief Computer worker HART index in MM Shire.
*/
#define CW_IN_MM_SHIRE   0xFFFFFFFF00000000ULL

/*! \def TRACE_CONFIG_CHECK_MM_HART
    \brief Helper macro to check if given shire and thread masks contains any MM HART.
*/
#define TRACE_CONFIG_CHECK_MM_HART(shire_mask, thread_mask)                             \
                    (shire_mask & MM_SHIRE_MASK) && (thread_mask & MM_HART_MASK)

/*! \def TRACE_CONFIG_CHECK_CM_HART
    \brief Helper macro to check if given shire and thread masks contains any CM HART.
*/
#define TRACE_CONFIG_CHECK_CM_HART(shire_mask, thread_mask)                             \
                    (((shire_mask & CM_SHIRE_MASK) && (thread_mask & MM_HART_MASK)) ||  \
                    ((shire_mask & MM_SHIRE_MASK) && (thread_mask & CW_IN_MM_SHIRE)))

#endif