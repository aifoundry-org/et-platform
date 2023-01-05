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
/*! \file bl2_flash_fs.h
    \brief A C header that defines the flash file system's
    public interfaces. These interfaces provide services using which
    the host can issue file read/write commands.
*/
/***********************************************************************/

#ifndef __BL2_SPI_FLASH_FS_H__
#define __BL2_SPI_FLASH_FS_H__

#include "bl2_spi_flash.h"

/*! \fn int flash_fs_init(FLASH_FS_BL2_INFO_t *flash_fs_bl2_info)
    \brief This function initialize partition infos.
    \param flash_fs_bl2_info - bl2 flash info struct to initialize
    \return The function call status, pass/fail.
*/
int flash_fs_init(FLASH_FS_BL2_INFO_t *flash_fs_bl2_info);

/*! \fn int flash_fs_set_active_partition(bool primary)
    \brief UNIMPLEMENTED
*/
int flash_fs_get_config_data(void *buffer);

/*! \fn int flash_fs_set_active_partition(bool primary)
    \brief UNIMPLEMENTED
*/
int flash_fs_get_config_data(void *buffer);

/*! \fn int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size)
    \brief This function returns file size for a particular region.
    \param region_id - region id
    \param size - size of file data
    \return The function call status, pass/fail.
*/
int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size);

/*! \fn int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset,
*                              void *buffer, uint32_t buffer_size)
    \brief This function reads the data from the file of the particular region.
    \param region_id - region id
    \param offset - offset inside file to start read from
    \param buffer - data read from file
    \param buffer_size - size in bytes to be read
    \return The function call status, pass/fail.
*/
int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size);

/*! \fn int flash_fs_write_partition(uint32_t partition_address, void *buffer,
*                                    uint32_t buffer_size, uint32_t chunk_size)
    \brief This function writes new data to the flash partition.
    \param partition_address - address of the partition
    \param buffer - data to be written
    \param buffer_size - size of the data buffer
    \param chunk_size - size of data to be written to flash at the time (up to 256B)
    \return The function call status, pass/fail.
*/
int flash_fs_write_partition(uint32_t partition_address, void *buffer, uint32_t buffer_size,
                             uint32_t chunk_size);

/*! \fn int flash_fs_erase_partition(uint32_t partition_address, uint32_t partition_size)
    \brief This function erase the data inside flash partition.
    \param partition_address - address of the partition
    \param partition_size - size of the partition
    \return The function call status, pass/fail.
*/
int flash_fs_erase_partition(uint32_t partition_address, uint32_t partition_size);

/*! \fn int flash_fs_update_partition(void *buffer, uint64_t buffer_size, uint32_t chunk_size)
    \brief This function erase the data inside flash partition and writes new data.
    \param buffer - data to be written
    \param buffer_size - size of the data buffer
    \param chunk_size - size of data to be written to flash at the time (up to 256B)
    \return The function call status, pass/fail.
*/
int flash_fs_update_partition(void *buffer, uint64_t buffer_size, uint32_t chunk_size);

/*! \fn int flash_fs_read(bool active, void *buffer, uint64_t buffer_size, uint32_t chunk_size)
    \brief This function reads the data from give flash partition
    \param active - true for active partition else false
    \param buffer - data to be written
    \param chunk_size - size of data to be written to flash at the time (up to 256B)
    \return The function call status, pass/fail.
*/
int flash_fs_read(bool active, void *buffer, uint32_t chunk_size, uint32_t offset);

/*! \fn int flash_fs_swap_primary_boot_partition(void)
    \brief This function updates boot priority counters by deleting
*       active and inactive boot counter designator area and writing
*       three zeros to inactive boot designator area. That results with
*       priority_counter=0 for active partition and priority_counter=3 for
*       passive partition.
    \return The function call status, pass/fail.
*/
int flash_fs_swap_primary_boot_partition(void);

/*! \fn int flash_fs_get_boot_counters(uint32_t *attempted_boot_counter,
*                                      uint32_t *completed_boot_counter)
    \brief This function reads boot counter reagion from flash and counts zeros
*       to calculate attempted and completed boot counters.
    \param attempted_boot_counter - attempted boot counter
    \param completed_boot_counter - completed boot counter
    \return The function call status, pass/fail.
*/
int flash_fs_get_boot_counters(uint32_t *attempted_boot_counter, uint32_t *completed_boot_counter);

/*! \fn int flash_fs_increment_completed_boot_count(void)
    \brief This function increments completed boot counter by writing additional zero
*       in completed boot counter area.
    \return The function call status, pass/fail.
*/
int flash_fs_increment_completed_boot_count(void);

/*! \fn int flash_fs_increment_attempted_boot_count(void)
    \brief This function increments attempted boot counter by writing additional zero
*       in attempted boot counter area.
    \return The function call status, pass/fail.
*/
int flash_fs_increment_attempted_boot_count(void);

/*! \fn int flash_fs_get_manufacturer_name(char *mfg_name, size_t size)
    \brief This function returns ET-SOC manufacturer name.
    \param mfg_name - manufacturer name
    \param size - size of the name
    \return The function call status, pass/fail.
*/
int flash_fs_get_manufacturer_name(char *mfg_name);

/*! \fn int flash_fs_get_part_number(char *part_number, size_t size)
    \brief This function returns ET-SOC part number.
    \param part_number - part number
    \param size - size of part number
    \return The function call status, pass/fail.
*/
int flash_fs_get_part_number(char *part_number);

/*! \fn int flash_fs_set_part_number(uint32_t part_number)
    \brief This function sets ET-SOC part number.
    \param part_number - part number
    \return The function call status, pass/fail.
*/
int flash_fs_set_part_number(uint32_t part_number);

/*! \fn int flash_fs_get_serial_number(char *ser_number, )
    \brief This function returns ET-SOC serial number.
    \param ser_number - serial number
    \param size - size of the serial number
    \return The function call status, pass/fail.
*/
int flash_fs_get_serial_number(char *ser_number);

/*! \fn int flash_fs_get_module_rev(char *module_rev, size_t size)
    \brief This function returns ET-SOC module revision.
    \param module_rev - module revision
    \param size - size of the revision
    \return The function call status, pass/fail.
*/
int flash_fs_get_module_rev(char *module_rev);

/*! \fn int flash_fs_get_form_factor(char *form_factor, size_t size)
    \brief This function returns ET-SOC form factor (PCIe or Dual M.2).
    \param form_factor - form factor
    \param size - size of the form factor
    \return The function call status, pass/fail.
*/
int flash_fs_get_form_factor(char *form_factor);

/*! \fn int flash_fs_get_fw_release_rev(char *fw_release_rev)
    \brief This function returns ET-SOC firmware release revision.
    \param fw_release_rev - to get firmware release revision
    \return The function call status, pass/fail.
*/
int flash_fs_get_fw_release_rev(char *fw_release_rev);

/*! \fn int flash_fs_write_config_region(uint32_t partition)
    \brief This function writes config data from global config data.
    \param partition partition number to write configuration data
    \return The function call status, pass/fail.
*/
int flash_fs_write_config_region(uint32_t partition);

#endif
