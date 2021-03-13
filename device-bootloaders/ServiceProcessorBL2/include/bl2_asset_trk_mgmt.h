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
/*! \file asset.h
    \brief A C header that provides abstraction for Asset tracking service's
    interfaces. These interfaces provide services using which
    the host can query device for asset details.
*/
/***********************************************************************/
#ifndef ASSET_H
#define ASSET_H

#include "dm.h"
#include "bl2_flash_fs.h"
#include "bl2_otp.h"
#include "bl2_pcie.h"
#include "bl2_ddr_init.h"

/*! \fn int get_manufacturer_name(char *mfg_name)
    \brief Interface to get the manufacturer name.
    \param *mfg_name  Pointer to mfg name variable
    \returns Status indicating success or negative error
*/
int get_manufacturer_name(char *mfg_name);

/*! \fn int get_part_number(char *part_number)
    \brief Interface to get the Part number.
    \param *part_number  Pointer to part number variable
    \returns Status indicating success or negative error
*/
int get_part_number(char *part_number);

/*! \fn int get_serial_number(char *ser_number)
    \brief Interface to get the Module's Serial Number
    \param *ser_number  Pointer to Serial number
    \returns Status indicating success or negative error
*/
int get_serial_number(char *ser_number);

/*! \fn int get_chip_revision(char *chip_rev)
    \brief Interface to get the Chip revision info
    \param *chip_rev  Pointer to chip revision variable
    \returns Status indicating success or negative error
*/
int get_chip_revision(char *chip_rev);

/*! \fn int get_module_rev(char *module_rev)
    \brief Interface to get the module revision
    \param *module_rev  Pointer to module revision variable
    \returns Status indicating success or negative error
*/
int get_module_rev(char *module_rev);

/*! \fn int get_form_factor(char *form_factor)
    \brief Interface to get the form factor
    \param *form_factor  Pointer to form factor variable
    \returns Status indicating success or negative error
*/
int get_form_factor(char *form_factor);

/*! \fn int get_PCIE_speed(char *pcie_speed)
    \brief Interface to get the PCIE speed
    \param *pcie_speed  Pointer to pcie speed variable
    \returns Status indicating success or negative error
*/
int get_PCIE_speed(char *pcie_speed);

/*! \fn int get_memory_details(char *mem_vendor, char *mem_part)
    \brief Interface to get the memory details.
    \param *mem_vendor  Pointer to Memory vendor variable
    \param *mem_part  Pointer to Memory part number variable
    \returns Status indicating success or negative error
*/
int get_memory_details(char *mem_vendor, char *mem_part);

/*! \fn int get_memory_size(char *mem_size)
    \brief Interface to get the memory size
    \param *mem_size  Pointer to memory size variable
    \returns Status indicating success or negative error
*/
int get_memory_size(char *mem_size);

/*! \fn int get_memory_type(char *mem_type)
    \brief Interface to get the memory type info
    \param *mem_type  Pointer to memory type variable
    \returns Status indicating success or negative error
*/
int get_memory_type(char *mem_type);

#endif