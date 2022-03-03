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
#include "bl2_build_configuration.h"
#include "bl2_firmware_update.h"
#include "mm_iface.h"
#include "system/layout.h"
#include "bl_error_code.h"

#define SPI_FLASH_256B_CHUNK_SIZE 256

/************************************************************************
*
*   FUNCTION
*
*       reset_etsoc
*
*   DESCRIPTION
*
*       This function resets ETSOC
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void reset_etsoc(void)
{
    Log_Write(LOG_LEVEL_INFO, "Resetting ETSOC..!\n");

    // Now Reset SP.
    release_etsoc_reset();
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_get_public_keys
*
*   DESCRIPTION
*
*       This function reads public keys from Vault OTP
*
*   INPUTS
*
*       tag_id              Message tag ID
*       req_start_time      Message start time
*
*   OUTPUTS
*
*       void
*
***********************************************************************/
static void dm_svc_get_public_keys(tag_id_t tag_id, uint64_t req_start_time)
{
    uint8_t                                   gs_sp_root_ca_hash[512/8];
    uint32_t                                  hash_size;
    struct device_mgmt_fused_pub_keys_rsp_t   dm_rsp = { 0 };

    if (0 != crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH, gs_sp_root_ca_hash, sizeof(gs_sp_root_ca_hash), &hash_size)) {
        Log_Write(LOG_LEVEL_WARNING,"SP ROOT CA HASH not found in OTP!\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_FUSED_PUBLIC_KEYS, timer_get_ticks_count() - req_start_time, DM_STATUS_SUCCESS)

    // copy the root hash
    memcpy(dm_rsp.fused_public_keys.keys, gs_sp_root_ca_hash, hash_size);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_fused_pub_keys_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "send_status_response: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_get_firmware_status
*
*   DESCRIPTION
*
*       This function checks firmware boot status
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Firmware boot status
*
***********************************************************************/
static int32_t dm_svc_get_firmware_status(void)
{
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    if (0 != flash_fs_get_boot_counters(&attempted_boot_counter, &completed_boot_counter)) {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_get_boot_counters: failed to get boot counters!\n");
        return ERROR_FW_UPDATE_IMAGE_BOOT;
    } else {
        Log_Write(LOG_LEVEL_INFO, "flash_fs_get_boot_counters: Success!\n");
        if (attempted_boot_counter != completed_boot_counter) {
            Log_Write(LOG_LEVEL_ERROR,
                "flash_fs_get_boot_counters: Attempted and completed boot counter do not match!\n");
            return ERROR_FW_UPDATE_IMAGE_BOOT;
        }
    }

    // Return the firmware boot status as success
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_set_bl2_monotonic_counter
*
*   DESCRIPTION
*
*       This function sets BL2 monotonic counter value in the OTP
*
*   INPUTS
*
*       counter     Counter value
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t dm_svc_set_bl2_monotonic_counter(uint32_t counter)
{
    int32_t ret = 0;

    Log_Write(LOG_LEVEL_INFO, "BL2 monotonic counter is %d\n", counter);
    //TODO: Placeholder, needs implementation

    return ret;
}

/************************************************************************
*
*   FUNCTION
*
*       update_sw_boot_root_certificate_hash
*
*   DESCRIPTION
*
*       This is a helper function for writing SW boot root ceritifcate hash
*
*   INPUTS
*
*       Key blob            Pointer to key blob
*       Associated data     Pointer to associated data
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t update_sw_boot_root_certificate_hash(char *key_blob, char* associated_data)
{
    int32_t ret = 0;

    Log_Write(LOG_LEVEL_DEBUG, "recieved key_blob:\n");
    for (int i = 0; i < 48; i++) {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ", *(unsigned char *)&key_blob[i]);
    }
    Log_Write(LOG_LEVEL_DEBUG, "recieved associated_data:\n");
    for (int i = 0; i < 48; i++) {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ", *(unsigned char *)&associated_data[i]);
    }
    //TODO: Needs a future implementation. Currently we do not have a OTP field for SW boot certificate hash

    return ret;
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_update_sw_boot_root_certificate_hash
*
*   DESCRIPTION
*
*       This is a function for writing SW boot root ceritifcate hash to Vault OTP
*
*   INPUTS
*
*       certificate_hash_cmd    Pointer to Certificate hash structure containing key blob and associated data
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t dm_svc_update_sw_boot_root_certificate_hash(struct device_mgmt_certificate_hash_cmd_t *certificate_hash_cmd)
{
   //cmd_payload contains the hash for new certificate.
   if(0 != update_sw_boot_root_certificate_hash(certificate_hash_cmd->certificate_hash.key_blob, certificate_hash_cmd->certificate_hash.associated_data)) {
        Log_Write(LOG_LEVEL_ERROR, " dm_svc_update_sw_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
        return -1;
   }

   return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_sp_boot_root_certificate_hash
*
*   DESCRIPTION
*
*       This is a helper function for writing SP boot root ceritifcate hash
*
*   INPUTS
*
*       Key blob            Pointer to key blob
*       Associated data     Pointer to associated data
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
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

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_update_sp_boot_root_certificate_hash
*
*   DESCRIPTION
*
*       This is a function for writing SP boot root ceritifcate hash to Vault OTP
*
*   INPUTS
*
*       certificate_hash_cmd    Pointer to Certificate hash structure containing key blob and associated data
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
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
*       send_status_response
*
*   DESCRIPTION
*
*       This is a helper function for sending device response to host interface
*
*   INPUTS
*
*       tag_id              Unique ID for message/response correlation
*       msg_id              Command ID
*       req_start_time      Timer value at the moment response was sent
*       status              Status
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
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

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_firmware_update
*
*   DESCRIPTION
*
*       This is a function for updating firmware on passive partition of
*       flash memory. The active partition from which the system booted
*       is not modified. Host writes input image (4 MBs in size) to
*       scratch region of device memory. Input image is composed of two
*       2 MB partition images. Since partition size is 2 MBs, only
*       1st half of input image is always used in firmware update.
*       First, passive partition is erased and then image is flashed.
*       After this, verification is performed which involves comparing
*       input image residing in scratch region with that on passive
*       partition. After verification completes, priority counters for
*       both partitions are set such that passive partition gets
*       precedence after reboot and becomes the active partition and
*       brings up silicon using updated firmware.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t dm_svc_firmware_update(void)
{
    // Firmware image is available in the memory.
    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);
    Log_Write(LOG_LEVEL_DEBUG, "Image available at : %lx,  size: %x\n",
            (uint64_t)SP_DM_SCRATCH_REGION_BEGIN, SP_DM_SCRATCH_REGION_SIZE);

    const SERVICE_PROCESSOR_BL2_DATA_t *sp_bl2_data;
    uint32_t partition_size;
    uint64_t start_time;
    uint64_t end_time;

    sp_bl2_data = get_service_processor_bl2_data();
    if (sp_bl2_data == NULL) {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_firmware_update: Unable to get SP BL2 data!\n");
        return ERROR_FW_UPDATE_INVALID_BL2_DATA;
    }

    partition_size = sp_bl2_data->flash_fs_bl2_info.flash_size / 2;
    start_time = timer_get_ticks_count();

    // Program the image to flash.
    if (0 != flash_fs_update_partition((void *)SP_DM_SCRATCH_REGION_BEGIN, partition_size,
                                       SPI_FLASH_256B_CHUNK_SIZE)) {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_update_partition: failed to write data!\n");
        return ERROR_FW_UPDATE_ERASE_WRITE_PARTITION;
    } else {
        Log_Write(LOG_LEVEL_INFO, "Current FW image at partition %d\n", sp_bl2_data->flash_fs_bl2_info.active_partition);
        Log_Write(LOG_LEVEL_INFO, "New FW image at partition %d\n", 1 - sp_bl2_data->flash_fs_bl2_info.active_partition);
    }

    /* Read back the image data written into flash and compare with the
        the data present in the DDR - ensure data is written correctly to
        flash.
    */
    const uint8_t *ddr_data = (const uint8_t *)SP_DM_SCRATCH_REGION_BEGIN;
    uint8_t flash_data[SPI_FLASH_256B_CHUNK_SIZE];

    for (uint32_t i = 0; i < partition_size; i = i + SPI_FLASH_256B_CHUNK_SIZE) {
        /* Read data from flash passive partition */
        if (0 != flash_fs_read(false, flash_data, SPI_FLASH_256B_CHUNK_SIZE, i)) {
            Log_Write(LOG_LEVEL_ERROR, "flash_fs_read_partition: read back from flash failed!\n");
            return ERROR_FW_UPDATE_READ_PARTITON;
        }

        /* Compare with the image data in the DDR */
        if (memcmp((const void *)(ddr_data + i), (const void *)flash_data,
                                SPI_FLASH_256B_CHUNK_SIZE)) {
            Log_Write(LOG_LEVEL_ERROR, "flash_fs_read_partition: data validation failed!\n");
            return ERROR_FW_UPDATE_MEMCOMPARE;
        }
    }

    end_time = timer_get_ticks_count();

    // Swap the priority counter of the partitions. so bootrom will choose
    // the partition with an updated image
    if (0 != flash_fs_swap_primary_boot_partition()) {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_swap_primary_boot_partition: Update priority counter failed!\n");
        return ERROR_FW_UPDATE_PRIORITY_COUNTER_SWAP;
    }

    Log_Write(LOG_LEVEL_CRITICAL, "New FW updated successfully in %ld seconds\n", (end_time - start_time) / 1000000);

    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       sp_get_image_version_info
*
*   DESCRIPTION
*
*       This is a function returns image version information
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       file verison information
*
***********************************************************************/
uint32_t sp_get_image_version_info(void)
{
   return (FORMAT_VERSION((uint32_t)get_image_version_info()->file_version_major,
                       (uint32_t)get_image_version_info()->file_version_minor,
                       (uint32_t)get_image_version_info()->file_version_revision));
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_get_firmware_version
*
*   DESCRIPTION
*
*       This function returns values for BL1, BL2, MM, WM and MachM firmware versions
*
*   INPUTS
*
*       tag_id              Message tag ID
*       req_start_time      Message start time
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
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
        ret = MM_Iface_Send_Abort_All_Cmd();
        if( ret == SUCCESS)
        {
            ret = dm_svc_firmware_update();
        }
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;

    case DM_CMD_GET_MODULE_FIRMWARE_REVISIONS:
        dm_svc_get_firmware_version(tag_id, req_start_time);
        break;

    case DM_CMD_GET_FIRMWARE_BOOT_STATUS:
        ret = dm_svc_get_firmware_status();
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;

    case DM_CMD_SET_FIRMWARE_VERSION_COUNTER: {
        uint32_t counter = *((uint32_t*)buffer);
        ret = dm_svc_set_bl2_monotonic_counter(counter);
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    }

    case DM_CMD_SET_SW_BOOT_ROOT_CERT:{
        struct device_mgmt_certificate_hash_cmd_t *dm_cmd_req = (void*)buffer;
        ret = dm_svc_update_sw_boot_root_certificate_hash(dm_cmd_req);
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    }

    case DM_CMD_SET_SP_BOOT_ROOT_CERT: {
        struct device_mgmt_certificate_hash_cmd_t *dm_cmd_req = (void*)buffer;
        ret = dm_svc_update_sp_boot_root_certificate_hash(dm_cmd_req);
        send_status_response(tag_id, msg_id, req_start_time, ret);
        break;
    }

    case DM_CMD_GET_FUSED_PUBLIC_KEYS:
        dm_svc_get_public_keys(tag_id, req_start_time);
        break;

    case DM_CMD_RESET_ETSOC:
        reset_etsoc();
        break;
    }
}
