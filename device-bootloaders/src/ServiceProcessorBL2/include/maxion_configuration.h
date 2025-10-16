/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file maxion_configuration.h
    \brief A C header that defines the maxion configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __MAXION_CONFIGURATION_H__
#define __MAXION_CONFIGURATION_H__

#define MAXION_PLL_MHZ_1500 3
#define MAXION_PLL_MHZ_933  28

void Maxion_Shire_Channel_Enable(void);
void Maxion_Shire_Cache_Partition(void);
void Maxion_Set_BootVector(uint64_t address);
void Maxion_Fill_Bootram_With_Nops(void);
int Maxion_Configure_PLL_Core(uint8_t mode);
int Maxion_Configure_PLL_Uncore(uint8_t mode);
int Maxion_Init(uint8_t pllModeUncore, uint8_t pllModeCore);
void Maxion_Initialize_Boot_Code(void);
void Maxion_Check_If_Booted(void);
void Maxion_SetVoltage(uint8_t voltage);

#endif // __MAXION_CONFIGURATION_H__
