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

#pragma once

#include <cstdint>

/*! \enum MM_DEV_INTF_MM_BOOT_STATUS_e
    \brief Values representing Master Minion Boot status.
*/
enum MM_DEV_INTF_MM_BOOT_STATUS_e {
    MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_ERROR = -1,
    MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_NOT_READY = 0,
    MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_INTERRUPT_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_CM_INTERFACE_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_CM_WORKERS_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_WORKERS_INITIALIZED,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_HOST_VQ_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_INTERFACE_READY,
    MM_DEV_INTF_MM_BOOT_STATUS_MM_READY
};

/*! \enum MM_DEV_INTF_MEM_REGION_TYPE_e
    \brief Values representing the available types of
    memory regions supported by the Master Minion.
*/
enum MM_DEV_INTF_MEM_REGION_TYPE_e {
    MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER = 0 ,
    MM_DEV_INTF_MEM_REGION_TYPE_OPS_MMFW_TRACE,
    MM_DEV_INTF_MEM_REGION_TYPE_OPS_CMFW_TRACE,
    MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED,
    MM_DEV_INTF_MEM_REGION_TYPE_NUM
};

/*! \struct MM_DEV_INTF_MEM_REGION_ATTR
    \brief Holds the information of Master Minion interface memory region.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) MM_DEV_INTF_MEM_REGION_ATTR {
    uint16_t attributes_size;
    uint8_t type;
    uint8_t bar;
    uint32_t access_attr;
    uint64_t bar_offset;
    uint64_t bar_size;
    uint64_t dev_address;
} MM_DEV_INTF_MEM_REGION_ATTR_s;

/*! \struct MM_DEV_INTF_VQ_ATTR
    \brief Holds the information of Master Minion Virtual Queues.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) MM_DEV_INTF_VQ_ATTR {
    uint16_t attributes_size;
    uint8_t int_trg_size;
    uint8_t int_id;
    uint32_t int_trg_offset;
    uint32_t sq_offset;
    uint16_t sq_count;
    uint16_t per_sq_size;
    uint32_t cq_offset;
    uint16_t cq_count;
    uint16_t per_cq_size;
    uint32_t sq_hp_offset;
    uint16_t sq_hp_count;
    uint16_t per_sq_hp_size;
} MM_DEV_INTF_VQ_ATTR_s;

/*! \struct MM_DEV_INTF_GENERIC_ATTR
    \brief Holds the general information of Master Minion.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) MM_DEV_INTF_GENERIC_ATTR {
    uint16_t attributes_size;
    uint16_t version;
    uint16_t total_size;
    uint16_t num_mem_regions;
    int16_t status;
    uint32_t crc32;
    uint8_t reserved[2];
} MM_DEV_INTF_GENERIC_ATTR_s;

/*! \struct MM_DEV_INTF_REG
    \brief Master Minion DIR which will be used to public device capability to Host.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) _MM_DEV_INTF_REG {
    MM_DEV_INTF_GENERIC_ATTR_s generic_attr;
    MM_DEV_INTF_VQ_ATTR_s vq_attr;
    /* Memory regions can be extended by the FW. The host will read it as
    flexible array. Hence, always place this array at the end of structure.
    The count of this array is dictated by num_mem_regions */
    MM_DEV_INTF_MEM_REGION_ATTR_s mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_NUM];
} MM_DEV_INTF_REG;