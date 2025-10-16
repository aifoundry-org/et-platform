/*************************************************************************
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
#include "crc32.h"
#include "esperanto_flash_image.h"
#include "dm_task.h"

/* timout value for SP to complete boot */
#define SP_BOOT_TIMEOUT 60000000

/************************************************************************
*
*   FUNCTION
*
*       firmware_update_reset_etsoc
*
*   DESCRIPTION
*
*       This function resets ETSOC through PMIC and if a firmware update
*       was done in past, it'll boot to new firmware.
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
static void firmware_update_reset_etsoc(void)
{
    int32_t status;

    /* Reset PMIC as well if PMIC firmware was updated */
    if (pmic_check_firmware_updated())
    {
        Log_Write(LOG_LEVEL_INFO, "Resetting ETSOC and PMIC..!\n");

        /* Reset the SP + PMIC */
        pmic_force_power_off_on();
    }
    else
    {
        /* Enable and program external PMIC watchdog timeout to auto reset SP if it
        fails to reset watchdog in time. This watchdog reset will be disabled upon
        successful SP boot. PERST is also toggled by pmic in this force reset process */
        status = pmic_enable_wdog_timeout_reset();
        if (status == STATUS_SUCCESS)
        {
            status = pmic_set_wdog_timeout_time(SP_BOOT_TIMEOUT);
            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_INFO, "Resetting ETSOC..!\n");

                /* Reset the SP */
                pmic_force_reset();
            }
            else
            {
                pmic_disable_wdog_timeout_reset();

                Log_Write(LOG_LEVEL_ERROR, "Failed to set PMIC watchdog timeout. status: %d\r\n",
                          status);
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to enable PMIC watchdog. status: %d\r\n", status);
        }
    }
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
    uint8_t gs_sp_root_ca_hash[512 / 8];
    uint32_t hash_size;
    struct device_mgmt_fused_pub_keys_rsp_t dm_rsp = { 0 };

    if (0 != crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,
                                                  gs_sp_root_ca_hash, sizeof(gs_sp_root_ca_hash),
                                                  &hash_size))
    {
        Log_Write(LOG_LEVEL_WARNING, "SP ROOT CA HASH not found in OTP!\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_FUSED_PUBLIC_KEYS,
                    timer_get_ticks_count() - req_start_time, DM_STATUS_SUCCESS)

    // copy the root hash
    memcpy(dm_rsp.fused_public_keys.keys, gs_sp_root_ca_hash, hash_size);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_fused_pub_keys_rsp_t)))
    {
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

    if (0 != flash_fs_get_boot_counters(&attempted_boot_counter, &completed_boot_counter))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_get_boot_counters: failed to get boot counters!\n");
        return ERROR_FW_UPDATE_IMAGE_BOOT;
    }
    else
    {
        Log_Write(LOG_LEVEL_INFO, "flash_fs_get_boot_counters: Success!\n");
        if (attempted_boot_counter != completed_boot_counter)
        {
            Log_Write(
                LOG_LEVEL_ERROR,
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
static int32_t update_sw_boot_root_certificate_hash(char *key_blob, char *associated_data)
{
    int32_t ret = 0;

    Log_Write(LOG_LEVEL_DEBUG, "recieved key_blob:\n");
    for (int i = 0; i < 48; i++)
    {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ", *(unsigned char *)&key_blob[i]);
    }
    Log_Write(LOG_LEVEL_DEBUG, "recieved associated_data:\n");
    for (int i = 0; i < 48; i++)
    {
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
static int32_t dm_svc_update_sw_boot_root_certificate_hash(
    struct device_mgmt_certificate_hash_cmd_t *certificate_hash_cmd)
{
    //cmd_payload contains the hash for new certificate.
    if (0 != update_sw_boot_root_certificate_hash(
                 certificate_hash_cmd->certificate_hash.key_blob,
                 certificate_hash_cmd->certificate_hash.associated_data))
    {
        Log_Write(
            LOG_LEVEL_ERROR,
            " dm_svc_update_sw_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
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
static int32_t update_sp_boot_root_certificate_hash(char *key_blob, char *associated_data)
{
    static const uint32_t asset_policy = 4;
    static uint32_t asset_size;
    static uint32_t asset_id;
    static const uint32_t associated_data_size = sizeof(associated_data) - 1;
    static const uint32_t key_blob_size = sizeof(key_blob);

    Log_Write(LOG_LEVEL_INFO, "FW mgmt request: %s\n", __func__);

    if (0 != vaultip_drv_static_asset_search(
                 get_rom_identity(), VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH, &asset_id, &asset_size))
    {
        Log_Write(
            LOG_LEVEL_ERROR,
            "update_sp_boot_root_certificate_hash: vaultip_drv_static_asset_search(%u) failed!\n",
            VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH);
        return -1;
    }

    if (0 != vaultip_drv_otp_data_write(get_rom_identity(), VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,
                                        asset_policy, true, key_blob, key_blob_size,
                                        associated_data, associated_data_size))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  " update_sp_boot_root_certificate_hash: vaultip_public_data_write() failed!\n");
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
    for (int i = 0; i < 48; i++)
    {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ",
                  *(unsigned char *)&certificate_hash_cmd->certificate_hash.key_blob[i]);
    }
    Log_Write(LOG_LEVEL_DEBUG, "recieved associated_data:\n");
    for (int i = 0; i < 48; i++)
    {
        Log_Write(LOG_LEVEL_DEBUG, "%02x ",
                  *(unsigned char *)&certificate_hash_cmd->certificate_hash.associated_data[i]);
    }
    //Command payload contains the hash for new certificate.
    if (0 != update_sp_boot_root_certificate_hash(
                 certificate_hash_cmd->certificate_hash.key_blob,
                 certificate_hash_cmd->certificate_hash.associated_data))
    {
        Log_Write(
            LOG_LEVEL_ERROR,
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

    Log_Write(LOG_LEVEL_DEBUG, "Fw mgmt response for msg_id = %u, tag_id = %u\n", msg_id, tag_id);

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "send_status_response: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       verify_image_regions
*
*   DESCRIPTION
*
*       This function performs verification checks on regions of input
*       image before updating the image on flash.
*
*   INPUTS
*
*       fw_addr      Start of image which is start of scratch region
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t verify_image_regions(void *fw_addr)
{
    uint32_t crc;
    uint32_t region_offset_end;
    uint32_t partition_size_in_blocks;
    const ESPERANTO_FLASH_PARTITION_HEADER_t *header;
    const ESPERANATO_REGION_INFO_t *regions_table;

    header = (ESPERANTO_FLASH_PARTITION_HEADER_t *)fw_addr;
    if (header == NULL)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: image address is invalid!\n");
        return ERROR_FW_UPDATE_VERIFY_REGIONS_INVALID_ARGUMENTS;
    }
    partition_size_in_blocks = header->partition_size; // already in terms of blocks

    regions_table = (ESPERANATO_REGION_INFO_t *)((uint64_t)fw_addr +
                                                 sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t));

    for (uint32_t n = 0; n < header->regions_count; n++, regions_table++)
    {
        crc = 0;
        crc32(regions_table, offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum), &crc);
        if (crc != regions_table->region_info_checksum)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "verify_image_regions: region %u CRC mismatch! (expected %08x, got %08x)\n",
                      n, regions_table->region_info_checksum, crc);
            return ERROR_FW_UPDATE_IMG_REGION_CRC_MISMATCH;
        }
        if (0 == regions_table->region_offset ||
            regions_table->region_offset >= partition_size_in_blocks)
        {
            Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: invalid region %u offset!\n", n);
            return ERROR_FW_UPDATE_IMG_REGION_OFFSET_INVALID;
        }
        if (0 == regions_table->region_reserved_size)
        {
            Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: region %u has zero size!\n", n);
            return ERROR_FW_UPDATE_IMG_REGION_SIZE_INVALID;
        }
        region_offset_end = regions_table->region_offset + regions_table->region_reserved_size;
        if (region_offset_end < regions_table->region_offset)
        {
            Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: region %u offset/size overflow!\n",
                      n);
            return ERROR_FW_UPDATE_IMG_REGION_OFFSET_OR_SIZE_OVERFLOW;
        }
        if (region_offset_end > partition_size_in_blocks)
        {
            Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: invalid region %u size!\n", n);
            return ERROR_FW_UPDATE_IMG_REGION_SIZE_INVALID;
        }

        switch (regions_table->region_id)
        {
            case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
                if (1 != regions_table->region_reserved_size)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "verify_image_regions: invalid region %u PRI DESIGN wrong size!\n",
                              n);
                    return ERROR_FW_UPDATE_IMG_REGION_PRI_DES_WRONG_SIZE;
                }
                break;
            case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
                if (1 != regions_table->region_reserved_size)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "verify_image_regions: invalid region %u BOOT COUNT wrong size!\n",
                              n);
                    return ERROR_FW_UPDATE_IMG_REGION_BOOT_COUNT_WRONG_SIZE;
                }
                break;
            case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
                if (1 != regions_table->region_reserved_size)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "verify_image_regions: invalid region %u CFG DATA wrong size!\n", n);
                    return ERROR_FW_UPDATE_IMG_REGION_CFG_DATA_WRONG_SIZE;
                }
                break;
            case ESPERANTO_FLASH_REGION_ID_PCIE_CONFIG:
            case ESPERANTO_FLASH_REGION_ID_VAULTIP_FW:
            case ESPERANTO_FLASH_REGION_ID_SP_CERTIFICATES:
            case ESPERANTO_FLASH_REGION_ID_SP_BL1:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            case ESPERANTO_FLASH_REGION_ID_MACHINE_MINION:
            case ESPERANTO_FLASH_REGION_ID_MASTER_MINION:
            case ESPERANTO_FLASH_REGION_ID_WORKER_MINION:
            case ESPERANTO_FLASH_REGION_ID_MAXION_BL1:
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S0:
            case ESPERANTO_FLASH_REGION_ID_PMIC_FW_S1:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ:
            case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ:
            case ESPERANTO_FLASH_REGION_ID_SP_BL2:
                break;
            case ESPERANTO_FLASH_REGION_ID_INVALID:
            case ESPERANTO_FLASH_REGION_ID_SW_CERTIFICATES:
            case ESPERANTO_FLASH_REGION_ID_COMM_CERTIFICATES:
            case ESPERANTO_FLASH_REGION_ID_AFTER_LAST_SUPPORTED_VALUE:
            case ESPERANTO_FLASH_REGION_ID_FFFFFFFF:
                Log_Write(LOG_LEVEL_DEBUG, "verify_image_regions: ignoring region id %u.\n", n);
                continue;
            default:
                Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: invalid region id %u.\n", n);
                return ERROR_FW_UPDATE_IMG_REGION_ID_INVALID;
        }
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       verify_image_header
*
*   DESCRIPTION
*
*       This function performs verification checks on input update image
*       before updating the image on flash.
*
*   INPUTS
*
*       fw_addr      Start of image which is start of scratch region
*
*   OUTPUTS
*
*       Status
*
***********************************************************************/
static int32_t verify_image_header(void *fw_addr)
{
    const ESPERANTO_FLASH_PARTITION_HEADER_t *header;
    uint32_t crc = 0;

    header = (ESPERANTO_FLASH_PARTITION_HEADER_t *)fw_addr;
    if (header == NULL)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: image address is invalid!\n");
        return ERROR_FW_UPDATE_VERIFY_HEADER_INVALID_ARGUMENTS;
    }

    crc32(header, offsetof(ESPERANTO_FLASH_PARTITION_HEADER_t, partition_header_checksum), &crc);
    if (crc != header->partition_header_checksum)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: partition header CRC mismatch!\n");
        return ERROR_FW_UPDATE_IMG_HEADER_CRC_MISMATCH;
    }

    if (ESPERANTO_PARTITION_TAG != header->partition_tag)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "verify_image_header: partition header tag mismatch! (expected %08x, got %08x)\n",
                  ESPERANTO_PARTITION_TAG, header->partition_tag);
        return ERROR_FW_UPDATE_IMG_HEADER_TAG_MISMATCH;
    }
    if (sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t) != header->partition_header_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: partition header size mismatch!\n");
        return ERROR_FW_UPDATE_IMG_HEADER_SIZE_MISMATCH;
    }
    if (sizeof(ESPERANATO_REGION_INFO_t) != header->region_info_size)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: partition region size mismatch!\n");
        return ERROR_FW_UPDATE_IMG_HEADER_REGION_SIZE_MISMATCH;
    }
    if (header->regions_count > ESPERANTO_MAX_REGIONS_COUNT)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: invalid regions count value!\n");
        return ERROR_FW_UPDATE_IMG_HEADER_REGIONS_COUNT_INVALID;
    }

    return 0;
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
    uint64_t start;
    uint64_t end;
    uint64_t prog_start;
    uint64_t prog_end;
    uint64_t verify_start;
    uint64_t verify_end;
    int32_t ret = 0;
    uint32_t version;
    int status = STATUS_SUCCESS;

    start = timer_get_ticks_count();

    version = sp_get_image_version_info();
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Esperanto Flash Programmer Version %u.%u.%u\n",
              (version >> 24) & 0xFF, (version >> 16) & 0xFF, (version >> 8) & 0xFF);

    sp_bl2_data = get_service_processor_bl2_data();
    if (sp_bl2_data == NULL)
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_firmware_update: Unable to get SP BL2 data!\n");
        return ERROR_FW_UPDATE_INVALID_BL2_DATA;
    }

    partition_size = sp_bl2_data->flash_fs_bl2_info.flash_size / 2;

    ret = verify_image_header((void *)SP_DM_SCRATCH_REGION_BEGIN);
    if (ret != 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_header: update image verification failed!\n");
        return ret;
    }

    ret = verify_image_regions((void *)SP_DM_SCRATCH_REGION_BEGIN);
    if (ret != 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_image_regions: update image verification failed!\n");
        return ret;
    }

    prog_start = timer_get_ticks_count();

    // Image has passed verifcation checks, program it to flash.
    if (0 != flash_fs_update_partition((void *)SP_DM_SCRATCH_REGION_BEGIN, partition_size,
                                       SPI_FLASH_PAGE_SIZE))
    {
        Log_Write(LOG_LEVEL_ERROR, "flash_fs_update_partition: failed to write data!\n");
        return ERROR_FW_UPDATE_ERASE_WRITE_PARTITION;
    }
    else
    {
        prog_end = timer_get_ticks_count();
        Log_Write(LOG_LEVEL_INFO, "[ETFP] Current ETSoC FW image at partition %d\n",
                  sp_bl2_data->flash_fs_bl2_info.active_partition);
        Log_Write(LOG_LEVEL_INFO, "[ETFP] New ETSoC FW image at partition %d\n",
                  1 - sp_bl2_data->flash_fs_bl2_info.active_partition);
    }

    /* Read back the image data written into flash and compare with the
        the data present in the DDR - ensure data is written correctly to
        flash.
    */
    verify_start = timer_get_ticks_count();
    const uint8_t *ddr_data = (const uint8_t *)SP_DM_SCRATCH_REGION_BEGIN;
    uint8_t flash_data[SPI_FLASH_PAGE_SIZE];

    for (uint32_t i = 0; i < partition_size; i = i + SPI_FLASH_PAGE_SIZE)
    {
        /* Read data from flash passive partition */
        if (0 != flash_fs_read(false, flash_data, SPI_FLASH_PAGE_SIZE, i))
        {
            Log_Write(LOG_LEVEL_ERROR, "flash_fs_read: read back from flash failed!\n");
            return ERROR_FW_UPDATE_READ_PARTITON;
        }
        /* Compare with the image data in the DDR */
        if (memcmp((const void *)(ddr_data + i), (const void *)flash_data, SPI_FLASH_PAGE_SIZE))
        {
            Log_Write(LOG_LEVEL_ERROR, "flash_fs_read: data validation failed!\n");
            return ERROR_FW_UPDATE_MEMCOMPARE;
        }
    }

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Programming SP config region\n");
    ret = flash_fs_write_config_region(1 - sp_bl2_data->flash_fs_bl2_info.active_partition, false);
    if (ret != 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "flash_fs_write_config_region: failed to write config data, ret: %d\n", ret);
        return ERROR_FW_UPDATE_WRITE_CFG_REGION;
    }

    /* Re-scan the passive partition to load the updated data in BL2 runtime (if any) */
    status = flash_fs_scan_partition(1 - sp_bl2_data->flash_fs_bl2_info.active_partition);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Passive partition re-scan complete.\n");

    verify_end = timer_get_ticks_count();
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] All loaded bytes verified OK!\n");
    end = timer_get_ticks_count();
    Log_Write(LOG_LEVEL_CRITICAL,
              "[ETFP] SP Target erased, programmed and verified successfully\n");
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Completed after %ld seconds\n",
              timer_convert_ticks_to_secs(end - start));
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] OK (Total %lds, Erase and Prog %lds, Verify %lds)\n",
              timer_convert_ticks_to_secs(end - start),
              timer_convert_ticks_to_secs(prog_end - prog_start),
              timer_convert_ticks_to_secs(verify_end - verify_start));

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Initiating PMIC FW update...\n");
    /* Suspend the Periodic sampling during pmic fw update process */
    dm_sampling_task_semaphore_take();
    /* Update the PMIC firmware image */
    status = pmic_firmware_update();
    /* Resume the periodic sampling */
    dm_sampling_task_semaphore_give();

    /* Only switch to new partition if PMIC FW update was successful */
    if (status == STATUS_SUCCESS)
    {
        /* Swap the priority counter of the partitions. so bootrom will choose
        the partition with an updated image */
        if (0 != flash_fs_swap_primary_boot_partition())
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "flash_fs_swap_primary_boot_partition: Update priority counter failed!\n");
            return ERROR_FW_UPDATE_PRIORITY_COUNTER_SWAP;
        }
        end = timer_get_ticks_count();
        Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Executing exit sequence...\n");
        Log_Write(LOG_LEVEL_CRITICAL,
                  "[ETFP] Targets erased, programmed and verified successfully\n");
        Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Device firmware update completed after %ld seconds\n",
                  timer_convert_ticks_to_secs(end - start));
    }
    else
    {
        Log_Write(LOG_LEVEL_WARNING, "[ETFP] Executing exit sequence, return status = %d...\n",
                  status);
        Log_Write(LOG_LEVEL_ERROR, "[ETFP] Firmware update failed!\n");
    }

    return status;
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
    dm_rsp.firmware_version.mm_v =
        FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the WM FW version values
    firmware_service_get_wm_version(&major, &minor, &revision);
    dm_rsp.firmware_version.wm_v =
        FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the Machine FW version values
    firmware_service_get_machm_version(&major, &minor, &revision);
    dm_rsp.firmware_version.machm_v =
        FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the FW release revision values
    flash_fs_get_fw_release_rev((char *)&dm_rsp.firmware_version.fw_release_rev);

    if (0 != pmic_get_fw_version(&major, &minor, &revision))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_get_firmware_version: PMIC get FW Version error!\n");
    }
    dm_rsp.firmware_version.pmic_v =
        FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MODULE_FIRMWARE_REVISIONS,
                    timer_get_ticks_count() - req_start_time, DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(dm_rsp)))
    {
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
    *major =
        (((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF0000) >>
         16);
    *minor =
        (((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF00) >>
         8);
    *revision =
        ((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
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
    *major =
        (((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF0000) >>
         16);
    *minor =
        (((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF00) >>
         8);
    *revision =
        ((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
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
    *major =
        (((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF0000) >>
         16);
    *minor =
        (((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
          0xFF00) >>
         8);
    *revision =
        ((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
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

    switch (msg_id)
    {
        case DM_CMD_SET_FIRMWARE_UPDATE:
            ret = MM_Iface_Send_Abort_All_Cmd();
            if (ret != SUCCESS)
            {
                Log_Write(
                    LOG_LEVEL_ERROR,
                    "firmware_service_process_request: Unable to abort all MM commands. Status: %d\n",
                    ret);
            }
            /* Do firmware update regardless of MM commands not able to abort */
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

        case DM_CMD_SET_FIRMWARE_VERSION_COUNTER: {
            uint32_t counter = *((uint32_t *)buffer);
            ret = dm_svc_set_bl2_monotonic_counter(counter);
            send_status_response(tag_id, msg_id, req_start_time, ret);
            break;
        }

        case DM_CMD_SET_SW_BOOT_ROOT_CERT: {
            struct device_mgmt_certificate_hash_cmd_t *dm_cmd_req = (void *)buffer;
            ret = dm_svc_update_sw_boot_root_certificate_hash(dm_cmd_req);
            send_status_response(tag_id, msg_id, req_start_time, ret);
            break;
        }

        case DM_CMD_SET_SP_BOOT_ROOT_CERT: {
            struct device_mgmt_certificate_hash_cmd_t *dm_cmd_req = (void *)buffer;
            ret = dm_svc_update_sp_boot_root_certificate_hash(dm_cmd_req);
            send_status_response(tag_id, msg_id, req_start_time, ret);
            break;
        }

        case DM_CMD_GET_FUSED_PUBLIC_KEYS:
            dm_svc_get_public_keys(tag_id, req_start_time);
            break;

        case DM_CMD_RESET_ETSOC:
            firmware_update_reset_etsoc();
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "firmware_service_process_request: invalid message id %u.\n",
                      msg_id);
    }
}
