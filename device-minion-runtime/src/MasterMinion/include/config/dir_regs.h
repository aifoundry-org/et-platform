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
************************************************************************/
/*! \file dir_regs.h
    \brief A C header that defines the Device Interface Registers (DIRs)
    related structs and bindings.
*/
/***********************************************************************/

#ifndef __DIR_REGS_H__
#define __DIR_REGS_H__

#include <stdint.h>
#include "config/mm_config.h"
#include "system/layout.h"

/* List of REGIONS based on Spec as defined here:
 * https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map */

/* General Note: Only expose Bar based offset,size as address to Host.
   Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping */

/*! \def MM_DEV_INTF_REG_VERSION
    \brief Device Interface Register (DIR) version number.
*/
#define MM_DEV_INTF_REG_VERSION 1U

/***************************************/
/* Memory Region accessibility options */
/***************************************/

/*! \def MEM_REGION_IOACCESS_SET(x)
    \brief Macro that sets the IO access for a memory region
*/
#define MEM_REGION_IOACCESS_SET(x) (x & 0x00000001u)

/*! \def MEM_REGION_IOACCESS_DISABLED
    \brief Macro representing IO access disabled for a memory region
*/
#define MEM_REGION_IOACCESS_DISABLED 0x0

/*! \def MEM_REGION_IOACCESS_ENABLED
    \brief Macro representing IO access enabled for a memory region
*/
#define MEM_REGION_IOACCESS_ENABLED 0x1

/*! \def MEM_REGION_NODE_ACCESSIBLE_SET(x)
    \brief Macro that sets the node accessibility for a memory region
*/
#define MEM_REGION_NODE_ACCESSIBLE_SET(x) (((x) << 1) & 0x00000006u)

/*! \def MEM_REGION_NODE_ACCESSIBLE_NONE
    \brief Macro representing the not accessible node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_NONE 0x0

/*! \def MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT
    \brief Macro representing the management node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT 0x1

/*! \def MEM_REGION_NODE_ACCESSIBLE_OPS
    \brief Macro representing the OPS node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_OPS 0x2

/*! \def MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT_OPS
    \brief Macro representing the management and OPS node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT_OPS 0x3

/*! \def MEM_REGION_DMA_ALIGNMENT_SET(x)
    \brief Macro that sets the DMA alignment for a memory region
*/
#define MEM_REGION_DMA_ALIGNMENT_SET(x) (((x) << 3) & 0x00000018u)

/*! \def MEM_REGION_DMA_ALIGNMENT_NONE
    \brief Macro representing the none DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_NONE 0x0

/*! \def MEM_REGION_DMA_ALIGNMENT_8_BIT
    \brief Macro representing the 8-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_8_BIT 0x1

/*! \def MEM_REGION_DMA_ALIGNMENT_32_BIT
    \brief Macro representing the 32-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_32_BIT 0x2

/*! \def MEM_REGION_DMA_ALIGNMENT_64_BIT
    \brief Macro representing the 64-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_64_BIT 0x3

/*! \def MEM_REGION_DMA_ELEMENT_COUNT_SET(x)
    \brief Macro that sets the DMA element count
*/
#define MEM_REGION_DMA_ELEMENT_COUNT_SET(x) (((x)&0xF) << 5)

/*! \def MEM_REGION_DMA_ELEMENT_COUNT_SET(x)
    \brief Macro that sets the DMA element size
*/
#define MEM_REGION_DMA_ELEMENT_SIZE_SET(x) (((x)&0xFF) << 9)

/*! \def MEM_REGION_DMA_ELEMENT_SIZE_STEP
    \brief Macro which provides the step size of DMA element size in MBs
*/
#define MEM_REGION_DMA_ELEMENT_SIZE_STEP 32

/*! \def MEM_REGION_DMA_ELEMENT_SIZE
    \brief Macro for DMA element size
*/
#define MEM_REGION_DMA_ELEMENT_SIZE 4

/*! \def MEM_REGION_DMA_ELEMENT_COUNT
    \brief Macro for element count in DMA list
*/
#define MEM_REGION_DMA_ELEMENT_COUNT DMA_MAX_ENTRIES_PER_LL

/***************************/
/* MM DIRs data structures */
/***************************/

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
    MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER = 0,
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
typedef struct __attribute__((__packed__)) MM_DEV_INTF_REG {
    MM_DEV_INTF_GENERIC_ATTR_s generic_attr;
    MM_DEV_INTF_VQ_ATTR_s vq_attr;
    /* Memory regions can be extended by the FW. The host will read it as
    flexible array. Hence, always place this array at the end of structure.
    The count of this array is dictated by num_mem_regions */
    MM_DEV_INTF_MEM_REGION_ATTR_s mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_NUM];
} MM_DEV_INTF_REG_s;

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that MM DIRs are within limits */
static_assert(
    sizeof(MM_DEV_INTF_REG_s) <= MM_DEV_INTF_SIZE, "MM DIRs size is not within allowed limits.");

#endif /* __ASSEMBLER__ */

/***********************/
/* Function prototypes */
/***********************/

/*! \fn void DIR_Init(void)
    \brief Initialize Device Interface Registers
    \return none
*/
void DIR_Init(void);

/*! \fn void DIR_Set_Master_Minion_Status(uint8_t status)
    \brief Set Master Minion ready status
    \return none
*/
void DIR_Set_Master_Minion_Status(int16_t status);

/*! \fn void DIR_Update_Interface_Ready(void)
    \brief Calculate CRC for DIRs after update and set status to interface ready
    \return none
*/
void DIR_Update_Interface_Ready(void);

/*! \fn void DIR_Update_Mem_Region_Size(MM_DEV_INTF_MEM_REGION_TYPE_e region_type, uint64_t ddr_size)
    \brief Set Mem region bar size based on DDR size retrieved from SP
    \param region_type Type of region to update
    \param region_size size of region
    \return none
*/
void DIR_Update_Mem_Region_Size(int16_t region_type, uint64_t region_size);

#endif /* DIR_REGS_H */
