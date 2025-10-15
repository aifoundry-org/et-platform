/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file asset_track_mgmt.c
    \brief A C module that implements functions to retrieve
    the Asset Tracking Metrics

    Public interfaces:
    get_manufacturer_name
    get_part_number
    set_part_number
    get_serial_number
    get_chip_revision
    get_PCIE_speed
    get_module_rev
    get_form_factor
    get_memory_details
    get_memory_size
    get_memory_type
*/
/***********************************************************************/
#include "bl2_asset_trk_mgmt.h"

int get_manufacturer_name(char *mfg_name)
{
    return flash_fs_get_manufacturer_name(mfg_name);
}

int get_part_number(char *part_number)
{
    return flash_fs_get_part_number(part_number);
}

int set_part_number(uint32_t part_number)
{
    return flash_fs_set_part_number(part_number);
}

int get_serial_number(char *ser_number)
{
    // Retreive ECID from Fuse
    ecid_t ecid;
    read_ecid(&ecid);
    memcpy(ser_number, &ecid, sizeof(ecid_t));

    return 0;
}

int get_chip_revision(char *chip_rev)
{
    return otp_get_chip_revision(chip_rev);
}

int get_PCIE_speed(char *pcie_speed)
{
    return pcie_get_speed(pcie_speed);
}

int get_module_rev(char *module_rev)
{
    return flash_fs_get_module_rev(module_rev);
}

int get_form_factor(char *form_factor)
{
    return flash_fs_get_form_factor(form_factor);
}

int get_memory_vendor_ID(char *vendor_ID)
{
    uint32_t vendor_id;

    /* Get the vendor ID */
    ddr_get_memory_vendor_ID(&vendor_id);
    memcpy(vendor_ID, &vendor_id, sizeof(uint32_t));

    return 0;
}

int get_memory_size(char *mem_size)
{
    uint64_t ddr_mem_size;

    /* Get the memory size in bytes */
    ddr_get_memory_size(&ddr_mem_size);

    /* Convert it to GB */
    ddr_mem_size = ddr_mem_size / 1024 / 1024 / 1024;
    memcpy(mem_size, &ddr_mem_size, sizeof(uint64_t));

    return 0;
}

int get_memory_type(char *mem_type)
{
    return ddr_get_memory_type(mem_type);
}
