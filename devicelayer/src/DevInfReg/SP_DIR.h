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

#pragma once

#include <cstdint>

extern "C" {
/*! \enum SP_DEV_INTF_SP_BOOT_STATUS_e
    \brief Values representing Service Processor Boot status
*/
enum SP_DEV_INTF_SP_BOOT_STATUS_e {
    SP_DEV_INTF_SP_BOOT_STATUS_BOOT_ERROR = -1,
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

/*! \enum SP_DEV_INTF_MEM_REGION_TYPE_e
    \brief Values representing the available types of
    memory regions supported by the Service Processor
*/
enum SP_DEV_INTF_MEM_REGION_TYPE_e {
    SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER = 0 ,
    SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER,
    SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH,
    SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE,
    SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE,
    SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE,
    SP_DEV_INTF_MEM_REGION_TYPE_NUM
};

/*! \struct SP_DEV_INTF_MEM_REGION_ATTR
    \brief Holds the information of Service Processor interface memory region.
    \warning Must be 64-bit aligned.
*/
using SP_DEV_INTF_MEM_REGION_ATTR_s = struct __attribute__((__packed__)) SP_DEV_INTF_MEM_REGION_ATTR {
    uint16_t attributes_size;
    uint8_t type;
    uint8_t bar;
    uint32_t access_attr;
    uint64_t bar_offset;
    uint64_t bar_size;
    uint64_t dev_address;
};

/*! \struct SP_DEV_INTF_VQ_ATTR
    \brief Holds the information of Service Processor Virtual Queues.
    \warning Must be 64-bit aligned.
*/
using SP_DEV_INTF_VQ_ATTR_s = struct __attribute__((__packed__)) SP_DEV_INTF_VQ_ATTR {
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
};

/*! \struct SP_DEV_INTF_GENERIC_ATTR
    \brief Holds the general information of Service Processor.
    \warning Must be 64-bit aligned.
*/
using SP_DEV_INTF_GENERIC_ATTR_s = struct __attribute__((__packed__)) SP_DEV_INTF_GENERIC_ATTR {
    uint16_t attributes_size;
    uint16_t version;
    uint16_t total_size;
    uint16_t num_mem_regions;
    uint64_t minion_shires_mask;
    uint32_t minion_boot_freq;
    uint32_t crc32;
    int16_t  status;
    uint16_t form_factor;
    uint16_t device_tdp;
    uint16_t l3_size;
    uint16_t l2_size;
    uint16_t scp_size;
    uint16_t cache_line_size;
    uint16_t bar0_size;
    uint16_t bar2_size;
    uint8_t  reserved[6];
};

/*! \struct SP_DEV_INTF_REG
    \brief Service Processor DIRs which will be used to public device capability to Host.
    \warning Must be 64-bit aligned.
*/
using SP_DEV_INTF_REG = struct __attribute__((__packed__)) SP_DEV_INTF_REG {
    SP_DEV_INTF_GENERIC_ATTR_s generic_attr;
    SP_DEV_INTF_VQ_ATTR_s vq_attr;
    /* Memory regions can be extended by the FW. The host will read it as
    flexible array. Hence, always place this array at the end of structure.
    The count of this array is dictated by num_mem_regions */
    SP_DEV_INTF_MEM_REGION_ATTR_s mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_NUM];
};

}
