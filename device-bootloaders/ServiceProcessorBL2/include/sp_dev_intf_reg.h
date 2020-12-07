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

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SP_DEV_INTF_REG_VERSION 1U

// SP Dev Interface Register at PC_SP Mailbox + 1K
#define SP_DEV_INTF_BASE_ADDR (R_PU_MBOX_PC_SP_BASEADDR + 0x400UL)

// Only expose Bar based offset,size as address to Host
// Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping

// SP Virtual Queue (BAR=2, Offset=2KB ,Size=1KB)
#define SP_VQ_BAR     2U
#define SP_VQ_MSI_ID  1U
#define SP_VQ_OFFSET  0x800UL
#define SP_VQ_SIZE    0x400UL

#define VQ_CONTROL_SIZE       256ULL   // To be optimized as we profile
#define VQ_ELEMENT_COUNT      4U       // # of elements to enable in VQ
#define VQ_ELEMENT_SIZE       128U     // Size of each element within the VQ
#define VQ_ELEMENT_ALIGNMENT  4U       // Aligned to IO Accesses from SP

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

/// \brief Service Processor status register used to indicate Boot Status of SP
enum SP_DEV_INTF_SP_BOOT_STATUS_e {
    SP_DEV_INTF_SP_BOOT_STATUS_VQ_DESC_NOT_READY = 0,
    SP_DEV_INTF_SP_BOOT_STATUS_BOOT_ERROR,
    SP_DEV_INTF_SP_BOOT_STATUS_DEV_INTF_READY_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_VQ_DESC_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_VQ_QUEUE_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED,
    SP_DEV_INTF_SP_BOOT_STATUS_SP_FW_LAUNCHED,
    SP_DEV_INTF_SP_BOOT_STATUS_SP_MBOX_INITIALIZED
};

/// \brief List of REGIONS based on Spec as defined here: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map

enum SP_DEV_INTF_DDR_REGION_ATTRIBUTE_e {
    SP_DEV_INTF_DDR_REGION_ATTR_READ_ONLY = 0,
    SP_DEV_INTF_DDR_REGION_ATTR_WRITE_ONLY,
    SP_DEV_INTF_DDR_REGION_ATTR_READ_WRITE
};

enum SP_DEV_INTF_DDR_REGION_MAP_e {
    SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE = 0,
    SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH,
    SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE,
    SP_DEV_INTF_DDR_REGION_MAP_NUM
};

enum SP_DEV_INTF_BAR_TYPE_e {
    SP_DEV_INTF_BAR_0 = 0,
    SP_DEV_INTF_BAR_2 = 2,
    SP_DEV_INTF_BAR_4 = 4
};

typedef struct __attribute__((__packed__)) SP_DEV_INTF_DDR_REGION {
    uint8_t attr;    /// One of enum SP_DEV_INTF_DDR_REGION_ATTRIBUTE_e
    uint8_t bar;     /// One of enum SP_DEV_INTF_BAR_TYPE_e
    uint64_t offset;
    uint64_t size;
} SP_DEV_INTF_DDR_REGION_s;

typedef struct __attribute__((__packed__)) SP_DEV_INTF_VQ_SIZE_INFO {
    uint16_t control_size;      /// Region always placed at the start of SP VQ Bar contains vqueue_info structs
    uint16_t element_count;     /// Number of fixed-size buffers present in a VQ
    uint16_t element_size;      /// Size of each fixed-size buffer
    uint16_t element_alignment; /// Alignment requirement of each fixed-sized buffer
} SP_DEV_INTF_VQ_SIZE_INFO_s;

typedef struct __attribute__((__packed__)) SP_DEV_INTF_SP_VQ {
    uint32_t bar;     /// One of enum SP_DEV_INTF_BAR_TYPE_e
    uint32_t interrupt_vector;
    uint64_t offset;
    uint64_t size;
    SP_DEV_INTF_VQ_SIZE_INFO_s size_info;
} SP_DEV_INTF_SP_VQ_s;

/// \brief Service Processor Device interface register will be uses to public device capability to Host
typedef struct __attribute__((__packed__)) SP_DEV_INTF_REG {
    uint32_t version;                                                    /// Version of this structure
    uint32_t size;                                                       /// Size (in bytes) of this structure
    int32_t status;                                                      /// One of enum SP_DEV_INTF_SP_BOOT_STATUS_e
    uint64_t minion_shires;                                              /// Bitmask of available Minion Shires
    SP_DEV_INTF_SP_VQ_s sp_vq;                                           /// Virtual Queues information
    SP_DEV_INTF_DDR_REGION_s ddr_region[SP_DEV_INTF_DDR_REGION_MAP_NUM]; /// List of DDR regions
} SP_DEV_INTF_REG_s;

#ifdef __cplusplus
}
#endif

#endif
