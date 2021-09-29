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
/*! \file trace.h
    \brief A C header that implements the Trace services for CM UMode.
*/
/***********************************************************************/

#ifndef TRACE_UMODE_H
#define TRACE_UMODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <et-trace/encoder.h>

/*! \fn void __et_printf(const char *fmt)
    \brief This function logs the string message into Trace, If Trace was
           enabled for caller Hart.
           NOTE: CM UMode Trace is initialized by CM Runtime in SMode.
    \param fmt String format specifier
    \param ... Variable argument list
    \return None
*/
void __et_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* TRACE_UMODE_H */