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
************************************************************************/
/*! \file maxion_configuration.h
    \brief A C header that defines the maxion configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __MAXION_CONFIGURATION_H__
#define __MAXION_CONFIGURATION_H__

#include <stdint.h>
//#include "sp_otp.h"
#include "bl_error_code.h"
#include "etsoc/isa/io.h"

#include "hwinc/hal_device.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_misc.h"

void Maxion_Shire_Channel_Enable(void);
void Maxion_Set_BootVector(uint64_t address);
void Maxion_Reset_Cold_Deassert(void);
void Maxion_Reset_Warm_Uncore_Deassert(void);
void Maxion_Reset_PLL_Uncore_Deassert(void);
void Maxion_Reset_PLL_Core_Deassert(void);
int  Maxion_Reset_Warm_Core_Deassert(uint8_t coreNumber);
void Maxion_Reset_Warm_Core_All_Deassert(void);



#endif
