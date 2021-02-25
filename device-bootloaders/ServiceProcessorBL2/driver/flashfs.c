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
#include <stdio.h>

#include "serial.h"
#include "crc32.h"
#include "bl2_spi_controller.h"
#include "bl2_spi_flash.h"
#include "bl2_flash_fs.h"
#include "jedec_sfdp.h"

#pragma GCC push_options
//#pragma GCC optimize ("O2")
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define INVALID_REGION_INDEX         0xFFFFFFFF
#define MAXIMUM_FAILED_BOOT_ATTEMPTS 3
#define USE_SFDP
#define PAGE_PROGRAM_TIMEOUT 2000

static FLASH_FS_BL2_INFO_t *sg_flash_fs_bl2_info = NULL;

static uint32_t count_zero_bits(const unsigned long long *data, uint32_t data_size)
{
    uint32_t index;
    int count = 0;

    for (index = 0; index < data_size; index++) {
        count += __builtin_popcountll(data[index]);
    }

    return (data_size * (uint32_t)sizeof(unsigned long long) * 8u) - (uint32_t)count;
}

union {
    unsigned long long ull;
    uint8_t u8[sizeof(unsigned long long)];
} value_uu;
static int find_first_unset_bit_offset(uint32_t *offset, uint32_t *bit,
                                       const unsigned long long *ull_array, uint32_t ull_array_size)
{
    uint32_t n, b_index, ull_index, flag;
    const unsigned long long *data = ull_array;
    const unsigned long long *ull_array_end = ull_array + ull_array_size;

    while (data < ull_array_end) {
        if (0 != *data) {
            ull_index = (uint32_t)(data - ull_array);
            value_uu.ull = *data;
            for (b_index = 0; b_index < sizeof(unsigned long long); b_index++) {
                if (0 != value_uu.u8[b_index]) {
                    for (n = 0; n < 8; n++) {
                        flag = 0x01u << n;
                        if (flag & value_uu.u8[b_index]) {
                            *offset = b_index + (uint32_t)(sizeof(unsigned long long) * ull_index);
                            *bit = n;
                            return 0;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

static int flash_fs_scan_regions(uint32_t partition_size,
                                 ESPERANTO_PARTITION_BL2_INFO_t *partition_info)
{
    uint32_t crc;
    uint32_t n;
    uint32_t region_offset_end;
    uint32_t partition_size_in_blocks = partition_size / FLASH_PAGE_SIZE;

    partition_info->sp_bl2_region_index = INVALID_REGION_INDEX;

    for (n = 0; n < partition_info->header.regions_count; n++) {
        switch (partition_info->regions_table[n].region_id) {
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
        case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
        case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
        case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
        case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
            break;
        default:
            // printf("flash_fs_scan_regions: ignoring region %u.\n", n);
            continue;
        }

        crc = 0;
        crc32(&(partition_info->regions_table[n]),
              offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum), &crc);
        if (crc != partition_info->regions_table[n].region_info_checksum) {
            printf("flash_fs_scan_regions: region %u CRC mismatch! (expected %08x, got %08x)\n", n,
                   partition_info->regions_table[n].region_info_checksum, crc);
            return -1;
        }

        if (0 == partition_info->regions_table[n].region_offset ||
            partition_info->regions_table[n].region_offset >= partition_size_in_blocks) {
            printf("flash_fs_scan_regions: invalid region %u offset!\n", n);
            return -1;
        }
        if (0 == partition_info->regions_table[n].region_reserved_size) {
            printf("flash_fs_scan_regions: region %u has zero size!\n", n);
            return -1;
        }
        region_offset_end = partition_info->regions_table[n].region_offset +
                            partition_info->regions_table[n].region_reserved_size;
        if (region_offset_end < partition_info->regions_table[n].region_offset) {
            printf("flash_fs_scan_regions: region %u offset/size overflow!\n", n);
            return -1; // integer overflow
        }
        if (region_offset_end > partition_size_in_blocks) {
            printf("flash_fs_scan_regions: invalid region %u size!\n", n);
            return -1; // integer overflow
        }

        switch (partition_info->regions_table[n].region_id) {
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
            return -1; // we should never get here
        }
    }

    return 0;
}

static int flash_fs_scan_partition(uint32_t partition_size,
                                   ESPERANTO_PARTITION_BL2_INFO_t *partition_info)
{
    return flash_fs_scan_regions(partition_size, partition_info);
}

static int
init_partition_info_data(ESPERANTO_PARTITION_BL2_INFO_t *restrict bl2_partition_info,
                         const ESPERANTO_PARTITION_BL1_INFO_t *restrict bl1_partition_info)
{
    if (NULL == bl2_partition_info || NULL == bl1_partition_info) {
        printf("init_partition_info_data: invalid arguments!\n");
        return -1;
    }

    memcpy(&(bl2_partition_info->header), &(bl1_partition_info->header),
           sizeof(bl1_partition_info->header));
    memcpy(&(bl2_partition_info->regions_table), &(bl1_partition_info->regions_table),
           sizeof(bl1_partition_info->regions_table));

    bl2_partition_info->priority_designator_region_index =
        bl1_partition_info->priority_designator_region_index;
    bl2_partition_info->boot_counters_region_index = bl1_partition_info->boot_counters_region_index;
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

int flash_fs_init(FLASH_FS_BL2_INFO_t *restrict flash_fs_bl2_info,
                  const FLASH_FS_BL1_INFO_t *restrict flash_fs_bl1_info)
{
    uint32_t n;
    uint32_t partition_size;

    if (NULL == flash_fs_bl2_info || NULL == flash_fs_bl1_info) {
        printf("flash_fs_init: invalid arguments!\n");
        return -1;
    }

    sg_flash_fs_bl2_info = NULL;
    memset(flash_fs_bl2_info, 0, sizeof(FLASH_FS_BL2_INFO_t));

    for (n = 0; n < 2; n++) {
        if (0 != init_partition_info_data(&(flash_fs_bl2_info->partition_info[n]),
                                          &(flash_fs_bl1_info->partition_info[n]))) {
            printf("flash_fs_init: init_partition_info_data(%u) failed!\n", n);
            return -1;
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

    // re-scan both partitions in the flash
    for (n = 0; n < 2; n++) {
        printf("Re-scanning partition %u...\n", n);

        if (0 != flash_fs_scan_partition(partition_size, &flash_fs_bl2_info->partition_info[n])) {
            printf("Partition %u seems corrupted.\n", n);
            flash_fs_bl2_info->partition_info[n].partition_valid = false;
        } else {
            // test if the critical required files are present in the partition
            if (INVALID_REGION_INDEX ==
                flash_fs_bl2_info->partition_info[n].dram_training_region_index) {
                printf("DRAM training data not found!\n");
                flash_fs_bl2_info->partition_info[n].partition_valid = false;
            }
        }
    }

    if (false ==
        flash_fs_bl1_info->partition_info[flash_fs_bl1_info->active_partition].partition_valid) {
        // the active boot partition is no longer valid!
        printf("No valid partition found!\n");
        return -1;
    }

    sg_flash_fs_bl2_info = flash_fs_bl2_info;
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

    if (NULL == sg_flash_fs_bl2_info) {
        return -1;
    }

    if (0 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = 0;
    } else if (1 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = sg_flash_fs_bl2_info->flash_size / 2;
    } else {
        return -1;
    }

    switch (region_id) {
    case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
        file_info = &(sg_flash_fs_bl2_info->dram_training_file_info);
        region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .dram_training_region_index;
        break;
    case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
        file_info = &(sg_flash_fs_bl2_info->machine_minion_file_info);
        region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .machine_minion_region_index;
        break;
    case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
        file_info = &(sg_flash_fs_bl2_info->master_minion_file_info);
        region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .master_minion_region_index;
        break;
    case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
        file_info = &(sg_flash_fs_bl2_info->worker_minion_file_info);
        region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .worker_minion_region_index;
        break;
    case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
        file_info = &(sg_flash_fs_bl2_info->maxion_bl1_file_info);
        region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .maxion_bl1_region_index;
        break;
    default:
        return -1;
    }

    if (INVALID_REGION_INDEX == region_index) {
        return -1;
    }

    region_address = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;
    region_size = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                      .regions_table[region_index]
                      .region_reserved_size *
                  FLASH_PAGE_SIZE;

    if (0 == file_info->file_header_tag && 0 == file_info->file_header_size &&
        0 == file_info->file_size && 0 == file_info->file_header_crc) {
        if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info->flash_id,
                                       partition_address + region_address, (uint8_t *)file_info,
                                       sizeof(ESPERANATO_FILE_INFO_t))) {
            printf("flash_fs_load_file_info: failed to read file info!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (ESPERANTO_FILE_TAG != file_info->file_header_tag) {
            printf("flash_fs_load_file_info: invalid file info header tag!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (sizeof(ESPERANATO_FILE_INFO_t) != file_info->file_header_size) {
            printf("flash_fs_load_file_info: invalid file info header size!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        crc = 0;
        crc32(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc), &crc);
        if (crc != file_info->file_header_crc) {
            printf("flash_fs_load_file_info: file info CRC mismatch!\n");
            memset(file_info, 0, sizeof(ESPERANATO_FILE_INFO_t));
            return -1;
        }

        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t))) {
            printf("flash_fs_load_file_info: invalid file size!\n");
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

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size)) {
        printf("flash_fs_get_file_size: flash_fs_load_file_info(0x%x) failed!\n", region_id);
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

    if (NULL == buffer || 0 == buffer_size) {
        printf("flash_fs_read_file: invalid arguments!\n");
        return -1;
    }

    if (0 != flash_fs_load_file_info(region_id, &file_data_address, &file_size)) {
        printf("flash_fs_read_file: flash_fs_load_file_info(0x%x) failed!\n", region_id);
        return -1;
    }
    //printf("file data address: 0x%x\n", file_data_address);

    if (offset >= file_size) {
        printf("flash_fs_read_file: offset too large!\n");
        return -1;
    }

    end_offset = offset + buffer_size;
    if (end_offset < buffer_size) {
        printf("flash_fs_read_file: end_offset integer overflow!\n");
        return -1;
    }
    if (end_offset > file_size) {
        printf("flash_fs_read_file: end_offset too large!\n");
        return -1;
    }

    if (0 != spi_flash_normal_read(sg_flash_fs_bl2_info->flash_id, file_data_address + offset,
                                   (uint8_t *)buffer, buffer_size)) {
        printf("flash_fs_read_file: failed to read file data!\n");
        memset(buffer, 0, buffer_size);
        return -1;
    }

    return 0;
}

int flash_fs_write_partition(uint32_t partition_address, void *buffer, uint32_t buffer_size)
{
    if (NULL == buffer || 0 == buffer_size) {
        printf("flash_fs_write_file: invalid arguments!\n");
        return -1;
    }

    // Program the partition with data from the buffer
    if (0 != spi_flash_program(sg_flash_fs_bl2_info->flash_id, partition_address, (uint8_t *)buffer,
                               buffer_size)) {
        printf("spi_flash_program: failed to write data!\n");
        return -1;
    }

    return 0;
}

int flash_fs_erase_partition(uint32_t partition_address, uint32_t partition_size)
{
    // Erase partition
    if (0 != spi_flash_erase(sg_flash_fs_bl2_info->flash_id, partition_address, partition_size))
    {
        printf("spi_flash_erase: failed to erase data!\n");
        return -1;
    }

    return 0;
}


int flash_update_partition(void *buffer, uint64_t buffer_size)
{
    uint32_t passive_partition_address;

    // Check for active partition and get the passive partition address to store the new firmware image
    if (0 == sg_flash_fs_bl2_info->active_partition) {
        passive_partition_address = sg_flash_fs_bl2_info->flash_size / 2;
    } else if (1 == sg_flash_fs_bl2_info->active_partition) {
        passive_partition_address = 0;
    } else {
        return -1;
    }

    printf("passive partition address:%x  buffer:%lx  buffer_size:%x!\n",
            passive_partition_address, (uint64_t)buffer, (uint32_t)buffer_size);
    if (0 != flash_fs_write_partition(passive_partition_address, buffer, (uint32_t)buffer_size)) {
        printf(
            "flash_fs_write_file: failed to write data  passive partition address:%x  buffer:%lx  buffer_size:%x!\n",
            passive_partition_address, (uint64_t)buffer, (uint32_t)buffer_size);
        return -1;
    }

    return 0;
}

int flash_fs_get_boot_counters(uint32_t *attempted_boot_counter, uint32_t *completed_boot_counter)
{
    ESPERANTO_PARTITION_BL2_INFO_t *partition_info;
    uint32_t partition_address;

    if (0 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = 0;
    } else if (1 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = sg_flash_fs_bl2_info->flash_size / 2;
    } else {
        return -1;
    }

    partition_info =
        &(sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]);

    if (0 != spi_flash_normal_read(
                 sg_flash_fs_bl2_info->flash_id,
                 partition_address +
                     partition_info->regions_table[partition_info->boot_counters_region_index]
                             .region_offset *
                         FLASH_PAGE_SIZE,
                 partition_info->boot_counters_region_data.b, FLASH_PAGE_SIZE)) {
        printf("flash_fs_scan_regions: error reading boot counter region!\n");
        return -1;
    }

    *attempted_boot_counter =
        count_zero_bits(partition_info->boot_counters_region_data.ull, ULL_PER_PAGE / 2);
    *completed_boot_counter = count_zero_bits(
        partition_info->boot_counters_region_data.ull + ULL_PER_PAGE / 2, ULL_PER_PAGE / 2);

    printf("attempted_boot_counter: %d  completed_boot_counter:%d\n", *attempted_boot_counter,
           *completed_boot_counter);

    return 0;
}

int flash_fs_increment_completed_boot_count(void)
{
    uint32_t partition_address;
    uint32_t region_index;
    uint32_t region_address;
    uint32_t counter_data_address;
    uint32_t increment_offset, bit_offset;
    uint32_t page_address;
    uint8_t mask;
    uint8_t spi_status;
    uint32_t k;

    if (NULL == sg_flash_fs_bl2_info) {
        return -1;
    }

    if (0 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = 0;
    } else if (1 == sg_flash_fs_bl2_info->active_partition) {
        partition_address = sg_flash_fs_bl2_info->flash_size / 2;
    } else {
        return -1;
    }

    region_index = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                       .boot_counters_region_index;
    region_address = sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;

    printf("partition address: %x, region_index: %x, region_address: %x\n", partition_address,
           region_index, region_address);

    counter_data_address = (uint32_t)(partition_address + region_address);

    if (0 != find_first_unset_bit_offset(
                 &increment_offset, &bit_offset,
                 sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                         .boot_counters_region_data.ull +
                     FLASH_PAGE_SIZE / 2,
                 FLASH_PAGE_SIZE / 2)) {
        printf("flash_fs_increment_completed_boot_counter: attempted counter region is full!\n");
        return -1;
    }
    printf("First unset increment offset: 0x%x, bit offset: 0x%x\n", increment_offset, bit_offset);

    page_address = increment_offset & 0xFFFFFFF0u;
    printf("page_address: 0x%x\n", page_address);

    mask = (uint8_t) ~(1u << bit_offset);

    printf("Original data: %02x\n",
           sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
               .boot_counters_region_data.b[increment_offset]);

    sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
        .boot_counters_region_data.b[increment_offset] &= mask;

    printf("Updated data: %02x\n",
           sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
               .boot_counters_region_data.b[increment_offset]);

    printf("Writing 16 bytes @ %lx\n",
           (uintptr_t)(sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                           .boot_counters_region_data.b +
                       page_address));
    for (uint32_t n = 0; n < 16; n++) {
        printf(" %02x", sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]
                            .boot_counters_region_data.b[page_address + n]);
    }
    printf("\n to flash address 0x%x\n", counter_data_address + page_address + FLASH_PAGE_SIZE/2);

    if (0 != spi_flash_rdsr(sg_flash_fs_bl2_info->flash_id, &spi_status)) {
        printf("flash_fs_increment_attempted_boot_count: spi_flash_rdsr() failed!\n");
        return -1;
    }
    if (0 != spi_status) {
        printf("flash_fs_increment_attempted_boot_count: warning - SPI flash status is 0x%02x!\n", spi_status);
    }

    if (0 != spi_flash_wren(sg_flash_fs_bl2_info->flash_id)) {
        printf("flash_fs_increment_attempted_boot_count: spi_flash_wren() failed!\n");
        return -1;
    }

    if (0 != spi_flash_program(sg_flash_fs_bl2_info->flash_id, counter_data_address + page_address + FLASH_PAGE_SIZE/2,
                               sg_flash_fs_bl2_info
                                       ->partition_info[sg_flash_fs_bl2_info->active_partition]
                                       .boot_counters_region_data.b +
                                   page_address,
                               16)) {
        printf("flash_fs_increment_completed_boot_counter: spi_flash_program() failed!\n");
        return -1;
    }

    if (0 != spi_flash_rdsr(sg_flash_fs_bl2_info->flash_id, &spi_status)) {
        printf("flash_fs_increment_completed_boot_counter: spi_flash_rdsr() failed!\n");
        return -1;
    }

    k = 0;
    while (1 & spi_status) {
        k++;
        if (k > 2000) {
            printf("timeout waiting for page program to finish!\n");
            return -1;
        }
        if (0 != spi_flash_rdsr(sg_flash_fs_bl2_info->flash_id, &spi_status)) {
            printf("spi_flash_rdsr() failed!\n");
            return -1;
        }
    }

    return 0;
}

int flash_fs_swap_priority_counter(void)
{
    uint32_t partition_size;
    uint32_t active_partition_address, passive_partition_address;
    uint32_t inactive_partition_index;
    uint8_t active_partition_priority_counter;
    uint8_t passive_partition_priority_counter;
    ESPERANTO_PARTITION_BL2_INFO_t *active_partition_info, *passive_partition_info;

    // Retrieve the partition size
    partition_size = (sg_flash_fs_bl2_info->flash_size / 2);

    // Get Active partition address.
    if (0 == sg_flash_fs_bl2_info->active_partition) {
        active_partition_address = 0;
        passive_partition_address = sg_flash_fs_bl2_info->flash_size / 2;
        inactive_partition_index = 1;
    } else if (1 == sg_flash_fs_bl2_info->active_partition) {
        active_partition_address = sg_flash_fs_bl2_info->flash_size / 2;
        passive_partition_address = 0;
        inactive_partition_index = 0;
    } else {
        return -1;
    }

    // Get Active partition info
    active_partition_info =
        &(sg_flash_fs_bl2_info->partition_info[sg_flash_fs_bl2_info->active_partition]);

    // Read the priority counter from the active partition.
    if (0 !=
        spi_flash_normal_read(
            sg_flash_fs_bl2_info->flash_id,
            active_partition_address +
                active_partition_info
                        ->regions_table[active_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE,
            active_partition_info->priority_designator_region_data.b, FLASH_PAGE_SIZE)) {
        printf("spi_flash_normal_read: failed to read priority designator region!\n");
        return -1;
    }

    active_partition_priority_counter = (uint8_t)(
        count_zero_bits(active_partition_info->priority_designator_region_data.ull, ULL_PER_PAGE));

    printf(
        "flash_partition_swap_priority_counter- active_partition_index:%d, partition_address:%d, partition_size:%d, priority_counter: %d\n",
        sg_flash_fs_bl2_info->active_partition, active_partition_address, partition_size,
        active_partition_priority_counter);

    // Get the passive partition info
    passive_partition_info = &(sg_flash_fs_bl2_info->partition_info[inactive_partition_index]);

    // Read the priority counter from passive partition.
    if (0 !=
        spi_flash_normal_read(
            sg_flash_fs_bl2_info->flash_id,
            passive_partition_address +
                passive_partition_info
                        ->regions_table[passive_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE,
            passive_partition_info->priority_designator_region_data.b, FLASH_PAGE_SIZE)) {
        printf("spi_flash_normal_read: failed to read priority designator region!\n");
        return -1;
    }

    passive_partition_priority_counter = (uint8_t)(
        count_zero_bits(passive_partition_info->priority_designator_region_data.ull, ULL_PER_PAGE));

    printf(
        "flash_partition_swap_priority_counter- inactive_partition_index:%d, passive_partition_address:%d, partition_size:%d,  passive_partition_priority_counter: %d\n",
        inactive_partition_index, passive_partition_address, partition_size,
        passive_partition_priority_counter);

    // Update the passive_partition_priority_counter value into active partition of the flash.
    if (0 !=
        spi_flash_program(
            sg_flash_fs_bl2_info->flash_id,
            active_partition_address +
                active_partition_info
                        ->regions_table[active_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE,
            &passive_partition_priority_counter, FLASH_PAGE_SIZE)) {
        printf("spi_flash_program: failed to write priority counter data!\n");
        return -1;
    }

    // Update the active_partition_priority_counter value into passive partition of the flash.
    if (0 !=
        spi_flash_program(
            sg_flash_fs_bl2_info->flash_id,
            passive_partition_address +
                passive_partition_info
                        ->regions_table[passive_partition_info->priority_designator_region_index]
                        .region_offset *
                    FLASH_PAGE_SIZE,
            &active_partition_priority_counter, FLASH_PAGE_SIZE)) {
        printf("spi_flash_program: failed to write priority counter data!\n");
        return -1;
    }

    return 0;
}

#pragma GCC pop_options
