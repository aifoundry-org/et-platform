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
/***********************************************************************/
/*! \file SP_FW_memory_layout.h
    \brief A C header that defines the Service Processor Firmware Memory
     Layout
*/
/***********************************************************************/
#ifndef SP_FW_MEMORY_LAYOUT_H
#define SP_FW_MEMORY_LAYOUT_H

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

#include "etsoc_ddr_region_map.h"

// Cache_align macro to return a cache aligned address for any given input address
#define CACHE_LINE_ALIGN_(x) ((x + 63u) & ~63u)

// This range is dedicated for storing SP FW trace artifacts

#define SP_FW_MAP_TRACE_BUFFER_REGION_ADDRESS CACHE_LINE_ALIGN_(HIGH_OS_SUBREGION_BASE + 0x1000000) /* TBD */
#define SP_FW_MAP_TRACE_BUFFER_REGION_SIZE   0x1000U /* 4 KB memory to be optimized in future */

// This range is dedicated for Device Management library to manage. Example
// usage of this Scratch area is to storage new FW image from Host whilst
// SP is in the process of updating the correspoding Flash partition

#define SP_FW_MAP_DEV_MANAGEMENT_SCRATCH_REGION_ADDRESS CACHE_LINE_ALIGN_(HIGH_OS_SUBREGION_BASE + 0x2000000) /* TBD */
#define SP_FW_MAP_DEV_MANAGEMENT_SCRATCH_REGION_SIZE   0x400000U

// This is the region dedicated to device's MBOX triggers
#define SP_FW_MAP_DEV_INTERRUPT_TRG_BASE R_PU_TRG_PCIE_BASEADDR
#define SP_FW_MAP_DEV_INTERRUPT_TRG_SIZE R_PU_TRG_PCIE_SIZE

#endif /* SP_FW_MEMORY_LAYOUT_H */
