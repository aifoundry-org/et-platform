/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_firmware_update.h
    \brief A C header that defines the Firmware update service's
    public interfaces. These interfaces provide services using which
    the host can issue firmware update/status commands to device.
*/
/***********************************************************************/
#ifndef __BL2_FIRMWARE_UPDATE_H__
#define __BL2_FIRMWARE_UPDATE_H__

#include <stdint.h>
#include "dm.h"
#include "bl2_firmware_loader.h"
#include "bl2_flash_fs.h"
#include "bl2_main.h"
#include "bl2_pmic_controller.h"
#include "bl2_crypto.h"
#include "bl2_vaultip_driver.h"
#include "sp_host_iface.h"
#include "bl2_reset.h"

#include "hwinc/hal_device.h"

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

/*! \fn void firmware_service_get_mm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
    \brief Function to get Master Minion FW version
    \param major FW major version
    \param minor FW minor version
    \param revision FW revision version
    \returns none
*/
void firmware_service_get_mm_version(uint8_t *major, uint8_t *minor, uint8_t *revision);

/*! \fn void firmware_service_get_wm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
    \brief Function to get Worker Minion FW version
    \param major FW major version
    \param minor FW minor version
    \param revision FW revision version
    \returns none
*/
void firmware_service_get_wm_version(uint8_t *major, uint8_t *minor, uint8_t *revision);

/*! \fn void firmware_service_get_machm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
    \brief Function to get Machine Minion FW version
    \param major FW major version
    \param minor FW minor version
    \param revision FW revision version
    \returns none
*/
void firmware_service_get_machm_version(uint8_t *major, uint8_t *minor, uint8_t *revision);

/*! \fn int32_t sp_get_image_version_info(void)
    \brief Function to get image FW version
    \param none
    \returns image FW revision version
*/
uint32_t sp_get_image_version_info(void);

#endif
