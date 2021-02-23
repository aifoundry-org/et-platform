/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __MM_DEV_INTF_REG_H__
#define __MM_DEV_INTF_REG_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MM_DEV_INTF_REG_VERSION 1U

#define MM_VQ_COUNT 1U

// MM DEV Interface Register at PC_MM Mailbox + 1K
#define MM_DEV_INTF_BASE_ADDR (R_PU_MBOX_PC_MM_BASEADDR + 0x400UL)

// Only expose Bar based offset,size as address to Host
// Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping

// MM Virtual Queue (BAR=2, Offset=2KB, Size=1KB)
#define MM_VQ_BAR     2
#define MM_VQ_OFFSET  0x800UL
#define MM_VQ_SIZE    0x400UL

// DDR Region 0 USER_KERNEL_SPACE (BAR=0, Offset=4GB, Size=8GB)
#define MM_DEV_INTF_USER_KERNEL_SPACE_BAR    0
#define MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET 0x0100000000UL // TODO: Should be HOST_MANAGED_DRAM_START
#define MM_DEV_INTF_USER_KERNEL_SPACE_SIZE   0x0200000000UL // TODO: Should be (HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START)

/// \brief Master Minion status register used to indicate Boot Status of MM
enum MM_DEV_INTF_MM_BOOT_STATUS_e {
    MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_MB_TIMEOUT = -2,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_ERROR,
    MM_DEV_INTF_MM_BOOT_STATUS_VQ_DESC_NOT_READY = 0,
    MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_VQ_DESC_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_VQ_QUEUE_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_VQ_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_DDR_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_LAUNCHED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_MBOX_INITIALIZED
};

/// \brief List of REGIONS based on Spec as defined here: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map

enum MM_DEV_INTF_DDR_REGION_ATTRIBUTE_e {
    MM_DEV_INTF_DDR_REGION_ATTR_READ_ONLY = 0,
    MM_DEV_INTF_DDR_REGION_ATTR_WRITE_ONLY,
    MM_DEV_INTF_DDR_REGION_ATTR_READ_WRITE
};

enum MM_DEV_INTF_DDR_REGION_MAP_e {
    MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE = 0,
    MM_DEV_INTF_DDR_REGION_MAP_NUM
};

enum MM_DEV_INTF_BAR_TYPE_e {
    MM_DEV_INTF_BAR_0 = 0,
    MM_DEV_INTF_BAR_2 = 2,
    MM_DEV_INTF_BAR_4 = 4
};

typedef struct __attribute__((__packed__)) MM_DEV_INTF_DDR_REGION {
    uint8_t attr;    /// One of enum MM_DEV_INTF_DDR_REGION_ATTRIBUTE_e
    uint8_t bar;     /// One of enum MM_DEV_INTF_BAR_TYPE_e
    uint64_t offset;
    uint64_t devaddr;
    uint64_t size;
} MM_DEV_INTF_DDR_REGION_s;

typedef struct __attribute__((__packed__)) MM_DEV_INTF_VQ_SIZE_INFO {
    uint16_t control_size;      /// Region always placed at the start of MM VQ Bar contains vqueue_info structs
    uint16_t element_count;     /// Number of fixed-size buffers present in a VQ
    uint16_t element_size;      /// Size of each fixed-size buffer
    uint16_t element_alignment; /// Alignement requirement of each fixed-sized buffer
} MM_DEV_INTF_VQ_SIZE_INFO_s;

typedef struct __attribute__((__packed__)) MM_DEV_INTF_MM_VQ {
    uint8_t vq_count;
    uint8_t bar;     /// One of enum MM_DEV_INTF_BAR_TYPE_e
    uint64_t bar_offset;
    uint64_t bar_size;
    MM_DEV_INTF_VQ_SIZE_INFO_s size_info;
    uint16_t interrupt_vector[MM_VQ_COUNT];
} MM_DEV_INTF_MM_VQ_s;

/// \brief Master Minion Device interface register will be uses to public device capability to Host
typedef struct __attribute__((__packed__)) MM_DEV_INTF_REG {
    uint32_t version;
    uint32_t size;
    MM_DEV_INTF_MM_VQ_s mm_vq;
    MM_DEV_INTF_DDR_REGION_s ddr_region[MM_DEV_INTF_DDR_REGION_MAP_NUM];
    int32_t status;                                                      /// One of enum MM_DEV_INTF_MM_BOOT_STATUS_e
} MM_DEV_INTF_REG_s;

// Macro to extract MM DIRs
#define MM_DEV_INTF_GET_BASE   ((volatile MM_DEV_INTF_REG_s *)MM_DEV_INTF_BASE_ADDR)

#ifdef __cplusplus
}
#endif

#endif
