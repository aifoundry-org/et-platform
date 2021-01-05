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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface to access, initialize and manage 
*       Service Processor's Device Interface Registers.
*
***********************************************************************/

#ifndef __DIR_REGS_H__
#define __DIR_REGS_H__

#include <stdint.h>
#include "config/mgmt_dir_build_config.h"

/*! \file List of REGIONS based on Spec as defined here: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map */

#define SP_DEV_INTF_REG_VERSION 1U

// SP Dev Interface Register at PC_SP Mailbox + 1K
#define SP_DEV_INTF_BASE_ADDR (R_PU_MBOX_PC_SP_BASEADDR + 0x400UL)

// Only expose Bar based offset,size as address to Host
// Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping

// DDR Region 0 USER_KERNEL_SPACE (BAR=0, Offset=4GB, Size=8GB)
#define SP_DEV_INTF_USER_KERNEL_SPACE_BAR    0
#define SP_DEV_INTF_USER_KERNEL_SPACE_OFFSET 0x0100000000UL
#define SP_DEV_INTF_USER_KERNEL_SPACE_SIZE   0x0200000000UL
// DDR Region 1 FIRMWARE_UPDATE_SCRATCH (BAR=0, Offset=8MB, Size=4MB)
#define SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_BAR    0
#define SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_OFFSET 0x0800000UL
#define SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_SIZE   0x0400000UL
// DDR Region 2 MAP_MSIX_TABLE (BAR=0, Offset=12MB, Size=256K)
#define SP_DEV_INTF_MSIX_TABLE_BAR    0
#define SP_DEV_INTF_MSIX_TABLE_OFFSET 0x0C00000UL
#define SP_DEV_INTF_MSIX_TABLE_SIZE   0x0040000UL

/*! \enum SP_DEV_INTF_SP_BOOT_STATUS_e
    \brief Values representing Master Minion Boot status.
*/
enum SP_DEV_INTF_SP_BOOT_STATUS_e {
    SP_DEV_INTF_SP_BOOT_STATUS_BOOT_ERROR=-1,
    SP_DEV_INTF_SP_BOOT_STATUS_DEV_NOT_READY = 0,
    SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_NOC_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_MINION_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED,
    SP_DEV_INTF_SP_BOOT_STATUS_ATU_PROGRAMMED,
    SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY
};

/*! \enum SP_DEV_INTF_DDR_REGION_ATTRIBUTE_e
    \brief Values representing BAR numbers.
*/
enum SP_DEV_INTF_DDR_REGION_ATTRIBUTE_e {
    SP_DEV_INTF_DDR_REGION_ATTR_READ_ONLY = 0,
    SP_DEV_INTF_DDR_REGION_ATTR_WRITE_ONLY,
    SP_DEV_INTF_DDR_REGION_ATTR_READ_WRITE
};

/*! \enum SP_DEV_INTF_DDR_REGION_MAP_e
    \brief Values representing DDR region related information.
*/
enum SP_DEV_INTF_DDR_REGION_MAP_e {
    SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE = 0,
    SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH,
    SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE,
    SP_DEV_INTF_DDR_REGION_MAP_NUM
};

/*! \enum SP_DEV_INTF_BAR_TYPE_e
    \brief Values representing BAR numbers.
*/
enum SP_DEV_INTF_BAR_TYPE_e {
    SP_DEV_INTF_BAR_0 = 0,
    SP_DEV_INTF_BAR_2 = 2,
    SP_DEV_INTF_BAR_4 = 4
};

/*! \struct SP_DEV_INTF_DDR_REGION_s
    \brief Holds the information of Service Processor DDR region.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_DDR_REGION_ {
    uint16_t attr;
    uint16_t bar;
    uint64_t offset;
    uint64_t size;
    uint32_t reserved;
} SP_DEV_INTF_DDR_REGION_s;

/*! \struct SP_DEV_INTF_DDR_REGIONS_s
    \brief Holds the information of all the available Service Processor 
    DDR regions.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_DDR_REGIONS_ {
    uint32_t reserved;
    uint32_t num_regions;
    SP_DEV_INTF_DDR_REGION_s regions[SP_DEV_INTF_DDR_REGION_MAP_NUM];
} SP_DEV_INTF_DDR_REGIONS_s;

/*! \struct SP_DEV_INTF_SP_VQ_s
    \brief Holds the information of Service Processor Virtual Queues.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_SP_VQ_ {
    uint8_t reserved[3];
    uint8_t bar;
    uint32_t bar_size;
    uint32_t sq_offset;
    uint16_t sq_count;
    uint16_t per_sq_size;
    uint32_t cq_offset;
    uint16_t cq_count;
    uint16_t per_cq_size;
} SP_DEV_INTF_SP_VQ_s;

/*! \struct SP_DEV_INTF_REG_s
    \brief Service Processor DIR which will be used to public device capability to Host.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_REG_ {
    uint32_t version;
    uint32_t size;
    uint64_t minion_shires; 
    SP_DEV_INTF_SP_VQ_s sp_vq;
    SP_DEV_INTF_DDR_REGIONS_s ddr_regions;
    uint32_t reserved;
    int32_t status;
} SP_DEV_INTF_REG_s;

/*! \fn void DIR_Init(void)
    \brief Initialize Device Interface Registers
    \param None
*/
void DIR_Init(void);

/*! \fn void DIR_Set_Service_Processor_Status(uint8_t status)
    \brief Set Service Processor ready status
    \param None
*/
void DIR_Set_Service_Processor_Status(uint8_t status);

/*! \fn void DIR_Set_Minion_Shires(uint64_t minion_shires)
    \brief Set the available minion shires in system
    \param None
*/
void DIR_Set_Minion_Shires(uint64_t minion_shires);

#endif /* DIR_REGS_H */
