#ifndef ASSET_TRACKING_SERVICE_H
#define ASSET_TRACKING_SERVICE_H

#include <stdint.h>
#include "mailbox.h"

// PCIE gen bit rates(GT/s) definition
#define PCIE_GEN_1   2
#define PCIE_GEN_2   5
#define PCIE_GEN_3   8
#define PCIE_GEN_4   16
#define PCIE_GEN_5   32

// Function prototypes 
int64_t dm_service_asset_get_fw_version(char *fw_version);
int64_t dm_service_asset_get_manufacturer_name(char *mfg_name);
int64_t dm_service_asset_get_part_number(char *part_num);
int64_t dm_service_asset_get_serial_number(char *ser_num);
int64_t dm_service_asset_get_chip_rev(uint32_t *chip_rev);
int64_t dm_service_asset_get_PCIE_speed(uint32_t *speed);
int64_t dm_service_asset_get_module_rev(uint32_t *rev);
int64_t dm_service_asset_get_form_factor(char *form_factor);
int64_t dm_service_asset_get_memory_details(char *mem_vendor, char* mem_part);
int64_t dm_service_asset_get_memory_size(uint32_t *mem_size);
int64_t dm_service_asset_get_memory_type(char *mem_type);
void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id);

#endif
