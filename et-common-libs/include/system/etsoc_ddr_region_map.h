/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/***********************************************************************/
/*! \file etsoc_memory_map.h
    \brief A C header that defines the ET SOC Memory Map region layout
    for Secure Mode
*/
/***********************************************************************/
#ifndef _ETSOC_DDR_REGION_MAP_H_
#define _ETSOC_DDR_REGION_MAP_H_

/*****************************/
/* DDR low memory subregions */
/*****************************/
/* MCODE region is 2M from 0x8000000000 to 0x80001FFFFF */
#define LOW_MCODE_SUBREGION_BASE  0x8000000000ULL
#define LOW_MCODE_SUBREGION_SIZE  0x0000200000ULL

/* MDATA region is 6M from 0x8000200000 to 0x80007FFFFF */
#define LOW_MDATA_SUBREGION_BASE  0x8000200000ULL
#define LOW_MDATA_SUBREGION_SIZE  0x0000600000ULL

/* SCODE region is 8M from 0x8000800000 to 0x8000FFFFFF */
#define LOW_SCODE_SUBREGION_BASE  0x8000800000ULL
#define LOW_SCODE_SUBREGION_SIZE  0x0000800000ULL

/* SDATA region is 48M from 0x8001000000 to 0x8003FFFFFF */
#define LOW_SDATA_SUBREGION_BASE  0x8001000000ULL
#define LOW_SDATA_SUBREGION_SIZE  0x0003000000ULL

/* Low OS region is 4032M from 0x8004000000 to 0x80FFFFFFFF */
#define LOW_OS_SUBREGION_BASE  0x8004000000ULL
#define LOW_OS_SUBREGION_SIZE  0x00FC000000ULL

/* Low Memory region is 28G from 0x8100000000 to 0x87FFFFFFF */
#define LOW_MEMORY_SUBREGION_BASE 0x8100000000ULL
#define LOW_MEMORY_SUBREGION_SIZE 0x0700000000ULL

/* RESERVED MEMORY region is 224G from 0x8800000000 to 0xBFFFFFFFF */
#define LOW_RESERVED_SUBREGION_BASE  0x8800000000ULL
#define LOW_RESERVED_SUBREGION_SIZE  0x3800000000ULL

/******************************/
/* DDR high memory subregions */
/******************************/
/* MCODE region is 2M from 0xC000000000 to 0xC0001FFFFF */
#define HIGH_MCODE_SUBREGION_BASE 0xC000000000ULL
#define HIGH_MCODE_SUBREGION_SIZE 0x0000200000ULL

/* MDATA region is 6M from 0xC000200000 to 0xC0007FFFFF */
#define HIGH_MDATA_SUBREGION_BASE 0xC000200000ULL
#define HIGH_MDATA_SUBREGION_SIZE 0x0000600000ULL

/* SCODE region is 8M from 0xC000800000 to 0xC000FFFFFF */
#define HIGH_SCODE_SUBREGION_BASE 0xC000800000ULL
#define HIGH_SCODE_SUBREGION_SIZE 0x0000800000ULL

/* SDATA region is 48M from 0xC001000000 to 0xC003FFFFFF */
#define HIGH_SDATA_SUBREGION_BASE 0xC001000000ULL
#define HIGH_SDATA_SUBREGION_SIZE 0x0003000000ULL

/* High OS region is 4032M from 0xC004000000 to 0xC0FFFFFFFF */
#define HIGH_OS_SUBREGION_BASE 0xC004000000ULL
#define HIGH_OS_SUBREGION_SIZE 0x00FC000000ULL

/* High Memory region is 28G from 0xC100000000 to 0xC074000000 */
#define HIGH_MEMORY_SUBREGION_BASE 0xC100000000ULL
#define HIGH_MEMORY_SUBREGION_SIZE 0x0700000000ULL

/* RESERVED MEMORY region is 224G from 0xC800000000 to 0xFFFFFFFFFF */
#define HIGH_RESERVED_SUBREGION_BASE 0xC800000000ULL
#define HIGH_RESERVED_SUBREGION_SIZE 0x3800000000ULL

#endif /* _ETSOC_DDR_REGION_MAP_H_ */
