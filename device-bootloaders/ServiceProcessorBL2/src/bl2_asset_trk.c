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

#include <esperanto/device-api/device_management.h>


extern uint64_t request_start_time;
extern uint64_t response_complete_time;

static uint32_t PCIE_SPEED[5] = {PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5};

static int64_t dmServiceAssetGetfwVersion(char *fwVersion) {
   char fwVerTemp[8];
   const IMAGE_VERSION_INFO_t * image_version_info = get_service_processor_bl2_image_info();
   
/*
   printf("SP BL2 version: %u.%u.%u:%s (" BL2_VARIANT ")\n",
        image_version_info->file_version_major,
        image_version_info->file_version_minor,
        image_version_info->file_version_revision, image_version_info->git_version);
*/ 
   sprintf(fwVerTemp, "v-%u.%u.%u", image_version_info->file_version_major,
        image_version_info->file_version_minor,
        image_version_info->file_version_revision);

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
   strncpy(fwVersion, fwVerTemp, 8);

   return 0;
}

static int64_t dmServiceAssetGetManufactureName(char *mfgName) {
   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   strcpy(mfgName, "Esperant");
   return 0;
}

static int64_t dmServiceAssetGetPartNumber(char *partNumber) {
   
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(partNumber, "ETPART01");
    return 0;
}

static int64_t dmServiceAssetGetSerialNumber(char *serNumber) {
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(serNumber, "ETSERNO1");
    return 0;
}

static int64_t dmServiceAssetGetChipRev(char *chipRev) {
    uint64_t chipRevision;
    printf("Silicon Revision Info...\n");
    OTP_SILICON_REVISION_t silicon_revision;
    if (0 != sp_otp_get_silicon_revision(&silicon_revision)) {
        printf("sp_otp_get_silicon_revision() failed!\n");
        return -1;
    }

    chipRevision = (uint32_t)((silicon_revision.B.si_major_rev  << 4 ) | silicon_revision.B.si_minor_rev); 

    sprintf(chipRev, "%ld", chipRevision);

    return 0;

}

static int64_t dmServiceAssetGetPCIESpeed(char *pcieSpeed) {

   uint32_t pcieGen,tmp;
   uint64_t pcieSpeedtmp;
   tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

   // Get the PCIE Gen 
   pcieGen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);
   
   // Get the speed
   pcieSpeedtmp = PCIE_SPEED[pcieGen -1];

   sprintf(pcieSpeed, "%ld", pcieSpeedtmp);
   
   return 0;
}

static int64_t dmServiceAssetGetModuleRev(char *moduleRevision) {

   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   uint64_t revision = 1;
   sprintf(moduleRevision, "%ld", revision);
   return 0;
}

static int64_t dmServiceAssetGetFormFactor(char *formFactor) {

   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   strcpy(formFactor, "Dual_M2");
   return 0;
}

static int64_t dmServiceAssetGetMemoryDetails(char *memVendor, char* memPart) {

    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   strcpy(memVendor, "Micron");
   strcpy(memPart, "ETDDR4");
   return 0;
}

static int64_t dmServiceAssetGetMemorySize(char *memSize) {

   //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   sprintf(memSize, "%ld", (uint64_t)16);
   return 0;
}

static int64_t dmServiceAssetGetMemoryType(char *memType) {

   //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   strcpy(memType, "ETLPDDR4");
   return 0;
}

//TODO : Remove the first parameter and fix the dependent changes
struct dmControlBlock *create_dmctrlblk(struct dmControlBlock *dmcntlblk, uint32_t cmdID, uint32_t latency, uint32_t payloadSize, char payload[]) { 

    dmcntlblk = (struct dmControlBlock *)malloc(sizeof(cmdID) + sizeof(latency) + payloadSize); 
    dmcntlblk->cmd_id = cmdID; 
    dmcntlblk->dev_latency = latency; 
    strcpy(dmcntlblk->cmd_payload, payload); 
      
    return dmcntlblk;
} 

//TODO: Refactor this function
void assetTrackingProcessRequest(mbox_e mbox, uint32_t cmd_id) {

    int64_t ret=0;
    uint32_t latency=0,dm_cmd_rsp_size=0;
    char fw_version[MAX_LENGTH],mfg_name[MAX_LENGTH]={0},part_number[MAX_LENGTH], chip_rev[MAX_LENGTH];
    char serial_number[MAX_LENGTH], mem_vendor[MAX_LENGTH], mem_part[MAX_LENGTH], module_revision[MAX_LENGTH]={0};
    char form_factor[MAX_LENGTH], mem_type[MAX_LENGTH], mem_size[MAX_LENGTH]={0}, speed[MAX_LENGTH]={0};
    
    struct dmControlBlock *dm_cmd_rsp = NULL;

    switch (cmd_id)
    {
        case GET_MODULE_MANUFACTURE_NAME: {
             ret = dmServiceAssetGetManufactureName(mfg_name);
             if (!ret) {
                printf("dmServiceAssetGetManufactureName: %s \r\n", mfg_name);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, mfg_name);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetManufactureName error %ld\r\n", ret);
             }
            } break;

        case GET_MODULE_PART_NUMBER: {
             ret = dmServiceAssetGetPartNumber(part_number);
             if (!ret) {
                printf("dmServiceAssetGetPartNumber: %s \r\n", part_number);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, part_number);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetPartNumber error %ld\r\n", ret);
             } 
            } break;

         case GET_MODULE_FIRMWARE_REVISIONS: {
              ret = dmServiceAssetGetfwVersion(fw_version);
              if (!ret) {
                 printf("dmServiceAssetGetfwVersion: %s \r\n", fw_version);
                 response_complete_time = timer_get_ticks_count();
                 latency =  (uint32_t)(response_complete_time - request_start_time);
                 dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, fw_version);
                 dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                 ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
              } else {
                printf("dmServiceAssetGetfwVersion error %ld\r\n", ret);
              }
             } break;

        case GET_MODULE_SERIAL_NUMBER: {
             ret = dmServiceAssetGetSerialNumber(serial_number);
             if (!ret) {
                printf("dmServiceAssetGetSerialNumber: %s \r\n", serial_number);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, serial_number);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetSerialNumber error %ld\r\n", ret);
             }
            } break;

        case GET_ASIC_CHIP_REVISION: {
             ret = dmServiceAssetGetChipRev(chip_rev);
             if (!ret) {
                printf("dmServiceAssetGetChipRev: %s \r\n", chip_rev);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, chip_rev);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetChipRev error %ld\r\n", ret);
             }
            } break;

        case GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
             ret = dmServiceAssetGetPCIESpeed(speed);
             if (!ret) {
                printf("dmServiceAssetGetPCIESpeed: %s \r\n", speed);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, speed);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetPCIESpeed error %ld\r\n", ret);
             }
            } break;


        case GET_MODULE_REVISION: {
             ret = dmServiceAssetGetModuleRev(module_revision);
             if (!ret) {
                printf("dmServiceAssetGetModuleRev: %s \r\n", module_revision);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, module_revision);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetModuleRev error %ld\r\n", ret);
             }
            } break;

        case GET_MODULE_FORM_FACTOR: {
             ret = dmServiceAssetGetFormFactor(form_factor);
             if (!ret) {
                printf("dmServiceAssetGetFormFactor: %s \r\n", form_factor);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, form_factor);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetFormFactor error %ld\r\n", ret);
             }
            } break;


        case GET_MODULE_MEMORY_VENDOR_PART_NUMBER: {
             ret = dmServiceAssetGetMemoryDetails(mem_vendor, mem_part);
             if (!ret) {
                printf("dmServiceAssetGetMemoryDetails: %s \r\n", mem_vendor);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, mem_vendor);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetMemoryDetails error %ld\r\n", ret);
             }
            } break;

        case GET_MODULE_MEMORY_SIZE_MB: {
             ret = dmServiceAssetGetMemorySize(mem_size);
             if (!ret) {
                printf("dmServiceAssetGetMemorySize: %s \r\n", mem_size);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, mem_size);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetMemorySize error %ld\r\n", ret);
             }
            } break;

        case GET_MODULE_MEMORY_TYPE: {
             ret = dmServiceAssetGetMemoryType(mem_type);
             if (!ret) {
                printf("dmServiceAssetGetMemoryType: %s \r\n", mem_type);
                response_complete_time = timer_get_ticks_count();
                latency =  (uint32_t)(response_complete_time - request_start_time);
                dm_cmd_rsp = create_dmctrlblk(dm_cmd_rsp, cmd_id, latency, MAX_LENGTH, mem_type);
                dm_cmd_rsp_size = (sizeof(cmd_id) + sizeof(latency) + MAX_LENGTH);
                ret = MBOX_send(mbox, dm_cmd_rsp, dm_cmd_rsp_size);
             } else {
                printf("dmServiceAssetGetMemoryType error %ld\r\n", ret);
             }
            } break;
    }

    if (ret != 0) {
        printf("MBOX Send error %ld\r\n", ret);
    }

    if (dm_cmd_rsp != NULL) {
        free(dm_cmd_rsp);
    }
}
