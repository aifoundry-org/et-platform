/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements the console driver services
*
*   FUNCTIONS
*
*       Console_Putchar
*
***********************************************************************/
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
    SERIAL_write(UART0, &character, 1);
}
