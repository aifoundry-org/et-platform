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

#ifndef __BL2_FIRMWARE_UPDATE_H__
#define __BL2_FIRMWARE_UPDATE_H__

#include <stdint.h>

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
/*! \file bl2_firmware_update.h
    \brief A C header that defines the Firmware update service's
    public interfaces. These interfaces provide services using which
    the host can issue firmware update/status commands to device.
*/
/***********************************************************************/
#include "dm.h"
#include "bl2_build_configuration.h"
#include "bl2_firmware_loader.h"
#include "bl2_flash_fs.h"
#include "bl2_main.h"
#include "etsoc_hal/inc/hal_device.h"
#include "bl2_pmic_controller.h"
#include "bl2_crypto.h"
#include "bl2_vaultip_driver.h"

/**
 * @brief Enum of Firmware update status
 */
enum DEVICE_FW_UPDATE_STATUS {
    DEVICE_FW_FLASH_UPDATE_SUCCESS = 0,
    DEVICE_FW_FLASH_UPDATE_ERROR = 1,
    DEVICE_FW_FLASH_PRIORITY_COUNTER_SWAP_ERROR = 2,
    DEVICE_FW_UPDATED_IMAGE_BOOT_FAILED = 3
};

//Release 0.0.7 TODO: This needs to be retrieved from Device Interface registers

/*! \def DEVICE_FW_UPDATE_REGION_SIZE
    \brief Device Firmware update memory region size.
*/
#define DEVICE_FW_UPDATE_REGION_SIZE 0x0400000UL

/*! \def DEVICE_FW_UPDATE_REGION_BASE
    \brief Device Firmware update memory region base address.
*/
#define DEVICE_FW_UPDATE_REGION_BASE 0x8005120000ULL

//
/*! \def FORMAT_VERSION
    \brief Macro for formatting FW version. It will be of the format:  Major[1 byte].[Minor 1 byte].[Revision 1 byte].[NULL].
*/
#define FORMAT_VERSION(major, minor, revision) ((major << 24) | (minor << 16) | (revision << 8))

/*! \fn void firmware_service_process_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
    \brief Interface to process the firmware service command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \param buffer Pointer to Command Payload buffer
    \returns none
*/
void firmware_service_process_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

#endif
