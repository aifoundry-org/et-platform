/*------------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_APIS_TRACE_TYPES_H
#define ET_DEVICE_APIS_TRACE_TYPES_H

#include <stdint.h>

/* NOTE: This file is common for all Trace users including Master Minion,
         Compute Minion, Servie Processor, and Host side. */

/*! \def CM_DEFAULT_TRACE_THREAD_MASK
    \brief Default masks to enable Trace for first hart in CM Shires.
*/
#define CM_DEFAULT_TRACE_THREAD_MASK      (0x1UL)

/*! \def CM_DEFAULT_TRACE_SHIRE_MASK
    \brief Default masks to enable Trace for first CM shire.
*/
#define CM_DEFAULT_TRACE_SHIRE_MASK       (0x1UL)

#endif // ET_DEVICE_APIS_TRACE_TYPES_H
