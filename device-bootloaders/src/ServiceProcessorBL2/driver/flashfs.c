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
        flash_fs_get_config_data
        flash_fs_get_config_data
        flash_fs_get_file_size
        flash_fs_read_file
        flash_fs_write_partition
        flash_fs_erase_partition
        flash_fs_update_partition
        flash_fs_swap_primary_boot_partition
        flash_fs_get_boot_counters
        flash_fs_increment_completed_boot_count
        flash_fs_increment_attempted_boot_count
        flash_fs_get_manufacturer_name
        flash_fs_get_part_number
        flash_fs_get_serial_number
        flash_fs_get_module_rev
        flash_fs_get_memory_size
        flash_fs_get_form_factor
*/
/***********************************************************************/

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "log.h"
#include "serial.h"
#include "crc32.h"
#include "bl2_spi_controller.h"
#include "bl2_spi_flash.h"
#include "bl2_flash_fs.h"
#include "jedec_sfdp.h"
#include "bl2_main.h"
#include "bl_error_code.h"

#pragma GCC push_options
/* #pragma GCC optimize ("O2") */
#pragma GCC diagnostic ignored "-Wswitch-enum"

/*! \def INVALID_REGION_INDEX
    \brief invalid region index value.
*/
#define INVALID_REGION_INDEX         0xFFFFFFFF

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

static FLASH_FS_BL2_INFO_t sg_flash_fs_bl2_info = { 0 };

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
    uint32_t index;
    int count = 0;

    for (index = 0; index < data_size; index++)
    {
        count += __builtin_popcountll(data[index]);
    }

    return (data_size * (uint32_t)sizeof(unsigned long long) * 8u) - (uint32_t)count;
}

/************************************************************************
*
*   FUNCTION
*
*       find_first_unset_bit_offset
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

union
{
    unsigned long long ull;
    uint8_t u8[sizeof(unsigned long long)];
} value_uu;
static int find_first_unset_bit_offset(uint32_t *offset, uint32_t *bit,
                                       const unsigned long long *ull_array, uint32_t ull_array_size)
{
    uint32_t n, b_index, ull_index, flag;
    const unsigned long long *data = ull_array;
    const unsigned long long *ull_array_end = ull_array + ull_array_size;

    while (data < ull_array_end)
    {
        if (0 != *data)
        {
            ull_index = (uint32_t)(data - ull_array);
            value_uu.ull = *data;
            for (b_index = 0; b_index < sizeof(unsigned long long); b_index++)
            {
                if (0 != value_uu.u8[b_index])
                {
                    for (n = 0; n < 8; n++)
                    {
                        flag = 0x01u << n;
                        if (flag & value_uu.u8[b_index])
                        {
                            *offset = b_index + (uint32_t)(sizeof(unsigned long long) * ull_index);
                            *bit = n;
                            return 0;
                        }
                    }
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
    uint32_t n;
    uint32_t region_offset_end;
    uint32_t partition_size_in_blocks = partition_size / FLASH_PAGE_SIZE;

    partition_info->sp_bl2_region_index = INVALID_REGION_INDEX;

    for (n = 0; n < partition_info->header.regions_count; n++)
    {
        switch (partition_info->regions_table[n].region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
            case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
            case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
            case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
                break;
            default:
                Log_Write(LOG_LEVEL_WARNING, "flash_fs_scan_regions: ignoring region %u.\n", n);
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
*       flash_fs_scan_partition
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

static int flash_fs_scan_partition(uint32_t partition_size,
                                   ESPERANTO_PARTITION_BL2_INFO_t *partition_info)
{
    return flash_fs_scan_regions(partition_size, partition_info);
}

/************************************************************************
*
*   FUNCTION
*
*       init_partition_info_data
*
*   DESCRIPTION
*
*       This function initialize bl2 partition info struct by copying data
*       from bl1 partition info struct
*
*   INPUTS
*
*       bl1_partition_info   bl1 partition info struct
*
*   OUTPUTS
*
*       bl2_partition_info   bl2 partition info struct
*
***********************************************************************/

static int
init_partition_info_data(ESPERANTO_PARTITION_BL2_INFO_t *restrict bl2_partition_info,
                         const ESPERANTO_PARTITION_BL1_INFO_t *restrict bl1_partition_info)
{
    if (NULL == bl2_partition_info || NULL == bl1_partition_info)
    {
        MESSAGE_ERROR("init_partition_info_data: invalid arguments!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    memcpy(&(bl2_partition_info->header), &(bl1_partition_info->header),
           sizeof(bl1_partition_info->header));
    memcpy(&(bl2_partition_info->regions_table), &(bl1_partition_info->regions_table),
           sizeof(bl1_partition_info->regions_table));

    bl2_partition_info->priority_designator_region_index =
        bl1_partition_info->priority_designator_region_index;
    bl2_partition_info->boot_counters_region_index =
        bl1_partition_info->boot_counters_region_index;
    bl2_partition_info->configuration_data_region_index =
        bl1_partition_info->configuration_data_region_index;
    bl2_partition_info->vaultip_fw_region_index = bl1_partition_info->vaultip_fw_region_index;
    bl2_partition_info->pcie_config_region_index = bl1_partition_info->pcie_config_region_index;
    bl2_partition_info->sp_certificates_region_index =
        bl1_partition_info->sp_certificates_region_index;
    bl2_partition_info->sp_bl1_region_index = bl1_partition_info->sp_bl1_region_index;
    bl2_partition_info->sp_bl2_region_index = bl1_partition_info->sp_bl2_region_index;

    memcpy(&(bl2_partition_info->priority_designator_region_data),
           &(bl1_partition_info->priority_designator_region_data),
           sizeof(bl1_partition_info->priority_designator_region_data));
    memcpy(&(bl2_partition_info->boot_counters_region_data),
           &(bl1_partition_info->boot_counters_region_data),
           sizeof(bl1_partition_info->boot_counters_region_data));

    bl2_partition_info->priority_counter = bl1_partition_info->priority_counter;
    bl2_partition_info->attempted_boot_counter = bl1_partition_info->attempted_boot_counter;
    bl2_partition_info->completed_boot_counter = bl1_partition_info->completed_boot_counter;
    bl2_partition_info->partition_valid = bl1_partition_info->partition_valid;

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
*       This function initialize bl2 flash info struct by copying data
*       from bl1 flash info struct, also initialize partition infos.
*
*   INPUTS
*
*       flash_fs_bl1_info    bl1 partition info struct
*
*   OUTPUTS
*
*       flash_fs_bl2_info    bl2 partition info struct
*
***********************************************************************/

int flash_fs_init(FLASH_FS_BL2_INFO_t *restrict flash_fs_bl2_info,
                  const FLASH_FS_BL1_INFO_t *restrict flash_fs_bl1_info)
{
    uint32_t n;
    uint32_t partition_size;

    if (NULL == flash_fs_bl2_info || NULL == flash_fs_bl1_info)
    {
        MESSAGE_ERROR("flash_fs_init: invalid arguments!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    memset(flash_fs_bl2_info, 0, sizeof(FLASH_FS_BL2_INFO_t));

    for (n = 0; n < 2; n++)
    {
        if (0 != init_partition_info_data(&(flash_fs_bl2_info->partition_info[n]),
                                          &(flash_fs_bl1_info->partition_info[n])))
        {
            MESSAGE_ERROR("flash_fs_init: init_partition_info_data(%u) failed!\n", n);
            return ERROR_SPI_FLASH_FS_INIT_FAILED;
        }
    }

    flash_fs_bl2_info->flash_id_u32 = flash_fs_bl1_info->flash_id_u32;
    flash_fs_bl2_info->flash_size = flash_fs_bl1_info->flash_size;
    flash_fs_bl2_info->active_partition = flash_fs_bl1_info->active_partition;
    flash_fs_bl2_info->other_partition_valid = flash_fs_bl1_info->other_partition_valid;
    flash_fs_bl2_info->configuration_region_address =
        flash_fs_bl1_info->configuration_region_address;
    flash_fs_bl2_info->pcie_config_file_info = flash_fs_bl1_info->pcie_config_file_info;
    flash_fs_bl2_info->vaultip_firmware_file_info = flash_fs_bl1_info->vaultip_firmware_file_info;
    flash_fs_bl2_info->sp_certificates_file_info = flash_fs_bl1_info->sp_certificates_file_info;
    flash_fs_bl2_info->sp_bl1_file_info = flash_fs_bl1_info->sp_bl1_file_info;
    flash_fs_bl2_info->sp_bl2_file_info = flash_fs_bl1_info->sp_bl2_file_info;

    partition_size = flash_fs_bl1_info->flash_size / 2;

    /* re-scan both partitions in the flash */
    for (n = 0; n < 2; n++)
    {
        Log_Write(LOG_LEVEL_INFO, "Re-scanning partition %u...\n", n);

        if (0 != flash_fs_scan_partition(partition_size, &flash_fs_bl2_info->partition_info[n]))
        {
            Log_Write(LOG_LEVEL_ERROR, "Partition %u seems corrupted.\n", n);
            flash_fs_bl2_info->partition_info[n].partition_valid = false;
        }
        else
        {
            /* test if the critical required files are present in the partition */
            if (INVALID_REGION_INDEX ==
                flash_fs_bl2_info->partition_info[n].dram_training_region_index)
            {
                Log_Write(LOG_LEVEL_ERROR, "DRAM training data not found!\n");
                flash_fs_bl2_info->partition_info[n].partition_valid = false;
            }
        }
    }

    if (false ==
        flash_fs_bl1_info->partition_info[flash_fs_bl1_info->active_partition].partition_valid)
    {
        /* the active boot partition is no longer valid! */
        MESSAGE_ERROR("No valid partition found!\n");
        return ERROR_SPI_FLASH_FS_INIT_FAILED;
    }

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
*       This function loads file info for a particular region
*
*   INPUTS
*
*       region_id            region id
*
*   OUTPUTS
*
*       file_data_address    address of file data
*       file_size            size of file data
*
***********************************************************************/

static int flash_fs_load_file_info(ESPERANTO_FLASH_REGION_ID_t region_id,
                                   uint32_t *file_data_address, uint32_t *file_size)
{
    uint32_t crc;
    uint32_t partition_address;
    ESPERANATO_FILE_INFO_t *file_info = NULL;
    uint32_t region_index;
    uint32_t region_address;
    uint32_t region_size;

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

    switch (region_id)
    {
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            file_info = &(sg_flash_fs_bl2_info.dram_training_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                            .dram_training_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
            file_info = &(sg_flash_fs_bl2_info.machine_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                            .machine_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
            file_info = &(sg_flash_fs_bl2_info.master_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                            .master_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
            file_info = &(sg_flash_fs_bl2_info.worker_minion_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                            .worker_minion_region_index;
            break;
        case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
            file_info = &(sg_flash_fs_bl2_info.maxion_bl1_file_info);
            region_index =
                sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                            .maxion_bl1_region_index;
            break;
        default:
            return ERROR_SPI_FLASH_INVALID_REGION_ID;
    }

    if (INVALID_REGION_INDEX == region_index)
    {
        return ERROR_SPI_FLASH_INVALID_REGION_ID;
    }

    region_address = sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;
    region_size = sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
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

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size))
    {
        MESSAGE_ERROR("flash_fs_get_file_size: flash_fs_load_file_info(0x%x) failed!\n",
                         region_id);
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

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size))
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
    if (end_offset < buffer_size)
    {
        MESSAGE_ERROR("flash_fs_read_file: end_offset integer overflow!\n");
        return ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET;
    }
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
    for (uint32_t block_addr = partition_address;
         block_addr <= (partition_address + partition_size); block_addr += SPI_FLASH_BLOCK_SIZE)
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
    if (buffer_size < partition_size)
    {
        MESSAGE_ERROR("flash_fs_update_partition: failed (image buffer size is smaller \
                        than partition size)!\n");
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

    Log_Write(LOG_LEVEL_ERROR, "passive partition address:%x  partition size:%x  buffer:%lx  buffer_size:%x!\n",
           passive_partition_address, partition_size, (uint64_t)buffer, (uint32_t)buffer_size);

    if (0 != flash_fs_erase_partition(passive_partition_address, partition_size))
    {
        MESSAGE_ERROR("flash_fs_erase_partition. failed !\n");
        return ERROR_SPI_FLASH_PARTITION_ERASE_FAILED;
    }

    if (0 !=
        flash_fs_write_partition(passive_partition_address, buffer, partition_size, chunk_size))
    {
        MESSAGE_ERROR("flash_fs_write_file: failed to write data  passive partition address:%x  \
            buffer:%lx  buffer_size:%x!\n",
                      passive_partition_address, (uint64_t)buffer, (uint32_t)buffer_size);
        return ERROR_SPI_FLASH_PARTITION_PROGRAM_FAILED;
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

    partition_info =
        &(sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]);

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

    Log_Write(LOG_LEVEL_ERROR, "attempted_boot_counter: %d  completed_boot_counter:%d\n", *attempted_boot_counter,
           *completed_boot_counter);

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
    uint32_t increment_offset, bit_offset;
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

    /*Log_Write(LOG_LEVEL_ERROR, "partition address: %x, region_index: %x, region_address: %x\n", partition_address,
           region_index, region_address); */

    counter_data_address = (uint32_t)(partition_address + region_address);

    if (0 != find_first_unset_bit_offset(
                 &increment_offset, &bit_offset,
                 sg_flash_fs_bl2_info.partition_info[sg_flash_fs_bl2_info.active_partition]
                         .boot_counters_region_data.ull +
                     FLASH_PAGE_SIZE / 2,
                 FLASH_PAGE_SIZE / 2))
    {
        MESSAGE_ERROR("flash_fs_increment_completed_boot_counter: \
                        attempted counter region is full!\n");
        return ERROR_SPI_FLASH_BOOT_COUNTER_REGION_FULL;
    }

    page_address = increment_offset & 0xFFFFFFF0u;
    /* Log_Write(LOG_LEVEL_ERROR, "page_address: 0x%x\n", page_address); */
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

    Log_Write(LOG_LEVEL_ERROR, "flash_fs_swap_primary_boot_partition: priority counters updated!\n");

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

int flash_fs_get_manufacturer_name(char *mfg_name, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    char name[] = "Esperanto Technologies";
    snprintf(mfg_name, size, "%s", name);

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

int flash_fs_get_part_number(char *part_number, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    char name[] = "ETPART1";
    snprintf(part_number, size, "%s", name);

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

int flash_fs_get_serial_number(char *ser_number, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    char name[] = "ETSER_1";
    snprintf(ser_number, size, "%s", name);

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

int flash_fs_get_module_rev(char *module_rev, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    uint64_t revision = 1;
    snprintf(module_rev, size, "%ld", revision);
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       flash_fs_get_memory_size
*
*   DESCRIPTION
*
*       This function returns ET-SOC memory size in bytes.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       mem_size          memory size
*
***********************************************************************/

int flash_fs_get_memory_size(char *mem_size, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    uint64_t m_size = 16 * 1024;
    snprintf(mem_size, size, "%ld", m_size);
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

int flash_fs_get_form_factor(char *form_factor, size_t size)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-4327 */
    strncpy(form_factor, "Dual_M2", size);
    return 0;
}

#pragma GCC pop_options
