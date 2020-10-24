#include <stdio.h>
#include <string.h>
#include "bl2_asset_trk.h"
#include "bl2_timer.h"
#include "service_processor_BL2_data.h"
#include "bl2_build_configuration.h"
#include "bl2_firmware_loader.h"
#include "mailbox.h"
#include "bl2_main.h"
#include "esr_defines.h"
#include "sp_otp.h"
#include "io.h"
#include "pcie_device.h"

static uint32_t PCIE_SPEED[5] = { PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5 };

static int64_t dm_svc_asset_getfwversion(char *fwVersion)
{
    char fwVerTemp[8];
    const IMAGE_VERSION_INFO_t *image_version_info = get_service_processor_bl2_image_info();

    /*
   printf("SP BL2 version: %u.%u.%u:%s (" BL2_VARIANT ")\n",
        image_version_info->file_version_major,
        image_version_info->file_version_minor,
        image_version_info->file_version_revision, image_version_info->git_version);
*/
    sprintf(fwVerTemp, "v-%u.%u.%u", image_version_info->file_version_major,
            image_version_info->file_version_minor, image_version_info->file_version_revision);

    /*
   ESPERANTO_IMAGE_FILE_HEADER_t *image_file_header;

   image_file_header = get_mm_image_file_header();

   printf("Master Minion FW Details. File version:%d Git Version: %s\n",
        image_file_header->info.image_info_and_signaure.info.public_info.file_version,
                image_file_header->info.image_info_and_signaure.info.public_info.git_version);
  
   image_file_header = get_wm_image_file_header();

   printf("Worker Minion FW Details. File version:%d Git Version: %s\n",
        image_file_header->info.image_info_and_signaure.info.public_info.file_version,
        image_file_header->info.image_info_and_signaure.info.public_info.git_version);
*/

    //TODO : Need to return other firmware version as well.
    strncpy(fwVersion, fwVerTemp, MAX_LENGTH);

    return 0;
}

static int64_t dm_svc_asset_getmanufacturername(char *mfgName)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(mfgName, "Esperant");
    return 0;
}

static int64_t dm_svc_asset_getpartnumber(char *partNumber)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(partNumber, "ETPART01");
    return 0;
}

static int64_t dm_svc_asset_getserialnumber(char *serNumber)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(serNumber, "ETSERNO1");
    return 0;
}

static int64_t dm_svc_asset_getchiprevision(char *chipRev)
{
    uint64_t chipRevision;
    printf("Silicon Revision Info...\n");
    OTP_SILICON_REVISION_t silicon_revision;
    if (0 != sp_otp_get_silicon_revision(&silicon_revision)) {
        printf("sp_otp_get_silicon_revision() failed!\n");
        return -1;
    }

    chipRevision =
        (uint32_t)((silicon_revision.B.si_major_rev << 4) | silicon_revision.B.si_minor_rev);

    sprintf(chipRev, "%ld", chipRevision);

    return 0;
}

static int64_t dm_svc_asset_getPCIEspeed(char *pcieSpeed)
{
    uint32_t pcieGen, tmp;
    uint64_t pcieSpeedtmp;
    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

    // Get the PCIE Gen
    pcieGen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);

    // Get the speed
    pcieSpeedtmp = PCIE_SPEED[pcieGen - 1];

    sprintf(pcieSpeed, "%ld", pcieSpeedtmp);

    return 0;
}

static int64_t dm_svc_asset_getmodulerev(char *moduleRevision)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    uint64_t revision = 1;
    sprintf(moduleRevision, "%ld", revision);
    return 0;
}

static int64_t dm_svc_asset_getformfactor(char *formFactor)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(formFactor, "Dual_M2");
    return 0;
}

static int64_t dm_svc_asset_getmemorydetails(char *memVendor, char *memPart)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    strcpy(memVendor, "Micron");
    strcpy(memPart, "ETDDR4");
    return 0;
}

static int64_t dm_svc_asset_getmemorysize(char *memSize)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    sprintf(memSize, "%ld", (uint64_t)16);
    return 0;
}

static int64_t dm_svc_asset_getmemorytype(char *memType)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    strcpy(memType, "ETLPDDR4");
    return 0;
}

static int64_t asset_tracking_send_response(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time,
                                            char asset_info[])
{
    int64_t ret = 0;
    uint64_t rsp_complete_time;
    struct dm_control_block dm_cmd_rsp;
    rsp_complete_time = timer_get_ticks_count();
    dm_cmd_rsp.cmd_id = cmd_id;
    dm_cmd_rsp.dev_latency = (rsp_complete_time - req_start_time);
    strncpy(dm_cmd_rsp.cmd_payload, asset_info, MAX_LENGTH);
    ret = MBOX_send(mbox, &dm_cmd_rsp, sizeof(struct dm_control_block));
    return ret;
}

void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id)
{
    int64_t ret = 0;
    char req_asset_info[MAX_LENGTH] = { 0 }, mem_part[MAX_LENGTH] = { 0 };
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (cmd_id) {
    case GET_MODULE_MANUFACTURE_NAME: {
        ret = dm_svc_asset_getmanufacturername(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getmanufacturername: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);

        } else {
            printf("dm_svc_asset_getmanufacturername error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_PART_NUMBER: {
        ret = dm_svc_asset_getpartnumber(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getpartnumber: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getpartnumber error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_FIRMWARE_REVISIONS: {
        ret = dm_svc_asset_getfwversion(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getfwversion: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getfwversion error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_SERIAL_NUMBER: {
        ret = dm_svc_asset_getserialnumber(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getserialnumber: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getserialnumber error %ld\r\n", ret);
        }
    } break;

    case GET_ASIC_CHIP_REVISION: {
        ret = dm_svc_asset_getchiprevision(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getchiprevision: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getchiprevision error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
        ret = dm_svc_asset_getPCIEspeed(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getPCIEspeed: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getPCIEspeed error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_REVISION: {
        ret = dm_svc_asset_getmodulerev(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getmodulerev: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getmodulerev error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_FORM_FACTOR: {
        ret = dm_svc_asset_getformfactor(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getformfactor: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getformfactor error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_MEMORY_VENDOR_PART_NUMBER: {
        ret = dm_svc_asset_getmemorydetails(req_asset_info, mem_part);
        if (!ret) {
            printf("dm_svc_asset_getmemorydetails: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getmemorydetails error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_MEMORY_SIZE_MB: {
        ret = dm_svc_asset_getmemorysize(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getmemorysize: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getmemorysize error %ld\r\n", ret);
        }
    } break;

    case GET_MODULE_MEMORY_TYPE: {
        ret = dm_svc_asset_getmemorytype(req_asset_info);
        if (!ret) {
            printf("dm_svc_asset_getmemorytype: %s \r\n", req_asset_info);
            ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
        } else {
            printf("dm_svc_asset_getmemorytype error %ld\r\n", ret);
        }
    } break;
    }

    if (ret != 0) {
        printf("MBOX Send error %ld\r\n", ret);
    }
}