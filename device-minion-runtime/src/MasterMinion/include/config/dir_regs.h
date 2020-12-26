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
*       Master Minion's Device Interface Registers.
*
***********************************************************************/

#ifndef __DIR_REGS_H__
#define __DIR_REGS_H__

#include <stdint.h>
#include "config/mm_config.h"
#include "hal_device.h"

/*! \file List of REGIONS based on Spec as defined here: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map */

#define MM_DEV_INTF_REG_VERSION 1U

// MM DEV Interface Register at PC_MM Mailbox + 1K
#define MM_DEV_INTF_BASE_ADDR (R_PU_MBOX_PC_MM_BASEADDR + 0x400UL)

// Only expose Bar based offset,size as address to Host
// Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping

//TODO: place these macros in their rightful home.
// DDR Region 0 USER_KERNEL_SPACE (BAR=0, Offset=4GB, Size=8GB)
#define MM_DEV_INTF_USER_KERNEL_SPACE_BAR    0
#define MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET 0x0100000000UL // TODO: Should be HOST_MANAGED_DRAM_START
#define MM_DEV_INTF_USER_KERNEL_SPACE_SIZE   0x0200000000UL // TODO: Should be (HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START)

/*! \enum MM_DEV_INTF_MM_BOOT_STATUS_e
    \brief Values representing Master Minion Boot status.
*/
enum MM_DEV_INTF_MM_BOOT_STATUS_e {
    MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_MB_TIMEOUT = -2,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_ERROR,
    MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_NOT_READY = 0,
    MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_VQ_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_DDR_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_LAUNCHED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_MBOX_INITIALIZED
};

/*! \enum MM_DEV_INTF_DDR_REGION_ATTRIBUTE_e
    \brief Values representing BAR numbers.
*/
enum MM_DEV_INTF_DDR_REGION_ATTRIBUTE_e {
    MM_DEV_INTF_DDR_REGION_ATTR_READ_ONLY = 0,
    MM_DEV_INTF_DDR_REGION_ATTR_WRITE_ONLY,
    MM_DEV_INTF_DDR_REGION_ATTR_READ_WRITE
};

/*! \enum MM_DEV_INTF_DDR_REGION_MAP_e
    \brief Values representing DDR region related information.
*/
enum MM_DEV_INTF_DDR_REGION_MAP_e {
    MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE = 0,
    MM_DEV_INTF_DDR_REGION_MAP_NUM
};

/*! \enum MM_DEV_INTF_BAR_TYPE_e
    \brief Values representing BAR numbers.
*/
enum MM_DEV_INTF_BAR_TYPE_e {
    MM_DEV_INTF_BAR_0 = 0,
    MM_DEV_INTF_BAR_2 = 2,
    MM_DEV_INTF_BAR_4 = 4
};

/*! \struct MM_DEV_INTF_DDR_REGION_s
    \brief Holds the information of Master Minion DDR region.
    \warning Must be 64-bit aligned.
*/
typedef struct MM_DEV_INTF_DDR_REGION_ {
    uint16_t attr;
    uint16_t bar;
    uint64_t offset;
    uint64_t devaddr;
    uint64_t size;
    uint32_t reserved;
} MM_DEV_INTF_DDR_REGION_s;

/*! \struct MM_DEV_INTF_DDR_REGIONS_s
    \brief Holds the information of all the available Master Minion 
    DDR regions.
    \warning Must be 64-bit aligned.
*/
typedef struct MM_DEV_INTF_DDR_REGIONS_ {
    uint32_t reserved;
    uint32_t num_regions;
    MM_DEV_INTF_DDR_REGION_s regions[MM_DEV_INTF_DDR_REGION_MAP_NUM];
} MM_DEV_INTF_DDR_REGIONS_s;

/*! \struct MM_DEV_INTF_MM_VQ_s
    \brief Holds the information of Master Minion Virtual Queues.
    \warning Must be 64-bit aligned.
*/
typedef struct MM_DEV_INTF_MM_VQ_ {
    uint8_t reserved[3];
    uint8_t bar;
    uint32_t bar_size;
    uint32_t sq_offset;
    uint16_t sq_count;
    uint16_t per_sq_size;
    uint32_t cq_offset;
    uint16_t cq_count;
    uint16_t per_cq_size;
} MM_DEV_INTF_MM_VQ_s;

/*! \struct MM_DEV_INTF_REG_s
    \brief Master Minion DIR which will be used to public device capability to Host.
    \warning Must be 64-bit aligned.
*/
typedef struct MM_DEV_INTF_REG_ {
    uint32_t version;
    uint32_t size;
    MM_DEV_INTF_MM_VQ_s mm_vq;
    MM_DEV_INTF_DDR_REGIONS_s ddr_regions;
    uint32_t reserved;
    int32_t status;
} MM_DEV_INTF_REG_s;

// Macro to extract MM DIRs
#define MM_DEV_INTF_GET_BASE   ((volatile MM_DEV_INTF_REG_s *)MM_DEV_INTF_BASE_ADDR)

/*! \fn void DIR_Init(void)
    \brief Initialize Device Interface Registers
    \param None
*/
void DIR_Init(void);

/*! \fn void DIR_Set_Master_Minion_Status(uint8_t status)
    \brief Set Master Minion ready status
    \param None
*/
void DIR_Set_Master_Minion_Status(uint8_t status);

#endif /* DIR_REGS_H */
