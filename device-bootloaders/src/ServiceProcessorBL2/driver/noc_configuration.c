/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file noc_configuration.c
    \brief A C module that implements the main NOC configuration services.

    Public interfaces:

        NOC_Configure
*/
/***********************************************************************/
#include <stdint.h>
#include "bl2_sp_pll.h"

#include "bl_error_code.h"
#include "log.h"

#include "noc_configuration.h"


int32_t NOC_Configure(uint8_t mode)
{
    if (0 != configure_sp_pll_2(mode)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_2() failed!\n");
        return NOC_MAIN_CLOCK_CONFIGURE_ERROR;
    }

   // TBD: Configure Main NOC Registers via Regbus
   // Remap NOC ID based on MM OTP value
   // Other potential NOC workarounds
   
   return SUCCESS;
}

/* TODO: SW-8062 More functions to be implementated */ 


