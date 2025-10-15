/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file console.c
    \brief A C module that implements the Console Driver

    Public interfaces:
        Console_Putchar
*/
/***********************************************************************/
/* mm specific headers */
#include "drivers/console.h"

/************************************************************************
*
*   FUNCTION
*
*       Console_Putchar
*
*   DESCRIPTION
*
*       Writes a character to serial port
*
*   INPUTS
*
*       character   character to write to serial port
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Console_Putchar(char character)
{
    SERIAL_write(PU_UART0, &character, 1);
}
