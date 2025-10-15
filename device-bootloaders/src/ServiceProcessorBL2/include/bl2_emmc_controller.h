/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/
/************************************************************************/
/*! \file bl2_emmc_controller.h
    \brief A C header that defines public interfaces for eMMC driver
*/
/***********************************************************************/

#ifndef __BL2_EMMC_CONTROLLER_H__
#define __BL2_EMMC_CONTROLLER_H__

#include "stdbool.h"

typedef enum
{
    EMMC_MODE_LEGACY = 0,
    EMMC_MODE_HSSDR = 1,
    EMMC_MODE_HS200 = 3,
    EMMC_MODE_HSDDR = 4,
    EMMC_MODE_HS400 = 7

} EMMC_MODE_t;

#define EMMC_PROBE_VERBOSITY_OFF false
#define EMMC_PROBE_VERBOSITY_ON  true
#define EMMC_BLOCK_SIZE          512

// eMMC setup
int Emmc_Probe(EMMC_MODE_t mode, bool verbose);
int Emmc_Setup(EMMC_MODE_t mode);

// write/read functions
//iomode
int Emmc_Iomode_Blk_Wr(uint32_t addr, uint32_t *data_buff, uint16_t block_size,
                       uint32_t block_count, EMMC_MODE_t mode);
int Emmc_Iomode_Blk_Rd(uint32_t addr, uint32_t *data_buff, uint16_t block_size,
                       uint32_t block_count, EMMC_MODE_t mode);
//dma
int Emmc_Adma2_Wr(uint32_t addr, uint32_t *data_buff, uint16_t block_size, uint32_t block_count);
int Emmc_Adma2_Rd(uint32_t addr, uint32_t *data_buff, uint16_t block_size, uint32_t block_count);

//EMMC bulk read
int Emmc_read_to_buffer(uint8_t *buffer, size_t size, uint64_t sector);
#endif /* __BL2_EMMC_CONTROLLER_H__ */
