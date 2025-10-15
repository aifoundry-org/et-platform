/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
