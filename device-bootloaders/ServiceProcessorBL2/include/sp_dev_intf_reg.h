/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __SP_DEV_INTF_REG_H__
#define __SP_DEV_INTF_REG_H__

#include <stdint.h>
#include <assert.h>

#define DEV_INTF_REG_VERSION 1U 

// SP Dev Interface Register at PC_SP Mailbox + 1K 
#define DEV_INTF_BASE_ADDR (R_PU_MBOX_PC_SP_BASEADDR + 0x400UL)

// Only expose Bar based offset,size as address to Host
// Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping
 
// SP Virtual Queue (BAR =2, Offset=2KB ,Size= 1 KB) 
#define SP_VQ_BAR     2
#define SP_VQ_OFFSET  0x800UL
#define SP_VQ_SIZE    0x400UL

// DDR Region 0 USER_KERNEL_SPACE (BAR =0, Offset=4GB ,Size= 8 GB)
#define USER_KERNEL_SPACE_BAR    0 
#define USER_KERNEL_SPACE_OFFSET 0x0100000000UL
#define USER_KERNEL_SPACE_SIZE   0x0200000000UL
// DDR Region 1 FIRMWARE_UPDATE_SCRATCH (BAR =0, Offset=8MB ,Size = 4 MB)
#define FIRMWARE_UPDATE_SCRATCH_BAR    0
#define FIRMWARE_UPDATE_SCRATCH_OFFSET 0x0800000UL
#define FIRMWARE_UPDATE_SCRATCH_SIZE   0x0400000UL
// DDR Region 2 MAP_MSIX_TABLE (BAR =0, Offset=12 MB ,Size = 256 K)
#define MSIX_TABLE_BAR    0
#define MSIX_TABLE_OFFSET 0x0C00000UL
#define MSIX_TABLE_SIZE   0x0040000UL


/// \brief Service Processor status register used to indicate Boot Status of SP      
enum SP_BOOT_STATUS_e {
    STAT_VQ_DESC_NOT_READY = 0,
    STAT_BOOT_ERROR,
    STAT_DEV_INTF_READY_INITIALIZED,
    STAT_VQ_DESC_READY,
    STAT_VQ_QUEUE_INITIALIZED,
    STAT_VQ_READY,
    STAT_DDR_INITIALIZED,
    STAT_MM_FW_LAUNCHED,
    STAT_MM_MBOX_INITIALIZED
};
typedef uint8_t SP_BOOT_STATUS_e;

/// \brief List of REGIONS based on Spec as defined here: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map 

enum DDR_REGION_ATTRIBUTE_e {
    ATTR_READ_ONLY  = 0,
    ATTR_WRITE_ONLY,
    ATTR_READ_WRITE
};
typedef uint8_t DDR_REGION_ATTRIBUTE_e;

enum DDR_REGION_MAP_e {
    MAP_USER_KERNEL_SPACE  = 0,
    MAP_FIRMWARE_UPDATE_SCRATCH,
    MAP_MSIX_TABLE
};
typedef uint8_t DDR_REGION_MAP_e;

enum BAR_TYPE_e {
    BAR_0 = 0,
    BAR_2 = 2,
    BAR_4 = 4
};
typedef uint8_t BAR_TYPE_e;

typedef struct DDR_REGION {
    DDR_REGION_ATTRIBUTE_e attr; 
    BAR_TYPE_e bar;
    uint64_t offset;
    uint64_t size;
} DDR_REGION_s;

typedef struct SP_VQ {
    BAR_TYPE_e bar;
    uint64_t offset;
    uint64_t size;
} SP_VQ_s;


/// \brief Service Processor Device interface register will be uses to public device capability to Host 
typedef struct SP_DEV_INTF_REG {
    uint32_t version;
    uint32_t size;
    SP_VQ_s sp_vq;
    DDR_REGION_s ddr_region[sizeof(DDR_REGION_MAP_e)];
    SP_BOOT_STATUS_e status; 
} SP_DEV_INTF_REG_s;

#endif
