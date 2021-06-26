/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file firmware_update.c
    \brief A C module that implements the Firmware update services

    Public interfaces:
        firmware_service_process_request
*/
/***********************************************************************/

#include "bl2_firmware_update.h"

#define SPI_FLASH_WRITES_256B_CHUNK_SIZE 256
#define SPI_FLASH_WRITES_128B_CHUNK_SIZE 128
#define SPI_FLASH_WRITES_64B_CHUNK_SIZE  64

static void reset_etsoc(void)
{
    Log_Write(LOG_LEVEL_INFO, "Resetting ETSOC..!\n");

    // Now Reset SP.
    release_etsoc_reset();
}

static int32_t dm_svc_get_firmware_status(void)
{
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    if (0 != flash_fs_get_boot_counters(&attempted_boot_counter, &completed_boot_counter)) {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_get_boot_counters: failed to get boot counters!\n");
    } else {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_get_boot_counters: Success!\n");
        if (attempted_boot_counter != completed_boot_counter) {
            Log_Write(LOG_LEVEL_ERROR,
                "flash_fs_get_boot_counters: Attempted and completed boot counter do not match!\n");
            return DEVICE_FW_UPDATED_IMAGE_BOOT_FAILED;
        }
    }

    // Return the firmware boot status as success
    return 0;
}

/*  TODO: This feature is to be supported in v 0.0.7
static int32_t dm_svc_set_firmware_version_counter(void)
{
    int32_t ret = 0;

    // Release 0.0.7
    // SW-4553 implements this and is blocked by
    // SW-4570  FW Update Dev: BL2 driver to update the vault IP & SP fuses for monotonic counters

    return ret;
}
*/

/* TODO: This feature is to be supported in v 0.0.7
static int32_t dm_svc_set_firmware_valid_counter(void)
{
    int32_t ret = 0;
    // Release 0.0.7
    // SW-4546 FW Update Dev: Implement device side runtime firmware validation

    return ret;
}
*/

/* TODO: This feature is to be supported in v 0.0.7
static int32_t update_sw_boot_root_certificate_hash(char *hash)
{
    (void)hash;

    //TODO : SW-5133 SP BL2 Vault IP driver needs to provide support for provisioning SW BOOT ROOT Certificate and hash.

    return 0;
}

static int32_t dm_svc_update_sw_boot_root_certificate_hash(struct dm_control_block *dm_req)
{
   Log_Write(LOG_LEVEL_INFO, "hash :%s\n", dm_req->cmd_payload);

   //cmd_payload contains the hash for new certificate.
   if(0 != update_sw_boot_root_certificate_hash(dm_req->cmd_payload)) {
        Log_Write(LOG_LEVEL_ERROR, " dm_svc_update_sp_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
        return -1;
   }

   return 0;
}
*/

static int32_t update_sp_boot_root_certificate_hash(char *key_blob, char* associated_data)
{
    static const uint32_t asset_policy = 4;
    static uint32_t asset_size;
    static uint32_t asset_id;
    static const uint32_t associated_data_size = sizeof(associated_data) - 1;
    static const uint32_t key_blob_size = sizeof(key_blob);

    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    if (0 != vaultip_drv_static_asset_search(get_rom_identity(),
                                             VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH, &asset_id,
                                             &asset_size)) {
        Log_Write(LOG_LEVEL_ERROR,
            "update_sp_boot_root_certificate_hash: vaultip_drv_static_asset_search(%u) failed!\n",
            VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH);
        return -1;
    }

    if (0 != vaultip_drv_otp_data_write(get_rom_identity(), VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,
                                        asset_policy, true, key_blob, key_blob_size,
                                        associated_data, associated_data_size)) {
        Log_Write(LOG_LEVEL_ERROR, " update_sp_boot_root_certificate_hash: vaultip_public_data_write() failed!\n");
        return -1;
    };

    return 0;
}

static void send_status_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                 int32_t status)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    Log_Write(LOG_LEVEL_INFO, "FW mgmt response: %s\n", __func__);

    Log_Write(LOG_LEVEL_DEBUG, "Fw mgmt response for msg_id = %u, tag_id = %u\n",msg_id, tag_id);

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "send_status_response: Cqueue push error!\n");
    }
}

static int32_t dm_svc_firmware_update(void)
{
    // Firmware image is available in the memory.
    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);
    Log_Write(LOG_LEVEL_DEBUG, "Image available at : %lx,  size: %lx\n",
            (uint64_t)DEVICE_FW_UPDATE_REGION_BASE, DEVICE_FW_UPDATE_REGION_SIZE);

    // Program the image to flash.
    if (0 != flash_fs_update_partition((void *)DEVICE_FW_UPDATE_REGION_BASE,
                                       DEVICE_FW_UPDATE_REGION_SIZE,
                                       SPI_FLASH_WRITES_256B_CHUNK_SIZE)) {
        MESSAGE_ERROR("flash_fs_update_partition: failed to write data!\n");
        return DEVICE_FW_FLASH_UPDATE_ERROR;
    } else {
        Log_Write(LOG_LEVEL_INFO, "flash partition has been updated with new image!\n");
    }

    // Swap the priority counter of the partitions. so bootrom will choose
    // the partition with an updated image
    if (0 != flash_fs_swap_primary_boot_partition()) {
        MESSAGE_ERROR("flash_fs_swap_primary_boot_partition: Update priority counter failed!\n");
        return DEVICE_FW_FLASH_PRIORITY_COUNTER_SWAP_ERROR;
    }

    return DEVICE_FW_FLASH_UPDATE_SUCCESS;
}

static void dm_svc_get_firmware_version(tag_id_t tag_id, uint64_t req_start_time)
{
    struct device_mgmt_firmware_versions_rsp_t dm_rsp = { 0 };
    SERVICE_PROCESSOR_BL2_DATA_t *sp_bl2_data;
    uint8_t major;
    uint8_t minor;
    uint8_t revision;

    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    // Get BL1 version from BL2 data.
    sp_bl2_data = get_service_processor_bl2_data();

    dm_rsp.firmware_version.bl1_v =
        FORMAT_VERSION((uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_major,
                       (uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_minor,
                       (uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_revision);

    dm_rsp.firmware_version.bl2_v =
        FORMAT_VERSION((uint32_t)get_image_version_info()->file_version_major,
                       (uint32_t)get_image_version_info()->file_version_minor,
                       (uint32_t)get_image_version_info()->file_version_revision);

    // Get the MM FW version values
    firmware_service_get_mm_version(&major, &minor, &revision);
    dm_rsp.firmware_version.mm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the WM FW version values
    firmware_service_get_wm_version(&major, &minor, &revision);
    dm_rsp.firmware_version.wm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the Machine FW version values
    firmware_service_get_machm_version(&major, &minor, &revision);
    dm_rsp.firmware_version.machm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MODULE_FIRMWARE_REVISIONS,
                    timer_get_ticks_count() - req_start_time, DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(dm_rsp))) {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_get_firmware_version: Cqueue push error!\n");
    }
}

static int32_t dm_svc_update_sp_boot_root_certificate_hash(
    struct device_mgmt_certificate_hash_cmd_t *certificate_hash_cmd)
{
    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    Log_Write(LOG_LEVEL_DEBUG, "recieved key_blob:\n");
    for (int i = 0; i < 48; i++) {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ", *(unsigned char *)&certificate_hash_cmd->certificate_hash.key_blob[i]);
    }
    Log_Write(LOG_LEVEL_DEBUG, "recieved associated_data:\n");
    for (int i = 0; i < 48; i++) {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ", *(unsigned char *)&certificate_hash_cmd->certificate_hash.associated_data[i]);
    }
    //Command payload contains the hash for new certificate.
    if (0 != update_sp_boot_root_certificate_hash(certificate_hash_cmd->certificate_hash.key_blob, certificate_hash_cmd->certificate_hash.associated_data)) {
        Log_Write(LOG_LEVEL_ERROR,
            " dm_svc_update_sp_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
        return -1;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       firmware_service_get_mm_version
*
*   DESCRIPTION
*
*       This function takes pointer to major, minor and revision variable
*       and populates them with versions of Master Minion firmware.
*
*   INPUTS
*
*       major      Pointer to store major version
*       minor      Pointer to store minor version
*       revision   Pointer to store revision version
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void firmware_service_get_mm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
{
    ESPERANTO_IMAGE_FILE_HEADER_t *master_image_file_header;

    // Get firmware version info for master minion
    master_image_file_header = get_master_minion_image_file_header();

    // Populate the required values
    *major = (((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF0000) >> 16);
    *minor = (((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF00) >> 8);
    *revision = ((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF);
}

/************************************************************************
*
*   FUNCTION
*
*       firmware_service_get_wm_version
*
*   DESCRIPTION
*
*       This function takes pointer to major, minor and revision variable
*       and populates them with versions of Worker Minion firmware.
*
*   INPUTS
*
*       major      Pointer to store major version
*       minor      Pointer to store minor version
*       revision   Pointer to store revision version
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void firmware_service_get_wm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
{
    ESPERANTO_IMAGE_FILE_HEADER_t *worker_image_file_header;

    // Get firmware version info for worker minion
    worker_image_file_header = get_worker_minion_image_file_header();

    // Populate the required values
    *major = (((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF0000) >> 16);
    *minor = (((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF00) >> 8);
    *revision = ((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF);
}

/************************************************************************
*
*   FUNCTION
*
*       firmware_service_get_machm_version
*
*   DESCRIPTION
*
*       This function takes pointer to major, minor and revision variable
*       and populates them with versions of Machine Minion firmware.
*
*   INPUTS
*
*       major      Pointer to store major version
*       minor      Pointer to store minor version
*       revision   Pointer to store revision version
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void firmware_service_get_machm_version(uint8_t *major, uint8_t *minor, uint8_t *revision)
{
    ESPERANTO_IMAGE_FILE_HEADER_t *machine_image_file_header;

    // Get firmware version for machine minion
    machine_image_file_header = get_machine_minion_image_file_header();

    // Populate the required values
    *major = (((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF0000) >> 16);
    *minor = (((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF00) >> 8);
    *revision = ((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
        0xFF);
}

/************************************************************************
*
*   FUNCTION
*
*       firmware_service_process_request
*
*   DESCRIPTION
*
*       This function takes command ID as input from Host,
*       and accordingly calls the respective firmware update/
*       certificate hash update/status functions
*
*   INPUTS
*
*       msg_id     Unique enum representing specific command
*       buffer     Pointer to command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void firmware_service_process_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    int32_t ret = 0;
    uint64_t req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_SET_FIRMWARE_UPDATE:
        ret = dm_svc_firmware_update();
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    case DM_CMD_GET_MODULE_FIRMWARE_REVISIONS:
        dm_svc_get_firmware_version(tag_id, req_start_time);
        break;
    case DM_CMD_GET_FIRMWARE_BOOT_STATUS:
        ret = dm_svc_get_firmware_status();
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    /*  TODO: These feature is to be supported in v 0.0.7
    case DM_CMD_SET_FIRMWARE_VERSION_COUNTER:
        ret = dm_svc_set_firmware_version_counter();
        break;

    case DM_CMD_SET_FIRMWARE_VALID:
        ret = dm_svc_set_firmware_valid_counter();
        break;

    case DM_CMD_SET_SW_BOOT_ROOT_CERT:
        ret = dm_svc_update_sw_boot_root_certificate_hash(dm_cmd_req);
        break;
*/
    case DM_CMD_SET_SP_BOOT_ROOT_CERT: {
        struct device_mgmt_certificate_hash_cmd_t *dm_cmd_req = (void *)buffer;
        ret = dm_svc_update_sp_boot_root_certificate_hash(dm_cmd_req);
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    }
    case DM_CMD_RESET_ETSOC:
        reset_etsoc();
        break;
    }
}
