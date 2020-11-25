#include "bl2_firmware_update.h"

struct dm_control_block dm_cmd_rsp;

static int64_t firmware_service_send_response(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time,
                                              char response[], uint32_t length)
{
    printf("firmware_service_send_response -start\n");
    int64_t ret = 0;
    uint64_t rsp_complete_time;
    rsp_complete_time = timer_get_ticks_count();
    memset(&dm_cmd_rsp, 0, sizeof(struct dm_control_block));
    dm_cmd_rsp.cmd_id = cmd_id;
    dm_cmd_rsp.dev_latency = (rsp_complete_time - req_start_time);
    strncpy(dm_cmd_rsp.cmd_payload, response, length);
    printf("cmd_id: %d  dev_latency:%ld  rsp_payload: %s \r\n", dm_cmd_rsp.cmd_id,
           dm_cmd_rsp.dev_latency, dm_cmd_rsp.cmd_payload);
    ret = MBOX_send(mbox, &dm_cmd_rsp, sizeof(struct dm_control_block));
    printf("firmware_service_send_response - end\n");
    return ret;
}

static void send_status_response(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time, int64_t ret)
{
    char ret_status[8];
    printf("dm firmware svc. cmd_id :%d  status:%ld\n", cmd_id, ret);
    sprintf(ret_status, "%ld", ret);
    ret = firmware_service_send_response(mbox, cmd_id, req_start_time, ret_status, 8);
    if (ret != 0) {
        printf("MBOX Send error: %ld, cmd_id: %d\n", ret, cmd_id);
    }
}

static int64_t dm_svc_firmware_update(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time)
{
    char fw_update_status[8];

    // Firmware image is available in the memory.
    printf("fw image available at : %lx,  size: %lx\n", (uint64_t)DEVICE_FW_UPDATE_REGION_BASE,
           DEVICE_FW_UPDATE_REGION_SIZE);

    // Program the image to flash.
    if (0 != flash_update_partition((void *)DEVICE_FW_UPDATE_REGION_BASE,
                                    DEVICE_FW_UPDATE_REGION_SIZE)) {
        printf("flash_update_partition: failed to write data!\n");
        return DEVICE_FW_FLASH_UPDATE_ERROR;
    } else {
        printf("flash partition has been updated with new image!\n");
    }

    // Swap the priority counter of the partitions. so bootrom will choose the partition with an updated image
    if (0 != flash_fs_swap_priority_counter()) {
        printf("flash_partition_update_priority_counter: Updating priority counter failed!\n");
        return DEVICE_FW_FLASH_PRIORITY_COUNTER_SWAP_ERROR;
    }

    sprintf(fw_update_status, "%d", DEVICE_FW_FLASH_UPDATE_SUCCESS);

    // Send firmware update process status
    if (0 != firmware_service_send_response(mbox, cmd_id, req_start_time, fw_update_status, 8)) {
        printf("mbox send error while sending fw valid status!\n");
    }

    printf("Resetting ETSOC..!\n");

    // Now Reset SP.
    pmic_toggle_etsoc_reset();

    //Note: Code will never return from here since SP is reset above.
    return 0;
}

static int64_t dm_svc_get_firmware_status(void)
{
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    if (0 != flash_fs_get_boot_counters(&attempted_boot_counter, &completed_boot_counter)) {
        printf("flash_partition_get_boot_counters: failed to get boot counters !\n");
    } else {
        printf("flash_partition_get_boot_counters: Success !\n");
        if (attempted_boot_counter != completed_boot_counter) {
            printf(
                "flash_partition_get_boot_counters: Attempted and completed boot counter do not match!\n");
            return DEVICE_FW_UPDATED_IMAGE_BOOT_FAILED;
        }
    }

    // Return the firmware boot status as success
    return 0;
}

static void dm_svc_get_firmware_version(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time)
{
    char fw_vers[20];
    ESPERANTO_IMAGE_FILE_HEADER_t *master_image_file_header;
    ESPERANTO_IMAGE_FILE_HEADER_t *machine_image_file_header;
    ESPERANTO_IMAGE_FILE_HEADER_t *worker_image_file_header;
    SERVICE_PROCESSOR_BL2_DATA_t *sp_bl2_data;
    const IMAGE_VERSION_INFO_t *bl2_image_version;

    //Get BL1 version from BL2 data.
    sp_bl2_data = get_service_processor_bl2_data();

    //Get BL2 version
    bl2_image_version = get_image_version_info();
    
    // Get firmware version info for master minion
    master_image_file_header = get_master_minion_image_file_header();

    // Get firmware version info for worker minion
    worker_image_file_header = get_worker_minion_image_file_header();

    // Get firmware version for machine minion
    machine_image_file_header = get_machine_minion_image_file_header();

    sprintf(
        fw_vers, "%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u", bl2_image_version->file_version_major,
        bl2_image_version->file_version_minor, bl2_image_version->file_version_revision, '\0',
        sp_bl2_data->service_processor_bl1_image_file_version_major,
        sp_bl2_data->service_processor_bl1_image_file_version_minor,
        sp_bl2_data->service_processor_bl1_image_file_version_revision, '\0',
        ((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF0000) >>
            16,
        ((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF00) >>
            8,
        ((master_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF),
        '\0',
        ((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF0000) >>
            16,
        ((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF00) >>
            8,
        ((machine_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF),
        '\0',
        ((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF0000) >>
            16,
        ((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF00) >>
            8,
        ((worker_image_file_header->info.image_info_and_signaure.info.public_info.file_version) &
         0xFF),
        '\0');

    // Send the response containing firmware versions.
    if (0 != firmware_service_send_response(mbox, cmd_id, req_start_time, fw_vers, 20)) {
        printf("mbox send error while sending fw revisions!\n");
    }
}

/*  TODO: This feature is to be supported in v 0.0.7 
static int64_t dm_svc_set_firmware_version_counter(void)
{
    int64_t ret = 0;

    // Release 0.0.7
    // SW-4553 implements this and is blocked by
    // SW-4570  FW Update Dev: BL2 driver to update the vault IP & SP fuses for monotonic counters

    return ret;
}
*/

/* TODO: This feature is to be supported in v 0.0.7 
static int64_t dm_svc_set_firmware_valid_counter(void)
{
    int64_t ret = 0;
    // Release 0.0.7
    // SW-4546 FW Update Dev: Implement device side runtime firmware validation

    return ret;
}
*/

/* TODO: This feature is to be supported in v 0.0.7
static int64_t update_sw_boot_root_certificate_hash(char *hash)
{
    (void)hash;

    //TODO : SW-5133 SP BL2 Vault IP driver needs to provide support for provisioning SW BOOT ROOT Certificate and hash. 
    
    return 0;
}

static int64_t dm_svc_update_sw_boot_root_certificate_hash(struct dm_control_block *dm_req) 
{
   printf("hash :%s\n", dm_req->cmd_payload);
   
   //cmd_payload contains the hash for new certificate.
   if(0 != update_sw_boot_root_certificate_hash(dm_req->cmd_payload)) {
        printf(" dm_svc_update_sp_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
        return -1;
   }
   
   return 0;
}
*/

static int64_t update_sp_boot_root_certificate_hash(char *certficate_hash)
{
    static const uint32_t asset_policy = 26;
    static uint32_t asset_size;
    static uint32_t asset_id;

    if (0 != vaultip_drv_static_asset_search(get_rom_identity(),
                                             VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH, &asset_id,
                                             &asset_size)) {
        printf(
            "update_sp_boot_root_certificate_hash: vaultip_drv_static_asset_search(%u) failed!\n",
            VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH);
        return -1;
    }

    if (0 != vaultip_drv_otp_data_write(get_rom_identity(), VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,
                                        asset_policy, true, certficate_hash, asset_size,
                                        (void *)NULL, 0)) {
        printf(" update_sp_boot_root_certificate_hash: vaultip_public_data_write() failed!\n");
        return -1;
    };

    return 0;
}

static int64_t dm_svc_update_sp_boot_root_certificate_hash(struct dm_control_block *dm_req)
{
    printf("recieved hash:\n");
    for (int i=0; i<64;i++)
    {
        printf("%02x ", *(unsigned char*)&dm_req->cmd_payload[i]);
    }

    //cmd_payload contains the hash for new certificate.
    if (0 != update_sp_boot_root_certificate_hash(dm_req->cmd_payload)) {
        printf(
            " dm_svc_update_sp_boot_root_certificate_hash : vault_ip_update_sp_boot_root_certificate_hash failed!\n");
        return -1;
    }

    return 0;
}

void firmware_service_process_request(mbox_e mbox, uint32_t cmd_id, void *buffer)
{
    int64_t ret = 0;
    uint64_t req_start_time;
    struct dm_control_block *dm_cmd_req;
    printf("firmware_service_process_request - start\n");
    printf("cmd_id: %d\n", cmd_id);
    dm_cmd_req = (struct dm_control_block *)buffer;
    req_start_time = timer_get_ticks_count();

    switch (cmd_id) {
    case SET_FIRMWARE_UPDATE: {
        ret = dm_svc_firmware_update(mbox, cmd_id, req_start_time);
        send_status_response(mbox, cmd_id, req_start_time, ret);
    } break;

    case GET_MODULE_FIRMWARE_REVISIONS: {
        dm_svc_get_firmware_version(mbox, cmd_id, req_start_time);
    } break;

    case GET_FIRMWARE_BOOT_STATUS: {
        ret = dm_svc_get_firmware_status();
        send_status_response(mbox, cmd_id, req_start_time, ret);
    } break;

/*  TODO: These feature is to be supported in v 0.0.7 
    case SET_FIRMWARE_VERSION_COUNTER: {
        ret = dm_svc_set_firmware_version_counter();
    } break;

    case SET_FIRMWARE_VALID: {
        ret = dm_svc_set_firmware_valid_counter();
    } break;

    case SET_SW_BOOT_ROOT_CERT: {
        ret = dm_svc_update_sw_boot_root_certificate_hash(dm_cmd_req);
    } break;

*/
    case SET_SP_BOOT_ROOT_CERT: {
        ret = dm_svc_update_sp_boot_root_certificate_hash(dm_cmd_req);
        send_status_response(mbox, cmd_id, req_start_time, ret);
    } break;
    }
    printf("firmware_service_process_request - end\n");
}
