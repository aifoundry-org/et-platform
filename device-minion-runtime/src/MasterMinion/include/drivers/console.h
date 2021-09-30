/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file console.h
    \brief A C header that defines the Console Driver's public interfaces
*/
/***********************************************************************/
#ifndef CONSOLE_DEFS_H
#define CONSOLE_DEFS_H

/* mm_rt_helpers */
#include <common/printf.h>
#include <etsoc/drivers/serial/serial.h>

/*! \fn void Console_Putchar(char character)
    \brief Write a character to the serial port
    \param character character to write
*/
void Console_Putchar(char character);

#endif /* CONSOLE_DEFS_H */
