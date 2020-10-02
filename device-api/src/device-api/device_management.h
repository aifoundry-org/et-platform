/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_MANAGEMENT_H
#define ET_DEVICE_MANAGEMENT_H

#include "esperanto/device-api/device_api_message_types.h"

#include <stdint.h>


#define MAX_LENGTH 8  

// FIXME:TODO
// The following structs need to be defined and generated similar to Device API

/// @brief Device Management command

/// @brief Device Management reply
struct dm_rsp_t {
  struct response_header_t dm_response_info;
  char rsp_char[MAX_LENGTH];
};


// Device management commands ID's

// Device management commands:  Asset tracking service commands
#define DM_SVC_ASSET_GET_FW_VERSION      0x1
#define DM_SVC_ASSET_GET_MFG_NAME        0x2
#define DM_SVC_ASSET_GET_PART_NUMBER     0x3
#define DM_SVC_ASSET_GET_SERIAL_NUMBER   0x4
#define DM_SVC_ASSET_GET_CHIP_REVISION   0x5
#define DM_SVC_ASSET_GET_PCIE_PORTS      0x6
#define DM_SVC_ASSET_GET_MODULE_REV      0x7
#define DM_SVC_ASSET_GET_FORM_FACTOR     0x8
#define DM_SVC_ASSET_GET_MEMORY_DETAILS  0x9
#define DM_SVC_ASSET_GET_MEMORY_SIZE     0x10
#define DM_SVC_ASSET_GET_MEMORY_TYPE     0x11


// Asset sizes in bytes
#define FW_VERSION_MAX_LENGTH     0x8
#define MFG_NAME_MAX_LENGTH       0x8
#define PART_NUMBER_MAX_LENGTH    0x8
#define SERIAL_NUMBER_MAX_LENGTH  0x8 
#define MEM_VENDOR_MAX_LENGTH     0x8
#define MEM_PART_MAX_LENGTH       0x8
#define MEM_TYPE_MAX_LENGTH       0x8
#define FORM_FACTOR_MAX_LENGTH    0x8
#define MODULE_REV_MAX_LENGTH     0x8   

#endif // ET_DEVICE_MANAGEMENT_H
