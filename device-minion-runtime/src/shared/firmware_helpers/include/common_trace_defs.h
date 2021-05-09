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

/*! \def CM_DEFAULT_TRACE_THREAD_MASK
    \brief Default masks to enable Trace for first hart in CM Shires. 
*/
#define CM_DEFAULT_TRACE_THREAD_MASK      (0x1UL)

/*! \def CM_DEFAULT_TRACE_SHIRE_MASK
    \brief Default masks to enable Trace for first CM shire. 
*/
#define CM_DEFAULT_TRACE_SHIRE_MASK       (0x1UL)

#endif