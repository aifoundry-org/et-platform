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
/*! \file flashfs.c
    \brief A C module that implements the flash filesystem service. This
    service provides fs initialization and file read/write functionality.

    Public interfaces:
        flash_fs_init
        flash_fs_preload_config_data
        flash_fs_get_config_data
        flash_fs_get_file_size
        flash_fs_read_file
        flash_fs_partition_read_file
        flash_fs_write_partition
        flash_fs_erase_partition
        flash_fs_update_partition
        flash_fs_swap_primary_boot_partition
        flash_fs_get_boot_counters
        flash_fs_increment_completed_boot_count
        flash_fs_increment_attempted_boot_count
        flash_fs_get_manufacturer_name
        flash_fs_get_part_number
        flash_fs_set_part_number
        flash_fs_get_serial_number
        flash_fs_get_module_rev
        flash_fs_get_memory_size
        flash_fs_get_form_factor
        flash_fs_scan_partition
        flash_fs_get_vmin_lut
        flash_fs_set_vmin_lut
        flash_fs_get_mnn_boot_freq
        flash_fs_get_mnn_boot_voltage
        flash_fs_get_sram_boot_freq
        flash_fs_get_sram_boot_voltage
        flash_fs_get_noc_boot_freq
        flash_fs_get_noc_boot_voltage
        flash_fs_get_pcl_boot_freq
        flash_fs_get_pcl_boot_voltage
        flash_fs_get_ddr_boot_freq
        flash_fs_get_ddr_boot_voltage
        flash_fs_get_mxn_boot_freq
        flash_fs_get_mxn_boot_voltage
*/
/***********************************************************************/

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "log.h"
#include "etsoc/drivers/serial/serial.h"
#include "crc32.h"
#include "bl2_scratch_buffer.h"
#include "bl2_spi_controller.h"
#include "bl2_spi_flash.h"
#include "bl2_flash_fs.h"
#include "bl2_vaultip_driver.h"
#include "jedec_sfdp.h"
#include "bl2_main.h"
#include "bl_error_code.h"

#pragma GCC push_options
/* #pragma GCC optimize ("O2") */
#pragma GCC diagnostic ignored "-Wswitch-enum"

/* Assertion to make sure that the sector being modified in flash_fs_set_part_number() is the expected one */
static_assert(SPI_FLASH_SECTOR_SIZE >= sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_DATA_t),
              "flash_fs_set_part_number(): ESPERANTO_CONFIG_DATA_t not in the expected sector!");

/*! \def INVALID_REGION_INDEX
    \brief invalid region index value.
*/
#define INVALID_REGION_INDEX 0xFFFFFFFF

/*! \def MAXIMUM_FAILED_BOOT_ATTEMPTS
    \brief maximum attempts on boot failure
*/
#define MAXIMUM_FAILED_BOOT_ATTEMPTS 3

/*! \def USE_SFDP
    \brief enable use of SFDP
*/
#define USE_SFDP

/*! \def PAGE_PROGRAM_TIMEOUT
    \brief maximum timeout value for page program
*/
#define PAGE_PROGRAM_TIMEOUT 2000

/*! @union value_uu
    \brief Use to represent the smallest chunk of the Flash memory
*/
union
{
    unsigned long long ull;
    uint8_t u8[sizeof(unsigned long long)];
} value_uu;

static FLASH_FS_BL2_INFO_t sg_flash_fs_bl2_info = { 0 };

static int get_config_region_address(uint32_t *cfg_region_addr)
{
    uint32_t partition_address;

    if (0 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = 0;
    }
    else if (1 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    *cfg_region_addr = partition_address + sg_flash_fs_bl2_info.configuration_region_address;

    return STATUS_SUCCESS;
}

static int flash_fs_update_sector_and_read_back_for_validation(SPI_FLASH_ID_t flash_id,
                                                               uint32_t address, void *buffer,
                                                               uint32_t buffer_size)
{
    /* Erase sector */
    if (0 != spi_flash_sector_erase(flash_id, address))
    {
        MESSAGE_ERROR(
            "flash_fs_update_sector_and_read_back_for_validation: failed to erase sector!\n");
        return ERROR_SPI_FLASH_SE_FAILED;
    }

    /* Write back the updated sector */
    if (0 != flash_fs_write_partition(address, buffer, SPI_FLASH_SECTOR_SIZE, SPI_FLASH_PAGE_SIZE))
    {
        MESSAGE_ERROR(
            "flash_fs_update_sector_and_read_back_for_validation: spi_flash_program() failed!\n");
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    memset(buffer, 0, buffer_size);

    /* Read back sector for validation */
    if (0 != spi_flash_normal_read(flash_id, address, (uint8_t *)buffer, SPI_FLASH_SECTOR_SIZE))
    {
        MESSAGE_ERROR(
            "flash_fs_update_sector_and_read_back_for_validation: failed to read sector for validation!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       count_zero_bits
*
*   DESCRIPTION
*
*       This function returns count of zero bits in data array.
*
*   INPUTS
*
*       data                 pointer to data
*       data_size            size of data
*
*   OUTPUTS
*
*       number of zero bits
*
***********************************************************************/

static uint32_t count_zero_bits(const unsigned long long *data, uint32_t data_size)
{
    int count = 0;

    for (uint32_t index = 0; index < data_size; index++)
    {
        count += __builtin_popcountll(data[index]);
    }

    return (data_size * (uint32_t)sizeof(unsigned long long) * 8u) - (uint32_t)count;
}

static inline int search_bit_location(uint32_t byte_index, uint32_t current_chunk,
                                      uint32_t *chunk_offset, uint32_t flash_region, uint32_t *bit)
{
    uint32_t flag;
    if (0 != current_chunk)
    {
        for (uint32_t n = 0; n < 8; n++)
        {
            flag = 0x01u << n;
            if (flag & current_chunk)
            {
                *chunk_offset = byte_index + (uint32_t)(sizeof(unsigned long long) * flash_region);
                *bit = n;
                return 1;
            }
        }
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       find_first_set_bit_offset
*
*   DESCRIPTION
*
*       This function calculates offset of first non zero bit in data array.
*
*   INPUTS
*
*       data                 pointer to data
*       data_size            size of data
*
*   OUTPUTS
*
*       offset               offset of byte which contains non zero bit
*       bit                  offset of non zero bit inside byte
*
***********************************************************************/

static int find_first_set_bit_offset(uint32_t *offset, uint32_t *bit,
                                     const unsigned long long *ull_array, uint32_t ull_array_size)
{
    uint32_t ull_index;
    const unsigned long long *data = ull_array;
    const unsigned long long *ull_array_end = ull_array + ull_array_size;

    while (data < ull_array_end)
    {
        if (0 != *data)
        {
            ull_index = (uint32_t)(data - ull_array);
            value_uu.ull = *data;
            for (uint32_t b_index = 0; b_index < sizeof(unsigned long long); b_index++)
            {
                if (search_bit_location(b_index, value_uu.u8[b_index], offset, ull_index, bit))
                {
                    return 0;
                }
            }
        }
        data++;
    }

    return ERROR_SPI_FLASH_BOOT_COUNTER_REGION_FULL;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_scan_regions
*
*   DESCRIPTION
*
*       This function scans and checks partition regions
*
*   INPUTS
*
*       partition_size       size_of_partition
*       partition_info       partition info struct
*
*   OUTPUTS
*
*       partition_info       partition info struct with updated indexes
*
***********************************************************************/

static int flash_fs_scan_regions(uint32_t partition_size,
                                 ESPERANTO_PARTITION_BL2_INFO_t *partition_info)
{
    uint32_t crc;
    uint32_t region_offset_end;
    uint32_t partition_size_in_blocks = partition_size / FLASH_PAGE_SIZE;

    partition_info->sp_bl2_region_index = INVALID_REGION_INDEX;

    for (uint32_t n = 0; n < partition_info->header.regions_count; n++)
    {
        switch (partition_info->regions_table[n].region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
            case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
            case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
            case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ:
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S0:
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S1:
                break;
            default:
                continue;
        }

        crc = 0;
        crc32(&(partition_info->regions_table[n]),
              offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum), &crc);
        if (crc != partition_info->regions_table[n].region_info_checksum)
        {
            MESSAGE_ERROR("flash_fs_scan_regions: region %u CRC mismatch! (expected %08x, got %08x)\
                   \n",
                          n, partition_info->regions_table[n].region_info_checksum, crc);
            return ERROR_SPI_FLASH_REGION_CRC_MISMATCH;
        }

        if (0 == partition_info->regions_table[n].region_offset ||
            partition_info->regions_table[n].region_offset >= partition_size_in_blocks)
        {
            MESSAGE_ERROR("flash_fs_scan_regions: invalid region %u offset!\n", n);
            return ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET;
        }
        if (0 == partition_info->regions_table[n].region_reserved_size)
        {
            MESSAGE_ERROR("flash_fs_scan_regions: region %u has zero size!\n", n);
            return ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET;
        }
        region_offset_end = partition_info->regions_table[n].region_offset +
                            partition_info->regions_table[n].region_reserved_size;
        if (region_offset_end < partition_info->regions_table[n].region_offset)
        {
            MESSAGE_ERROR("flash_fs_scan_regions: region %u offset/size overflow!\n", n);
            return ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET; /* integer overflow */
        }
        if (region_offset_end > partition_size_in_blocks)
        {
            MESSAGE_ERROR("flash_fs_scan_regions: invalid region %u size!\n", n);
            return ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET; /* integer overflow */
        }

        switch (partition_info->regions_table[n].region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
                partition_info->dram_training_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
                partition_info->machine_minion_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
                partition_info->master_minion_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
                partition_info->worker_minion_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
                partition_info->maxion_bl1_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ:
                partition_info->dram_training_payload_800mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ:
                partition_info->dram_training_payload_933mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ:
                partition_info->dram_training_payload_1067mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D:
                partition_info->dram_training_2d_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ:
                partition_info->dram_training_2d_payload_800mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ:
                partition_info->dram_training_2d_payload_933mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ:
                partition_info->dram_training_2d_payload_1067mhz_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S0:
                partition_info->pmic_fw_s0_region_index = n;
                break;
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S1:
                partition_info->pmic_fw_s1_region_index = n;
                break;
            default:
                MESSAGE_ERROR("flash_fs_scan_regions: invalid region id: %u!\n",
                              partition_info->regions_table[n].region_id);
                return ERROR_SPI_FLASH_INVALID_REGION_ID; /* we should never get here */
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_preload_config_data
*
*   DESCRIPTION
*
*       This function loads asset config data from the flash image.
*
*   INPUTS
*
*       flash_fs_bl2_info      Pointer to the info struct to populate
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

static int flash_fs_preload_config_data(FLASH_FS_BL2_INFO_t *flash_fs_bl2_info)
{
    if (NULL == flash_fs_bl2_info)
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    // @cabul: Copied from flash_fs_load_file_info
    uint32_t partition_address;
    uint32_t config_data_address = 0;
    // Get partition address based on active partition
    if (0 == flash_fs_bl2_info->active_partition)
    {
        partition_address = 0;
    }
    else if (1 == flash_fs_bl2_info->active_partition)
    {
        partition_address = flash_fs_bl2_info->flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    uint32_t region_address = flash_fs_bl2_info->configuration_region_address;

    config_data_address =
        partition_address + region_address + (uint32_t)sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t);

    if (0 != spi_flash_normal_read(flash_fs_bl2_info->flash_id, config_data_address,
                                   (uint8_t *)&(flash_fs_bl2_info->asset_config_header),
                                   sizeof(ESPERANTO_CONFIG_HEADER_t)))
    {
        MESSAGE_ERROR("flash_fs_preload_config_data: failed to read asset config header!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "asset_config_header.tag:     0x%08x\n",
              flash_fs_bl2_info->asset_config_header.tag);
    Log_Write(LOG_LEVEL_DEBUG, "asset_config_header.version: 0x%08x\n",
              flash_fs_bl2_info->asset_config_header.version);
    Log_Write(LOG_LEVEL_DEBUG, "asset_config_header.hash:    0x%016lx\n",
              flash_fs_bl2_info->asset_config_header.hash);

    if (0 !=
        spi_flash_normal_read(flash_fs_bl2_info->flash_id,
                              config_data_address + (uint32_t)sizeof(ESPERANTO_CONFIG_HEADER_t),
                              (uint8_t *)&(flash_fs_bl2_info->asset_config_data),
                              sizeof(ESPERANTO_CONFIG_DATA_t)))
    {
        MESSAGE_ERROR("flash_fs_preload_config_data: failed to read asset config data!\n");
        memset(&(flash_fs_bl2_info->asset_config_data), 0, sizeof(ESPERANTO_CONFIG_DATA_t));
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    Log_Write(
        LOG_LEVEL_DEBUG,
        "asset_config_data:\n\tmanuf_name:  %s\n\tpart_num:    0x%08x\n\tserial_num:  0x%016lx\n",
        flash_fs_bl2_info->asset_config_data.persistent_config.manuf_name,
        flash_fs_bl2_info->asset_config_data.persistent_config.part_num,
        flash_fs_bl2_info->asset_config_data.persistent_config.serial_num);
    Log_Write(LOG_LEVEL_DEBUG, "\tmodule_rev:  0x%08x\n\tform_factor: 0x%02x\n",
              flash_fs_bl2_info->asset_config_data.persistent_config.module_rev,
              flash_fs_bl2_info->asset_config_data.persistent_config.form_factor);
    for (uint32_t i = 0; i < NUMBER_OF_VMIN_LUT_POINTS; i++)
    {
        Log_Write(LOG_LEVEL_DEBUG, "\tvmin_lut[%d]:\n", i);
        Log_Write(LOG_LEVEL_DEBUG,
                  "\tmnn.freq: %4d, mnn.volt: 0x%02x\n\tsrm.freq: %4d, srm.volt: 0x%02x\n",
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].mnn.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].mnn.volt,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].sram.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].sram.volt);
        Log_Write(LOG_LEVEL_DEBUG,
                  "\tnoc.freq: %4d, noc.volt: 0x%02x\n\tpcl.freq: %4d, pcl.volt: 0x%02x\n",
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].noc.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].noc.volt,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].pcl.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].pcl.volt);
        Log_Write(LOG_LEVEL_DEBUG,
                  "\tddr.freq: %4d, ddr.volt: 0x%02x\n\tmxn.freq: %4d, mxn.volt: 0x%02x\n",
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].ddr.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].ddr.volt,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].mxn.freq,
                  flash_fs_bl2_info->asset_config_data.persistent_config.vmin_lut[i].mxn.volt);
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_init
*
*   DESCRIPTION
*
*       This function initialize partition infos.
*
*   INPUTS
*
*       flash_fs_bl2_info    bl2 partition info struct
*
***********************************************************************/

int flash_fs_init(FLASH_FS_BL2_INFO_t *flash_fs_bl2_info)
{
    uint32_t partition_size;

    if (NULL == flash_fs_bl2_info)
    {
        MESSAGE_ERROR("flash_fs_init: invalid argument!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    partition_size = flash_fs_bl2_info->flash_size / 2;

    /* re-scan both partitions in the flash */
    for (uint32_t n = 0; n < 2; n++)
    {
        Log_Write(LOG_LEVEL_INFO, "Re-scanning partition %u...\n", n);

        if (false == flash_fs_bl2_info->partition_info[n].partition_valid)
        {
            Log_Write(LOG_LEVEL_WARNING, "Partition %u is not valid, skipping\n", n);
            continue;
        }

        /* Verify and load the partition regions info */
        if (0 != flash_fs_scan_regions(partition_size, &flash_fs_bl2_info->partition_info[n]))
        {
            Log_Write(LOG_LEVEL_ERROR, "Partition %u seems corrupted.\n", n);
        }
        /* test if the critical required files are present in the partition */
        else if (INVALID_REGION_INDEX ==
                 flash_fs_bl2_info->partition_info[n].dram_training_region_index)
        {
            Log_Write(LOG_LEVEL_ERROR, "DRAM training data not found!\n");
        }
        else
        {
            continue;
        }

        flash_fs_bl2_info->partition_info[n].partition_valid = false;
        if (flash_fs_bl2_info->other_partition_valid == 1 &&
            flash_fs_bl2_info->active_partition == n)
        {
            flash_fs_bl2_info->active_partition = 1 - n;
            Log_Write(LOG_LEVEL_CRITICAL, "Partition %u is now the active partition.\n", 1 - n);
        }
        flash_fs_bl2_info->other_partition_valid = 0;
    }

    if (false ==
        flash_fs_bl2_info->partition_info[flash_fs_bl2_info->active_partition].partition_valid)
    {
        /* the active boot partition is no longer valid! */
        MESSAGE_ERROR("No valid partition found!\n");
        return ERROR_SPI_FLASH_FS_INIT_FAILED;
    }

    flash_fs_preload_config_data(flash_fs_bl2_info);

    memcpy(&sg_flash_fs_bl2_info, flash_fs_bl2_info, sizeof(FLASH_FS_BL2_INFO_t));

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_load_file_info
*
*   DESCRIPTION
*
*       This function loads file info for a particular region in the
*       given partition.
*
*   INPUTS
*
*       partition            partition ID
*       region_id            region id
*
*   OUTPUTS
*
*       file_data_address    address of file data
*       file_size            size of file data
*
***********************************************************************/

int flash_fs_load_file_info(uint32_t partition, ESPERANTO_FLASH_REGION_ID_t region_id,
                            uint32_t *file_data_address, uint32_t *file_size)
{
    uint32_t crc;
    uint32_t partition_address;
    ESPERANATO_FILE_INFO_t *file_info = NULL;
    uint32_t region_index;
    uint32_t region_address;
    uint32_t region_size;

    if (0 == partition)
    {
        partition_address = 0;
    }
    else if (1 == partition)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    switch (region_id)
    {
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            file_info = &(sg_flash_fs_bl2_info.dram_training_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[partition].dram_training_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
            file_info = &(sg_flash_fs_bl2_info.machine_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[partition].machine_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
            file_info = &(sg_flash_fs_bl2_info.master_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[partition].master_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
            file_info = &(sg_flash_fs_bl2_info.worker_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[partition].worker_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
            file_info = &(sg_flash_fs_bl2_info.maxion_bl1_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition].maxion_bl1_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_payload_800mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_payload_800mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_payload_933mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_payload_933mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_payload_1067mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_payload_1067mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D:
            file_info = &(sg_flash_fs_bl2_info.dram_training_2d_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[partition].dram_training_2d_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_2d_payload_800mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_2d_payload_800mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_2d_payload_933mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_2d_payload_933mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ:
            file_info = &(sg_flash_fs_bl2_info.dram_training_2d_payload_1067mhz_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition]
                               .dram_training_2d_payload_1067mhz_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S0:
            file_info = &(sg_flash_fs_bl2_info.pmic_fw_s0_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition].pmic_fw_s0_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S1:
            file_info = &(sg_flash_fs_bl2_info.pmic_fw_s1_file_info);
            region_index = sg_flash_fs_bl2_info.partition_info[partition].pmic_fw_s1_region_index;
            break;
        default:
            return ERROR_SPI_FLASH_INVALID_REGION_ID;
    }

    if (INVALID_REGION_INDEX == region_index)
    {
        return ERROR_SPI_FLASH_INVALID_REGION_ID;
    }

    region_address =
        sg_flash_fs_bl2_info.partition_info[partition].regions_table[region_index].region_offset *
        FLASH_PAGE_SIZE;
    region_size = sg_flash_fs_bl2_info.partition_info[partition]
                      .regions_table[region_index]
                      .region_reserved_size *
                  FLASH_PAGE_SIZE;

    if (0 == file_info->file_header_tag && 0 == file_info->file_header_size &&
        0 == file_info->file_size && 0 == file_info->file_header_crc)
    {
        if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id,
                                       partition_address + region_address, (uint8_t *)file_info,
                                       sizeof(ESPERANATO_FILE_INFO_t)))
        {
            MESSAGE_ERROR("flash_fs_load_file_info: failed to read file info!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
        }

        if (ESPERANTO_FILE_TAG != file_info->file_header_tag)
        {
            MESSAGE_ERROR("flash_fs_load_file_info: invalid file info header tag!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return ERROR_SPI_FLASH_INVALID_FILE_INFO;
        }

        if (sizeof(ESPERANATO_FILE_INFO_t) != file_info->file_header_size)
        {
            MESSAGE_ERROR("flash_fs_load_file_info: invalid file info header size!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return ERROR_SPI_FLASH_INVALID_FILE_INFO;
        }

        crc = 0;
        crc32(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc), &crc);
        if (crc != file_info->file_header_crc)
        {
            MESSAGE_ERROR("flash_fs_load_file_info: file info CRC mismatch!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return ERROR_SPI_FLASH_INVALID_FILE_INFO;
        }

        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t)))
        {
            MESSAGE_ERROR("flash_fs_load_file_info: invalid file size!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return ERROR_SPI_FLASH_INVALID_FILE_INFO;
        }
    }

    *file_data_address =
        (uint32_t)(partition_address + region_address + sizeof(ESPERANATO_FILE_INFO_t));
    *file_size = file_info->file_size;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_file_size
*
*   DESCRIPTION
*
*       This function returns file size for a particular region
*
*   INPUTS
*
*       region_id            region id
*
*   OUTPUTS
*
*       size                 size of file data
*
***********************************************************************/

int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size)
{
    uint32_t file_data_address;
    uint32_t file_size;

    if (0 != flash_fs_load_file_info(sg_flash_fs_bl2_info.active_partition, region_id,
                                     &file_data_address, &file_size))
    {
        MESSAGE_ERROR("flash_fs_get_file_size: flash_fs_load_file_info(0x%x) failed!\n", region_id);
        return ERROR_SPI_FLASH_LOAD_FILE_INFO_FAILED;
    }

    *size = file_size;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_read_file
*
*   DESCRIPTION
*
*       This function reads the data from the file of the particular region.
*
*   INPUTS
*
*       region_id            region id
*       offset               offset inside file to start read from
*       buffer_size          size in bytes to be read
*
*   OUTPUTS
*
*       buffer               data read from file
*
***********************************************************************/

int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size)
{
    uint32_t file_data_address;
    uint32_t file_size;
    uint32_t end_offset;

    if (NULL == buffer || 0 == buffer_size)
    {
        MESSAGE_ERROR("flash_fs_read_file: invalid arguments!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (0 != flash_fs_load_file_info(sg_flash_fs_bl2_info.active_partition, region_id,
                                     &file_data_address, &file_size))
    {
        MESSAGE_ERROR("flash_fs_read_file: flash_fs_load_file_info(0x%x) failed!\n", region_id);
        return ERROR_SPI_FLASH_LOAD_FILE_INFO_FAILED;
    }

    if (offset >= file_size)
    {
        MESSAGE_ERROR("flash_fs_read_file: offset too large!\n");
        return ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET;
    }

    end_offset = offset + buffer_size;
    if (end_offset > file_size)
    {
        MESSAGE_ERROR("flash_fs_read_file: end_offset too large!\n");
        return ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET;
    }

    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, file_data_address + offset,
                                   (uint8_t *)buffer, buffer_size))
    {
        MESSAGE_ERROR("flash_fs_read_file: failed to read file data!\n");
        memset(buffer, 0, buffer_size);
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_partition_read_file
*
*   DESCRIPTION
*
*       This function reads the data from the file of the particular region
*       in the given partition.
*
*   INPUTS
*
*       partition            partition ID
*       region_id            region id
*       offset               offset inside file to start read from
*       buffer_size          size in bytes to be read
*
*   OUTPUTS
*
*       buffer               data read from file
*
***********************************************************************/

int flash_fs_partition_read_file(uint32_t partition, ESPERANTO_FLASH_REGION_ID_t region_id,
                                 uint32_t offset, void *buffer, uint32_t buffer_size)
{
    uint32_t file_data_address;
    uint32_t file_size;
    uint32_t end_offset;

    if (NULL == buffer || 0 == buffer_size)
    {
        MESSAGE_ERROR("flash_fs_partition_read_file: invalid arguments!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (0 != flash_fs_load_file_info(partition, region_id, &file_data_address, &file_size))
    {
        MESSAGE_ERROR("flash_fs_partition_read_file: flash_fs_load_file_info(0x%x) failed!\n",
                      region_id);
        return ERROR_SPI_FLASH_LOAD_FILE_INFO_FAILED;
    }

    if (offset >= file_size)
    {
        MESSAGE_ERROR("flash_fs_partition_read_file: offset too large!\n");
        return ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET;
    }

    end_offset = offset + buffer_size;
    if (end_offset > file_size)
    {
        MESSAGE_ERROR("flash_fs_partition_read_file: end_offset too large!\n");
        return ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET;
    }

    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, file_data_address + offset,
                                   (uint8_t *)buffer, buffer_size))
    {
        MESSAGE_ERROR("flash_fs_partition_read_file: failed to read file data!\n");
        memset(buffer, 0, buffer_size);
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_write_partition
*
*   DESCRIPTION
*
*       This function writes new data to the flash partition.
*
*   INPUTS
*
*       partition_address      address of the partition
*       buffer                 data to be written
*       buffer_size            size of the data buffer
*       chunk_size             size of data to be written to flash at the time (up to 256B)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_write_partition(uint32_t partition_address, void *buffer, uint32_t buffer_size,
                             uint32_t chunk_size)
{
    uint32_t iter_partition_address = partition_address;

    if (NULL == buffer || 0 == buffer_size)
    {
        MESSAGE_ERROR("flash_fs_write_partition: invalid buffer arguments!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (SPI_FLASH_PAGE_SIZE < chunk_size)
    {
        MESSAGE_ERROR("flash_fs_write_partition: invalid chunk_size!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    /* Program the partition with data from the buffer */
    for (uint32_t iter_buffer_size = 0; iter_buffer_size < buffer_size;
         iter_buffer_size += chunk_size, buffer = (uint8_t *)buffer + chunk_size,
                  iter_partition_address = iter_partition_address + chunk_size)
    {
        if (0 != spi_flash_page_program(sg_flash_fs_bl2_info.flash_id, iter_partition_address,
                                        (uint8_t *)buffer, chunk_size))
        {
            MESSAGE_ERROR("flash_fs_write_partition: failed to write data!\n");
            return ERROR_SPI_FLASH_PP_FAILED;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_erase_partition
*
*   DESCRIPTION
*
*       This function erase the data inside flash partition.
*
*   INPUTS
*
*       partition_address      address of the partition
*       partition_size         size of the partition
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_erase_partition(uint32_t partition_address, uint32_t partition_size)
{
    if (0 != (partition_size & SPI_FLASH_BLOCK_MASK))
    {
        MESSAGE_ERROR("spi_flash_erase: partision_size need to be multiple of 64kB!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    /* Erase partition */
    for (uint32_t block_addr = partition_address; block_addr < (partition_address + partition_size);
         block_addr += SPI_FLASH_BLOCK_SIZE)
    {
        if (0 != spi_flash_block_erase(sg_flash_fs_bl2_info.flash_id, block_addr))
        {
            MESSAGE_ERROR("spi_flash_erase: failed to erase block data!\n");
            return ERROR_SPI_FLASH_BE_FAILED;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_update_partition
*
*   DESCRIPTION
*
*       This function erase the data inside flash partition and writes new data.
*
*   INPUTS
*
*       buffer                 data to be written
*       buffer_size            size of the data buffer
*       chunk_size             size of data to be written to flash at the time (up to 256B)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_update_partition(void *buffer, uint64_t buffer_size, uint32_t chunk_size)
{
    uint32_t passive_partition_address;
    uint32_t partition_size;

    partition_size = sg_flash_fs_bl2_info.flash_size / 2;

    /* Check that buffer size is greater than partition size */
    if (buffer_size != partition_size)
    {
        MESSAGE_ERROR("flash_fs_update_partition: failed (update image buffer size is not \
                        equal to partition size)!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    /* Check for active partition and get the passive partition address to store the
     new firmware image */
    if (0 == sg_flash_fs_bl2_info.active_partition)
    {
        passive_partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else if (1 == sg_flash_fs_bl2_info.active_partition)
    {
        passive_partition_address = 0;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    Log_Write(LOG_LEVEL_INFO,
              "passive partition address:%x  partition size:%x  buffer:%lx  buffer_size:%x!\n",
              passive_partition_address, partition_size, (uint64_t)buffer, (uint32_t)buffer_size);

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Erasing affected sectors ...\n");

    if (0 != flash_fs_erase_partition(passive_partition_address, partition_size))
    {
        MESSAGE_ERROR("flash_fs_erase_partition. failed !\n");
        return ERROR_SPI_FLASH_PARTITION_ERASE_FAILED;
    }

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Erase operation completed successfully\n");
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Programming target (%d bytes) ...\n", partition_size);

    if (0 !=
        flash_fs_write_partition(passive_partition_address, buffer, partition_size, chunk_size))
    {
        MESSAGE_ERROR("flash_fs_write_file: failed to write data  passive partition address:%x  \
            buffer:%lx  buffer_size:%x!\n",
                      passive_partition_address, (uint64_t)buffer, (uint32_t)buffer_size);
        return ERROR_SPI_FLASH_PARTITION_PROGRAM_FAILED;
    }

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Target programmed successfully\n");

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_read
*
*   DESCRIPTION
*
*       This function reads the data from given flash partition.
*
*   INPUTS
*
*       active                 read from active partition
*       buffer                 data to be read
*       chunk_size             size of data to be read from flash at the time (up to 256B)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int flash_fs_read(bool active, void *buffer, uint32_t chunk_size, uint32_t offset)
{
    uint32_t partition_address;

    if (SPI_FLASH_PAGE_SIZE < chunk_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_read_partition: invalid chunk_size!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (active)
    {
        /* Check if the partition 0 is active */
        if (sg_flash_fs_bl2_info.active_partition == 0)
        {
            partition_address = 0;
        }
        else /* Second partition (1) is active */
        {
            partition_address = sg_flash_fs_bl2_info.flash_size / 2;
        }
    }
    else
    {
        if (sg_flash_fs_bl2_info.active_partition == 0)
        {
            /* Partition 0 is active which means the passive partition is partition 1,
                use its address */
            partition_address = sg_flash_fs_bl2_info.flash_size / 2;
        }
        else
        {
            partition_address = 0;
        }
    }

    Log_Write(LOG_LEVEL_DEBUG, "Read from partition address:%x  buffer:%lx  chunk_size:%x!\n",
              partition_address, (uint64_t)buffer, chunk_size);

    /* Read the data from the partition */
    if (0 != SPI_Flash_Read_Page(sg_flash_fs_bl2_info.flash_id, partition_address + offset, buffer,
                                 chunk_size))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_read_partition: failed to read data!\n");
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_boot_counters
*
*   DESCRIPTION
*
*       This function reads boot counter reagion from flash and counts zeros
*       to calculate attempted and completed boot counters.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       attempted_boot_counter    attempted boot counter
*       completed_boot_counter    completed boot counter
*
***********************************************************************/

int flash_fs_get_boot_counters(uint32_t *attempted_boot_counter, uint32_t *completed_boot_counter)
{
    ESPERANTO_PARTITION_BL2_INFO_t *partition_info;
    uint32_t partition_address;

    if (0 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = 0;
    }
    else if (1 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    partition_info = &(sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]);

    if (0 != spi_flash_normal_read(
                 sg_flash_fs_bl2_info.flash_id,
                 partition_address +
                     partition_info->regions_table[partition_info->boot_counters_region_index]
                             .region_offset *
                         FLASH_PAGE_SIZE,
                 partition_info->boot_counters_region_data.b, FLASH_PAGE_SIZE))
    {
        MESSAGE_ERROR("flash_fs_scan_regions: error reading boot counter region!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    *attempted_boot_counter =
        count_zero_bits(partition_info->boot_counters_region_data.ull, ULL_PER_PAGE / 2);
    *completed_boot_counter = count_zero_bits(
        partition_info->boot_counters_region_data.ull + ULL_PER_PAGE / 2, ULL_PER_PAGE / 2);

    Log_Write(LOG_LEVEL_CRITICAL, "attempted_boot_counter: %d  completed_boot_counter:%d\n",
              *attempted_boot_counter, *completed_boot_counter);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_increment_completed_boot_count
*
*   DESCRIPTION
*
*       This function increments completed boot counter by writing additional zero
*       in completed boot counter area.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_increment_completed_boot_count(void)
{
    uint32_t partition_address;
    uint32_t region_index;
    uint32_t region_address;
    uint32_t counter_data_address;
    uint32_t increment_offset = 0;
    uint32_t bit_offset = 0;
    uint32_t page_address;
    uint8_t mask;

    if (0 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = 0;
    }
    else if (1 == sg_flash_fs_bl2_info.active_partition)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    region_index = sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                       .boot_counters_region_index;
    region_address = sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;

    counter_data_address = (uint32_t)(partition_address + region_address);

    if (0 != find_first_set_bit_offset(
                 &increment_offset, &bit_offset,
                 sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                         .boot_counters_region_data.ull +
                     ULL_PER_PAGE / 2,
                 ULL_PER_PAGE / 2))
    {
        MESSAGE_ERROR("flash_fs_increment_completed_boot_counter: \
                        attempted counter region is full!\n");
        return ERROR_SPI_FLASH_BOOT_COUNTER_REGION_FULL;
    }

    page_address = increment_offset & 0xFFFFFFF0u;
    Log_Write(LOG_LEVEL_DEBUG, "page_address: 0x%x\n", page_address);
    mask = (uint8_t) ~(1u << bit_offset);

    sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
        .boot_counters_region_data.b[increment_offset] &= mask;

    if (0 != spi_flash_page_program(sg_flash_fs_bl2_info.flash_id,
                                    counter_data_address + page_address + FLASH_PAGE_SIZE / 2,
                                    sg_flash_fs_bl2_info
                                            .partition_info[sg_flash_fs_bl2_info.active_partition]
                                            .boot_counters_region_data.b +
                                        page_address,
                                    16))
    {
        MESSAGE_ERROR("flash_fs_increment_completed_boot_counter: spi_flash_program() failed!\n");
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_swap_primary_boot_partition
*
*   DESCRIPTION
*
*       This function updates boot priority counters by deleting
*       active and inactive boot counter designator area and writing
*       three zeros to inactive boot designator area. That results with
*       priority_counter=0 for active partition and priority_counter=3 for
*       passive partition.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_swap_primary_boot_partition(void)
{
    uint32_t active_partition_address, passive_partition_address;
    uint32_t inactive_partition_index;
    uint32_t passive_partition_priority_counter;
    ESPERANTO_PARTITION_BL2_INFO_t *active_partition_info, *passive_partition_info;

    /* Get Active partition address. */
    if (0 == sg_flash_fs_bl2_info.active_partition)
    {
        active_partition_address = 0;
        passive_partition_address = sg_flash_fs_bl2_info.flash_size / 2;
        inactive_partition_index = 1;
    }
    else if (1 == sg_flash_fs_bl2_info.active_partition)
    {
        active_partition_address = sg_flash_fs_bl2_info.flash_size / 2;
        passive_partition_address = 0;
        inactive_partition_index = 0;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    /* Get Active partition info */
    active_partition_info =
        &(sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]);

    /* Get the passive partition info */
    passive_partition_info = &(sg_flash_fs_bl2_info.partition_info[inactive_partition_index]);

    /* Erase the active partition priority designator area. */
    if (0 !=
        spi_flash_sector_erase(
            sg_flash_fs_bl2_info.flash_id,
            active_partition_address +
                active_partition_info
                        ->regions_table[active_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE))
    {
        MESSAGE_ERROR("spi_flash_sector_erase: failed to erase active priority counter data!\n");
        return ERROR_SPI_FLASH_SE_FAILED;
    }

    /* Erase the passive partition priority designator area. */
    if (0 !=
        spi_flash_sector_erase(
            sg_flash_fs_bl2_info.flash_id,
            passive_partition_address +
                passive_partition_info
                        ->regions_table[passive_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE))
    {
        MESSAGE_ERROR("spi_flash_sector_erase: failed to erase passive priority counter data!\n");
        return ERROR_SPI_FLASH_SE_FAILED;
    }

    /* Set priority of 3 to passive partion by writting 3 zeros. */
    passive_partition_priority_counter = 0xFFFFFFF8;
    if (0 !=
        spi_flash_page_program(
            sg_flash_fs_bl2_info.flash_id,
            passive_partition_address +
                passive_partition_info
                        ->regions_table[passive_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE,
            (uint8_t *)(&passive_partition_priority_counter), 4))
    {
        MESSAGE_ERROR("spi_flash_program: failed to write priority counter data!\n");
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    Log_Write(LOG_LEVEL_CRITICAL,
              "flash_fs_swap_primary_boot_partition: priority counters updated!\n");

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_config_data
*
*   DESCRIPTION
*
*       This function returns ET-SOC configuration data.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       buffer        Pointer to buffer which will hold config data
*
***********************************************************************/

int flash_fs_get_config_data(void *buffer)
{
    if (NULL == buffer)
    {
        MESSAGE_ERROR("flash_fs_get_config_data: buffer points to null\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }
    // Since config data is preloaded we can simply copy it here
    memcpy(buffer, &(sg_flash_fs_bl2_info.asset_config_data), sizeof(ESPERANTO_CONFIG_DATA_t));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_manufacturer_name
*
*   DESCRIPTION
*
*       This function returns ET-SOC manufacturer name.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       mfg_name             manufacturer name
*
***********************************************************************/

int flash_fs_get_manufacturer_name(char *mfg_name)
{
    memcpy(mfg_name, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.manuf_name),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.persistent_config.manuf_name));

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_part_number
*
*   DESCRIPTION
*
*       This function returns ET-SOC part number.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       part_number          part number
*
***********************************************************************/

int flash_fs_get_part_number(char *part_number)
{
    memcpy(part_number, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.part_num),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.persistent_config.part_num));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_set_part_number
*
*   DESCRIPTION
*
*       This function sets ET-SOC part number.
*
*   INPUTS
*
*       part_number          part number
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int flash_fs_set_part_number(uint32_t part_number)
{
    int status = STATUS_SUCCESS;
    uint32_t config_region_address;
    uint32_t scratch_buffer_size;
    ESPERANTO_CONFIG_DATA_t *cfg_data;
    void *scratch_buffer;

    /* Since Flash sector size is 4KB, get scratch buffer to use */
    scratch_buffer = get_scratch_buffer(&scratch_buffer_size);
    if (scratch_buffer_size < SPI_FLASH_SECTOR_SIZE)
    {
        return ERROR_INSUFFICIENT_MEMORY;
    }

    status = get_config_region_address(&config_region_address);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Read the whole sector */
    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, config_region_address,
                                   (uint8_t *)scratch_buffer, SPI_FLASH_SECTOR_SIZE))
    {
        MESSAGE_ERROR("flash_fs_set_part_number: failed to read asset config region!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Update the part_num */
    cfg_data = (ESPERANTO_CONFIG_DATA_t *)(((uint8_t *)scratch_buffer) +
                                           sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t));
    cfg_data->persistent_config.part_num = part_number;

    status = flash_fs_update_sector_and_read_back_for_validation(
        sg_flash_fs_bl2_info.flash_id, config_region_address, scratch_buffer, scratch_buffer_size);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* verify if part number was updated sucessfully, then update it in global data */
    cfg_data = (ESPERANTO_CONFIG_DATA_t *)(((uint8_t *)scratch_buffer) +
                                           sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t));

    if (cfg_data->persistent_config.part_num != part_number)
    {
        MESSAGE_ERROR("flash_fs_set_part_number: part num mismatch!\n");
        return ERROR_SPI_FLASH_BL2_INFO_PARTNUM_MISMATCH;
    }
    else
    {
        sg_flash_fs_bl2_info.asset_config_data.persistent_config.part_num =
            cfg_data->persistent_config.part_num;
    }

    /* Compare with the persistent data in flash with bl2 global data */
    if (memcmp(((uint8_t *)scratch_buffer) + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                   sizeof(ESPERANTO_CONFIG_HEADER_t),
               (uint8_t *)&(sg_flash_fs_bl2_info.asset_config_data.persistent_config),
               sizeof(ESPERANTO_CONFIG_PERSISTENT_DATA_t)))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "flash_fs_set_part_number: persistant data validation failed!\n");
        return ERROR_SPI_FLASH_BL2_INFO_CFG_PERS_MISMATCH;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_serial_number
*
*   DESCRIPTION
*
*       This function returns ET-SOC serial number.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       ser_number          serial number
*
***********************************************************************/

int flash_fs_get_serial_number(char *ser_number)
{
    memcpy(ser_number, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.serial_num),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.persistent_config.serial_num));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_module_rev
*
*   DESCRIPTION
*
*       This function returns ET-SOC module revision.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       module_rev          module revision
*
***********************************************************************/

int flash_fs_get_module_rev(char *module_rev)
{
    memcpy(module_rev, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.module_rev),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.persistent_config.module_rev));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_form_factor
*
*   DESCRIPTION
*
*       This function returns ET-SOC form factor (PCIe or Dual M.2).
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       form_factor          form factor
*
***********************************************************************/

int flash_fs_get_form_factor(char *form_factor)
{
    memcpy(form_factor, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.form_factor),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.persistent_config.form_factor));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_fw_release_rev
*
*   DESCRIPTION
*
*       This function returns ET-SOC firmware release revision.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       fw_release_rev          Firmware release revision
*
***********************************************************************/

int flash_fs_get_fw_release_rev(char *fw_release_rev)
{
    memcpy(fw_release_rev,
           &(sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.fw_release_rev),
           sizeof(sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.fw_release_rev));
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_write_config
*
*   DESCRIPTION
*
*       This function writes config data from global config data.
*
*   INPUTS
*
*       partition partition number to write configuration data
*       write_non_persistant write non persistant configuration from global
*
*   OUTPUTS
*
*       None
*
***********************************************************************/

int flash_fs_write_config_region(uint32_t partition, bool write_non_persistant)
{
    ESPERANTO_CONFIG_DATA_t *cfg_data = NULL;
    uint32_t config_reg_address;
    uint32_t partition_address;
    uint32_t buffer_size;
    void *scratch_buff;

    /* Since Flash sector size is 4KB, get scratch buffer to use */
    scratch_buff = get_scratch_buffer(&buffer_size);
    if (buffer_size < SPI_FLASH_SECTOR_SIZE)
    {
        return ERROR_INSUFFICIENT_MEMORY;
    }

    /* validate partition number and calculate partition address */
    if (partition == 0)
    {
        partition_address = 0;
    }
    else if (partition == 1)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    /* Calculate config data address */
    config_reg_address = partition_address + sg_flash_fs_bl2_info.configuration_region_address;

    /* Read the whole sector */
    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, config_reg_address,
                                   (uint8_t *)scratch_buff, SPI_FLASH_SECTOR_SIZE))
    {
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Check if config data can be perserved due to config region compatibility */
    ESPERANTO_CONFIG_HEADER_t cfg_header;
    memcpy((void *)&cfg_header,
           (void *)((uint8_t *)scratch_buff + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t)),
           sizeof(ESPERANTO_CONFIG_HEADER_t));

    if (sg_flash_fs_bl2_info.asset_config_header.version > cfg_header.version)
    {
        Log_Write(
            LOG_LEVEL_WARNING,
            "Config data version of update fw image is not commpatible with current fw image, data can't be be perserved!\n");
        return 0;
    }

    /* Update the config region data in buffer */
    cfg_data = (ESPERANTO_CONFIG_DATA_t *)(((uint8_t *)scratch_buff) +
                                           sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t));

    /* Restore fixed configuration data from global */
    memcpy((void *)&cfg_data->persistent_config,
           (void *)&sg_flash_fs_bl2_info.asset_config_data.persistent_config,
           sizeof(ESPERANTO_CONFIG_PERSISTENT_DATA_t));

    if (write_non_persistant)
    {
        /* Restore non persistant configuration data from global */
        memcpy((void *)&cfg_data->non_persistent_config,
               (void *)&sg_flash_fs_bl2_info.asset_config_data.non_persistent_config,
               sizeof(ESPERANTO_CONFIG_NON_PERSISTENT_DATA_t));
    }

    /* Erase the asset config region. */
    if (0 != spi_flash_sector_erase(sg_flash_fs_bl2_info.flash_id, config_reg_address))
    {
        return ERROR_SPI_FLASH_SE_FAILED;
    }

    /* Write back the updated sector */
    if (0 != flash_fs_write_partition(config_reg_address, scratch_buff, SPI_FLASH_SECTOR_SIZE,
                                      SPI_FLASH_PAGE_SIZE))
    {
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    memset(scratch_buff, 0, buffer_size);

    /* Read the whole sector again to verify that data written is successfull*/
    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, config_reg_address,
                                   (uint8_t *)scratch_buff, SPI_FLASH_SECTOR_SIZE))
    {
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Compare with the persistent data in flash with bl2 global data */
    if (memcmp(((uint8_t *)scratch_buff) + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                   sizeof(ESPERANTO_CONFIG_HEADER_t),
               (uint8_t *)&(sg_flash_fs_bl2_info.asset_config_data.persistent_config),
               sizeof(ESPERANTO_CONFIG_PERSISTENT_DATA_t)))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "flash_fs_write_config_region: persistant data validation failed!\n");
        return ERROR_FW_UPDATE_WRITE_CFG_REGION_MEMCOMPARE;
    }

    /* Compare with non persistent data in flash with bl2 global data */
    if (write_non_persistant &&
        (memcmp(((uint8_t *)scratch_buff) + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                    sizeof(ESPERANTO_CONFIG_HEADER_t) + sizeof(ESPERANTO_CONFIG_PERSISTENT_DATA_t),
                (uint8_t *)&(sg_flash_fs_bl2_info.asset_config_data.non_persistent_config),
                sizeof(ESPERANTO_CONFIG_NON_PERSISTENT_DATA_t))))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "flash_fs_write_config_region: non persistant data validation failed!\n");
        return ERROR_FW_UPDATE_WRITE_CFG_REGION_MEMCOMPARE;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_update_shire_cache_config
*
*   DESCRIPTION
*
*       This function updates the shire cache configuration with supplied parameters.
*
*   INPUTS
*
*       scp_size scratchpad size
*       l2_size L2 cache size
*       l3_size L3 cache size
*
*   OUTPUTS
*
*       None
*
***********************************************************************/

int flash_fs_update_shire_cache_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size)
{
    /* copy shire cache configuration data from parameters to global */
    sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.scp_size = scp_size;
    sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.l2_size = l2_size;
    sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.l3_size = l3_size;

    /* Update shire cache configuration in flash */
    return flash_fs_write_config_region(sg_flash_fs_bl2_info.active_partition, true);
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_sc_config
*
*   DESCRIPTION
*
*       This function returns shire cache configuration from global flash config.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       sc_cfg          shire cache configuration
*
***********************************************************************/

int flash_fs_get_sc_config(struct shire_cache_config_t *sc_cfg)
{
    if (sc_cfg != NULL)
    {
        /* Retrieve shire cache config from global flash config data*/
        sc_cfg->scp_size = sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.scp_size;
        sc_cfg->l2_size = sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.l2_size;
        sc_cfg->l3_size = sg_flash_fs_bl2_info.asset_config_data.non_persistent_config.l3_size;
    }
    else
    {
        return ERROR_INVALID_ARGUMENT;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_scan_partition
*
*   DESCRIPTION
*
*       This function scans the given partition and loads the partition
*       info in global data.
*
*   INPUTS
*
*       partition    Partition number (0 or 1)
*
*   OUTPUTS
*
*       status       success or error.
*
***********************************************************************/
int flash_fs_scan_partition(uint32_t partition)
{
    uint32_t crc = 0;
    uint32_t partition_address;
    uint32_t partition_size = sg_flash_fs_bl2_info.flash_size / 2;
    ESPERANTO_PARTITION_BL2_INFO_t *partition_info;

    /* Verify the partition number */
    if (0 == partition)
    {
        partition_address = 0;
    }
    else if (1 == partition)
    {
        partition_address = sg_flash_fs_bl2_info.flash_size / 2;
    }
    else
    {
        return ERROR_SPI_FLASH_NO_VALID_PARTITION;
    }

    /* Get the pointer to partition info */
    partition_info = &sg_flash_fs_bl2_info.partition_info[partition];

    /* Read the partiton header from flash */
    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, partition_address,
                                   (uint8_t *)&(partition_info->header),
                                   sizeof(partition_info->header)))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: failed to read header!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Verify the partiton header tag */
    if (ESPERANTO_PARTITION_TAG != partition_info->header.partition_tag)
    {
        Log_Write(
            LOG_LEVEL_ERROR,
            "flash_fs_scan_partition: partition header tag mismatch! (expected %08x, got %08x)\n",
            ESPERANTO_PARTITION_TAG, partition_info->header.partition_tag);
        return ERROR_SPI_FLASH_PARTITION_INVALID_HEADER;
    }

    /* Verify the partition header size */
    if (sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t) != partition_info->header.partition_header_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: partition header size mismatch!\n");
        return ERROR_SPI_FLASH_PARTITION_INVALID_SIZE;
    }

    /* Verify the partition regions size */
    if (sizeof(ESPERANATO_REGION_INFO_t) != partition_info->header.region_info_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: partition region size mismatch!\n");
        return ERROR_SPI_FLASH_PARTITION_INVALID_SIZE;
    }

    /* Check if the regions count in partition is in limits */
    if (partition_info->header.regions_count > ESPERANTO_MAX_REGIONS_COUNT)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: invalid regions count value!\n");
        return ERROR_SPI_FLASH_PARTITION_INVALID_COUNT;
    }

    /* Calculate and verify the partition header checksum */
    crc32(&(partition_info->header),
          offsetof(ESPERANTO_FLASH_PARTITION_HEADER_t, partition_header_checksum), &crc);
    if (crc != partition_info->header.partition_header_checksum)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: partition header CRC mismatch!\n");
        return ERROR_SPI_FLASH_PARTITION_CRC_MISMATCH;
    }

    /* Verify the partiton total size */
    if ((partition_size / FLASH_PAGE_SIZE) != partition_info->header.partition_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: partition size mismatch!\n");
        return ERROR_SPI_FLASH_PARTITION_INVALID_SIZE;
    }

    /* Read the image regions from the flash partition */
    if (0 !=
        spi_flash_normal_read(
            sg_flash_fs_bl2_info.flash_id,
            (uint32_t)(partition_address + sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t)),
            (uint8_t *)(partition_info->regions_table),
            (uint32_t)(partition_info->header.regions_count * sizeof(ESPERANATO_REGION_INFO_t))))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_scan_partition: failed to read regions table!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Scan the parition regions for correctness and load info to global data */
    return flash_fs_scan_regions(partition_size, partition_info);
}

// ***************************************************************
// *** VMI LUT related functions                               ***
// ***************************************************************
int flash_fs_get_vmin_lut(char *vmin_lut)
{
    uint32_t vmin_lut_size_bytes =
        NUMBER_OF_VMIN_LUT_POINTS * sizeof(ESPERANTO_VMIN_LUT_SINGLE_POINT_t);
    memcpy(vmin_lut, &(sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut),
           vmin_lut_size_bytes);
    return 0;
}

int flash_fs_set_vmin_lut(const char *vmin_lut)
{
    int status = STATUS_SUCCESS;
    uint32_t config_region_address;
    uint32_t scratch_buffer_size;
    ESPERANTO_CONFIG_DATA_t *cfg_data;
    void *scratch_buffer;
    uint32_t vmin_lut_size_bytes =
        NUMBER_OF_VMIN_LUT_POINTS * sizeof(ESPERANTO_VMIN_LUT_SINGLE_POINT_t);

    //todo validate lut

    /* Since Flash sector size is 4KB, get scratch buffer to use */
    scratch_buffer = get_scratch_buffer(&scratch_buffer_size);
    if (scratch_buffer_size < SPI_FLASH_SECTOR_SIZE)
    {
        return ERROR_INSUFFICIENT_MEMORY;
    }

    status = get_config_region_address(&config_region_address);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Read the whole sector */
    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info.flash_id, config_region_address,
                                   (uint8_t *)scratch_buffer, SPI_FLASH_SECTOR_SIZE))
    {
        MESSAGE_ERROR("flash_fs_set_vmin_lut_boot_voltages: failed to read asset config region!\n");
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    /* Update the vmin lut*/
    cfg_data = (ESPERANTO_CONFIG_DATA_t *)(((uint8_t *)scratch_buffer) +
                                           sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t));
    memcpy(cfg_data->persistent_config.vmin_lut, vmin_lut, vmin_lut_size_bytes);

    status = flash_fs_update_sector_and_read_back_for_validation(
        sg_flash_fs_bl2_info.flash_id, config_region_address, scratch_buffer, scratch_buffer_size);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* verify if voltage was updated sucessfully, then update it in global data */
    cfg_data = (ESPERANTO_CONFIG_DATA_t *)(((uint8_t *)scratch_buffer) +
                                           sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                                           sizeof(ESPERANTO_CONFIG_HEADER_t));
    if (memcmp(cfg_data->persistent_config.vmin_lut, vmin_lut, vmin_lut_size_bytes) == 0)
    {
        memcpy(sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut, vmin_lut,
               vmin_lut_size_bytes);
    }
    else
    {
        MESSAGE_ERROR("flash_fs_set_vmin_lut: vmin lut mismatch!\n");
        return ERROR_SPI_FLASH_BL2_INFO_VMIN_MISMATCH;
    }

    /* Compare the persistent data in flash with bl2 global data */
    if (memcmp(((uint8_t *)scratch_buffer) + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) +
                   sizeof(ESPERANTO_CONFIG_HEADER_t),
               (uint8_t *)&(sg_flash_fs_bl2_info.asset_config_data.persistent_config),
               sizeof(ESPERANTO_CONFIG_PERSISTENT_DATA_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_set_vmin_lut: persistant data validation failed!\n");
        return ERROR_SPI_FLASH_BL2_INFO_CFG_PERS_MISMATCH;
    }

    return status;
}

//NOTE: Boot frequency and voltage values are stored at [0] index of VMIN LUT.
//This will be changed in the future.

uint16_t flash_fs_get_mnn_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].mnn.freq;
}

uint8_t flash_fs_get_mnn_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].mnn.volt;
}

uint16_t flash_fs_get_sram_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].sram.freq;
}

uint8_t flash_fs_get_sram_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].sram.volt;
}

uint16_t flash_fs_get_noc_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].noc.freq;
}

uint8_t flash_fs_get_noc_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].noc.volt;
}

uint16_t flash_fs_get_pcl_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].pcl.freq;
}

uint8_t flash_fs_get_pcl_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].pcl.volt;
}

uint16_t flash_fs_get_ddr_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].ddr.freq;
}

uint8_t flash_fs_get_ddr_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].ddr.volt;
}

uint16_t flash_fs_get_mxn_boot_freq(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].mxn.freq;
}

uint8_t flash_fs_get_mxn_boot_voltage(void)
{
    return sg_flash_fs_bl2_info.asset_config_data.persistent_config.vmin_lut[0].mxn.volt;
}

#pragma GCC pop_options
