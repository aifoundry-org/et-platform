#include <stdio.h>
#include <string.h>
#include "bl2_asset_trk.h"
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


static uint32_t PCIE_SPEED[5] = {PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5};

int64_t dm_service_asset_get_fw_version(char *fw_version)
{
   (void)fw_version;
   const IMAGE_VERSION_INFO_t * image_version_info = get_service_processor_bl2_image_info();
   printf("SP BL2 version: %u.%u.%u:%s (" BL2_VARIANT ")\n",
        image_version_info->file_version_major,
        image_version_info->file_version_minor,
        image_version_info->file_version_revision, image_version_info->git_version);
 
   ESPERANTO_IMAGE_FILE_HEADER_t *image_file_header;

   image_file_header = get_mm_image_file_header();

   printf("Master Minion FW Details. File version:%d Git Version: %s\n",
        image_file_header->info.image_info_and_signaure.info.public_info.file_version,
                image_file_header->info.image_info_and_signaure.info.public_info.git_version);
  
   image_file_header = get_wm_image_file_header();

   printf("Worker Minion FW Details. File version:%d Git Version: %s\n",
        image_file_header->info.image_info_and_signaure.info.public_info.file_version,
        image_file_header->info.image_info_and_signaure.info.public_info.git_version);


   //TODO: use a string/stringbuilder and concatenate the major/minor/revision info.
   //      and return it using fw_version 
   

   return 0;
}

int64_t dm_service_asset_get_manufacturer_name(char *mfg_name)
{
   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   memcpy(mfg_name, "Esperanto", strlen("Esperanto"));
   return 0;
}

int64_t dm_service_asset_get_part_number(char *part_num)
{
   
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    memcpy(part_num, "PARTNO1", strlen("PARTNO1"));
    return 0;
}

int64_t dm_service_asset_get_serial_number(char *ser_num)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    memcpy(ser_num, "SERNO1", strlen("SERNO1"));
    return 0;
}

int64_t dm_service_asset_get_chip_rev(uint32_t *chip_rev)
{
    (void)chip_rev;
    printf("Silicon Revision Info...\n");
    OTP_SILICON_REVISION_t silicon_revision;
    if (0 != sp_otp_get_silicon_revision(&silicon_revision)) {
        printf("sp_otp_get_silicon_revision() failed!\n");
        return -1;
    }

    *chip_rev = (uint32_t)((silicon_revision.B.si_major_rev  << 4 ) | silicon_revision.B.si_minor_rev); 

    return 0;

}

int64_t dm_service_asset_get_PCIE_speed(uint32_t *speed)
{

   uint32_t pcie_gen,tmp;
   tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

   /* Get the PCIE Gen */
   pcie_gen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);
   
   /* Get the speed */
   *speed = PCIE_SPEED[pcie_gen -1];
   
   return 0;
}

int64_t dm_service_asset_get_module_rev(uint32_t *rev)
{
   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   *rev = 1.0;
   return 0;
}

int64_t dm_service_asset_get_form_factor(char *form_factor)
{
   // TODO : Retrieve this parameter from BL2 partition header SW-4327
   memcpy(form_factor, "M.2", strlen("M.2"));
   return 0;
}

int64_t dm_service_asset_get_memory_details(char *mem_vendor, char* mem_part)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   memcpy(mem_vendor, "Micron", strlen("Micron"));
   memcpy(mem_part, "ETDDR4", strlen("ETDDR4"));
   return 0;
}

int64_t dm_service_asset_get_memory_size(uint32_t *mem_size)
{
   //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   *mem_size = 16;
   return 0;
}

int64_t dm_service_asset_get_memory_type(char *mem_type)
{
   //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
   memcpy(mem_type, "LPDDR4", strlen("LPDDR4"));
   return 0;
}

void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id)
{

    int64_t ret=0;
    uint32_t chip_rev,speed, module_revision, mem_size;
    char fw_version[FW_VERSION_MAX_LENGTH],mfg_name[MFG_NAME_MAX_LENGTH],part_number[PART_NUMBER_MAX_LENGTH];          
    char serial_number[SERIAL_NUMBER_MAX_LENGTH], mem_vendor[MEM_VENDOR_MAX_LENGTH], mem_part[MEM_PART_MAX_LENGTH];
    char  form_factor[FORM_FACTOR_MAX_LENGTH], mem_type[MEM_TYPE_MAX_LENGTH];
 
    struct dm_rsp_t rsp;
    
    rsp.dm_response_info.message_id = cmd_id;
    rsp.dm_response_info.command_info.command_id = cmd_id;

    switch (cmd_id)
    {
           case DM_SVC_ASSET_GET_MFG_NAME: {
		ret = dm_service_asset_get_manufacturer_name(mfg_name);
		if (!ret){
                    printf("dm_service_asset_get_manufacturer_name: %s \r\n", mfg_name); 
		    strcpy(rsp.rsp_char, mfg_name); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
                }   
		else{
		    printf("dm_service_asset_get_manufacturer_name error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_PART_NUMBER: {
		ret = dm_service_asset_get_part_number(part_number);
		if (!ret){
                    printf("dm_service_asset_get_part_number: %s \r\n", part_number);
		    strcpy(rsp.rsp_char,part_number); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp)); 
		}
                else{
		    printf("dm_service_asset_get_part_number error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_FW_VERSION: {
		ret = dm_service_asset_get_fw_version(fw_version);
		if (!ret){
                    printf("dm_service_asset_get_fw_version: %s \r\n", fw_version);
		    strcpy(rsp.rsp_char,fw_version); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
		}
                else{
		    printf("dm_service_asset_get_fw_version error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_SERIAL_NUMBER: {
		ret = dm_service_asset_get_serial_number(serial_number);
		if (!ret) {
                    printf("dm_service_asset_get_serial_number: %s \r\n", serial_number);
		    strcpy(rsp.rsp_char,serial_number); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
		}
                else{
		    printf("dm_service_asset_get_serial_number error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_CHIP_REVISION: {
		chip_rev = 0;
		ret = dm_service_asset_get_chip_rev(&chip_rev);
		if (!ret){
                    printf("dm_service_asset_get_chip_rev: %d \r\n", chip_rev);
		    memcpy(rsp.rsp_char,(char *)&chip_rev, sizeof(unsigned int)); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
                }
		else {
		    printf("dm_service_asset_get_chip_rev error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_PCIE_PORTS: {
		ret = dm_service_asset_get_PCIE_speed(&speed);
		if (!ret){
                    printf("dm_service_asset_get_PCIE_speed: %d \r\n", speed);
		    memcpy(rsp.rsp_char,(char *)&speed, sizeof(unsigned int)); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
                }
		else {
		    printf("dm_service_asset_get_PCIE_speed error %ld\r\n", ret);
                }
	    }
	     break;


	    case DM_SVC_ASSET_GET_MODULE_REV: {
		ret = dm_service_asset_get_module_rev(&module_revision);
		if (!ret){
                    printf("dm_service_asset_get_module_rev: %d \r\n", module_revision);
		    memcpy(rsp.rsp_char,(char *)&module_revision, sizeof(unsigned int)); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
                }
                else {
		    printf("dm_service_asset_get_module_rev error %ld\r\n", ret);
                }
	    }
	     break;


	    case DM_SVC_ASSET_GET_FORM_FACTOR: {
		ret = dm_service_asset_get_form_factor(form_factor);
		if (!ret) {
                    printf("dm_service_asset_get_form_factor: %s \r\n", form_factor);
		    strcpy(rsp.rsp_char,form_factor); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
		}
                else {
		    printf("dm_service_asset_get_form_factor error %ld\r\n", ret);
                }	    
            }
	     break;


	    case DM_SVC_ASSET_GET_MEMORY_DETAILS: {
		ret = dm_service_asset_get_memory_details(mem_vendor, mem_part);
		if (!ret){
                     printf("dm_service_asset_get_memory_details: mem_venor -%s , mem_part -%s \r\n", mem_vendor ,mem_part);
		    strcpy(rsp.rsp_char,mem_vendor); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
		}
                else{
 		    printf("dm_service_asset_get_memory_details error %ld\r\n", ret);
                }
	    }
	     break;

	    case DM_SVC_ASSET_GET_MEMORY_SIZE: {
		ret = dm_service_asset_get_memory_size(&mem_size);
		if (!ret){
                    printf("dm_service_asset_get_memory_size: %d \r\n", mem_size);
		    memcpy(rsp.rsp_char, (char *)&mem_size,sizeof(unsigned int)); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));
		}
                else{
		    printf("dm_service_asset_get_memory_size error %ld\r\n", ret);
               }
	    }
	     break;

	    case DM_SVC_ASSET_GET_MEMORY_TYPE: {
		ret = dm_service_asset_get_memory_type(mem_type);
		if (!ret){
                    printf("dm_service_asset_get_memory_type: %s \r\n", mem_type);
		    strcpy(rsp.rsp_char,mem_type); 
                    ret = MBOX_send(mbox, &rsp, sizeof(rsp));

                }
		else{
		    printf("dm_service_asset_get_memory_type error %ld\r\n", ret);
                }
	    }
	    break;

    }

    if (ret != 0)
    {
        printf("MBOX Send error %ld\r\n", ret);
    }
}
