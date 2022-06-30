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
/*! \file asset_track_mgmt.c
    \brief A C module that implements functions to retrieve
    the Asset Tracking Metrics

    Public interfaces:
    get_manufacturer_name
    get_part_number
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
    /* TODO: reading the registers here hang the system. To be fixed later.
    return ddr_get_memory_vendor_ID(vendor_ID); */
    char id[] = "1234567";
    snprintf(vendor_ID, 8, "%s", id);

    return 0;
}

int get_memory_size(char *mem_size)
{
    return flash_fs_get_memory_size(mem_size);
}

int get_memory_type(char *mem_type)
{
    return ddr_get_memory_type(mem_type);
}
