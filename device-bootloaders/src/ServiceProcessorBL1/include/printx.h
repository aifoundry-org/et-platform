/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

// printx.h

#ifndef __PRINTX_H__
#define __PRINTX_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

int printx(const char *format, ...);
int snprintx(char *str, size_t size, const char *format, ...);

//int vprintx(const char *format, va_list ap);
//int vsnprintx(char *str, size_t size, const char *format, va_list ap);

#endif
