/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include <string.h>
#include <stddef.h>

#include "etsoc/drivers/serial/serial.h"
#include "printx.h"
#include "crc32.h"
#include "bl1_spi_controller.h"
#include "bl1_spi_flash.h"
#include "bl1_flash_fs.h"
#include "jedec_sfdp.h"

#pragma GCC push_options
//#pragma GCC optimize ("O2")
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define INVALID_REGION_INDEX         0xFFFFFFFF
#define MAXIMUM_FAILED_BOOT_ATTEMPTS 3
#define USE_SFDP
#define PAGE_PROGRAM_TIMEOUT 2000

static FLASH_FS_BL1_INFO_t *sg_flash_fs_bl1_info = NULL;

static int flash_fs_scan_regions(uint32_t partition_size,
                                 ESPERANTO_PARTITION_BL1_INFO_t *partition_info)
{
    uint32_t crc;
    uint32_t region_offset_end;
    uint32_t partition_size_in_blocks = partition_size / FLASH_PAGE_SIZE;

    partition_info->sp_bl2_region_index = INVALID_REGION_INDEX;

    for (uint32_t n = 0; n < partition_info->header.regions_count; n++)
    {
        switch (partition_info->regions_table[n].region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_SP_BL2:
                break;
            default:
                // printx("flash_fs_scan_regions: ignoring region %u.\n", n);
                continue;
        }

        crc = 0;
        crc32(&(partition_info->regions_table[n]),
              offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum), &crc);
        if (crc != partition_info->regions_table[n].region_info_checksum)
        {
            printx("flash_fs_scan_regions: region %u CRC mismatch! (expected %08x, got %08x)\n", n,
                   partition_info->regions_table[n].region_info_checksum, crc);
            return -1;
        }

        if (0 == partition_info->regions_table[n].region_offset ||
            partition_info->regions_table[n].region_offset >= partition_size_in_blocks)
        {
            printx("flash_fs_scan_regions: invalid region %u offset!\n", n);
            return -1;
        }
        if (0 == partition_info->regions_table[n].region_reserved_size)
        {
            printx("flash_fs_scan_regions: region %u has zero size!\n", n);
            return -1;
        }
        region_offset_end = partition_info->regions_table[n].region_offset +
                            partition_info->regions_table[n].region_reserved_size;
        if (region_offset_end < partition_info->regions_table[n].region_offset)
        {
            printx("flash_fs_scan_regions: region %u offset/size overflow!\n", n);
            return -1; // integer overflow
        }
        if (region_offset_end > partition_size_in_blocks)
        {
            printx("flash_fs_scan_regions: invalid region %u size!\n", n);
            return -1; // integer overflow
        }

        switch (partition_info->regions_table[n].region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_SP_BL2:
                partition_info->sp_bl2_region_index = n;
                break;
            default:
                return -1; // we should never get here
        }
    }

    return 0;
}

static int flash_fs_scan_partition(uint32_t partition_size,
                                   ESPERANTO_PARTITION_BL1_INFO_t *partition_info)
{
    return flash_fs_scan_regions(partition_size, partition_info);
}

int flash_fs_init(FLASH_FS_BL1_INFO_t *flash_fs_bl1_info)
{
    uint32_t partition_size;

    if (NULL == flash_fs_bl1_info)
    {
        printx("flash_fs_init: invalid argument!\n");
        return -1;
    }

    sg_flash_fs_bl1_info = NULL;

    partition_size = flash_fs_bl1_info->flash_size / 2;

    // re-scan both partitions in the flash
    for (uint32_t n = 0; n < 2; n++)
    {
        //printx("Re-scanning partition %u...\n", n);

        if (false == flash_fs_bl1_info->partition_info[n].partition_valid)
        {
            printx("Partition %u is not valid, skipping\n", n);
            continue;
        }

        if (0 != flash_fs_scan_partition(partition_size, &flash_fs_bl1_info->partition_info[n]))
        {
            printx("Partition %u seems corrupted.\n", n);
        }
        else if (INVALID_REGION_INDEX == flash_fs_bl1_info->partition_info[n].sp_bl2_region_index)
        {
            printx("sp bl2 not found!\n");
        }
        else
        {
            continue;
        }

        flash_fs_bl1_info->partition_info[n].partition_valid = false;
        if (flash_fs_bl1_info->other_partition_valid == 1 &&
            flash_fs_bl1_info->active_partition == n)
        {
            flash_fs_bl1_info->active_partition = 1 - n;
            printx("Partition %u is now the active partition.\n", 1 - n);
        }
        flash_fs_bl1_info->other_partition_valid = 0;
    }

    if (false ==
        flash_fs_bl1_info->partition_info[flash_fs_bl1_info->active_partition].partition_valid)
    {
        // the active boot partition is no longer valid!
        printx("No valid partition found!\n");
        return -1;
    }

    sg_flash_fs_bl1_info = flash_fs_bl1_info;
    return 0;
}

static int flash_fs_load_file_info(ESPERANTO_FLASH_REGION_ID_t region_id,
                                   uint32_t *file_data_address, uint32_t *file_size)
{
    uint32_t crc;
    uint32_t partition_address;
    ESPERANATO_FILE_INFO_t *file_info = NULL;
    uint32_t region_index;
    uint32_t region_address;
    uint32_t region_size;

    if (NULL == sg_flash_fs_bl1_info)
    {
        return -1;
    }

    if (0 == sg_flash_fs_bl1_info->active_partition)
    {
        partition_address = 0;
    }
    else if (1 == sg_flash_fs_bl1_info->active_partition)
    {
        partition_address = sg_flash_fs_bl1_info->flash_size / 2;
    }
    else
    {
        return -1;
    }

    switch (region_id)
    {
        case ESPERANTO_FLASH_REGION_ID_SP_BL2:
            file_info = &(sg_flash_fs_bl1_info->sp_bl2_file_info);
            region_index =
                sg_flash_fs_bl1_info->partition_info[sg_flash_fs_bl1_info->active_partition]
                    .sp_bl2_region_index;
            break;
        default:
            return -1;
    }

    if (INVALID_REGION_INDEX == region_index)
    {
        return -1;
    }

    //printx("flash_fs_load_file_info(%x)\n", region_id);
    region_address = sg_flash_fs_bl1_info->partition_info[sg_flash_fs_bl1_info->active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;
    region_size = sg_flash_fs_bl1_info->partition_info[sg_flash_fs_bl1_info->active_partition]
                      .regions_table[region_index]
                      .region_reserved_size *
                  FLASH_PAGE_SIZE;
    //printx("reg_addr: %x\n", region_address);
    //printx("reg_size: %u\n", region_size);

    if (0 == file_info->file_header_tag && 0 == file_info->file_header_size &&
        0 == file_info->file_size && 0 == file_info->file_header_crc)
    {
        if (0 != spi_flash_normal_read(sg_flash_fs_bl1_info->flash_id,
                                       partition_address + region_address, (uint8_t *)file_info,
                                       sizeof(ESPERANATO_FILE_INFO_t)))
        {
            printx("flash_fs_load_file_info: failed to read file info!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (ESPERANTO_FILE_TAG != file_info->file_header_tag)
        {
            printx("flash_fs_load_file_info: invalid file info header tag!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (sizeof(ESPERANATO_FILE_INFO_t) != file_info->file_header_size)
        {
            printx("flash_fs_load_file_info: invalid file info header size!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        crc = 0;
        crc32(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc), &crc);
        if (crc != file_info->file_header_crc)
        {
            printx("flash_fs_load_file_info: file info CRC mismatch!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t)))
        {
            printx("flash_fs_load_file_info: invalid file size!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }
    }

    *file_data_address =
        (uint32_t)(partition_address + region_address + sizeof(ESPERANATO_FILE_INFO_t));
    *file_size = file_info->file_size;

    return 0;
}

int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size)
{
    uint32_t file_data_address;
    uint32_t file_size;

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size))
    {
        printx("flash_fs_get_file_size: flash_fs_load_file_info(0x%x) failed!\n", region_id);
        return -1;
    }

    *size = file_size;
    return 0;
}

int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size)
{
    uint32_t file_data_address;
    uint32_t file_size;
    uint32_t end_offset;

    if (NULL == buffer || 0 == buffer_size)
    {
        printx("flash_fs_read_file: invalid arguments!\n");
        return -1;
    }

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size))
    {
        printx("flash_fs_read_file: flash_fs_load_file_info(0x%x) failed!\n", region_id);
        return -1;
    }
    //printx("file data address: 0x%x\n", file_data_address);

    if (offset >= file_size)
    {
        printx("flash_fs_read_file: offset too large!\n");
        return -1;
    }

    end_offset = offset + buffer_size;
    if (end_offset < buffer_size)
    {
        printx("flash_fs_read_file: end_offset integer overflow!\n");
        return -1;
    }
    if (end_offset > file_size)
    {
        printx("flash_fs_read_file: end_offset too large!\n");
        return -1;
    }

    if (0 != spi_flash_normal_read(sg_flash_fs_bl1_info->flash_id, file_data_address + offset,
                                   (uint8_t *)buffer, buffer_size))
    {
        printx("flash_fs_read_file: failed to read file data!\n");
        memset(buffer, 0, buffer_size);
        return -1;
    }

    return 0;
}

#pragma GCC pop_options
