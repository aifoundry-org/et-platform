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

#ifndef __MGMT_DIR_REGS_H__
#define __MGMT_DIR_REGS_H__

#include <stdint.h>
#include "config/mgmt_build_config.h"

/*! \file List of REGIONS based on Spec as defined here:
    https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1233584203/Memory+Map */

/* General Note: Only expose Bar based offset,size as address to Host.
   Host Address -> SOC Address mapping will happen via ET SOC PCIe Device ATU mapping */

/*! \def SP_DEV_INTF_REG_VERSION
    \brief Device Interface Register (DIR) version number.
*/
#define SP_DEV_INTF_REG_VERSION            1U

/*! \func Magic_Marker_Instruction
 *  \brief A magic marker instruction which is used to trigger an external
 *   event. This is a NOP instructions
*/
#define ISSUE_MAGIC_MARKER() __asm__ __volatile__("sltiu zero,zero,0x799\n")

/***************************************/
/* Memory Region accessibility options */
/***************************************/

/*! \def MEM_REGION_PRIVILEDGE_MODE_SET(x)
    \brief Macro that sets the priviledge mode for a memory region
*/
#define MEM_REGION_PRIVILEDGE_MODE_SET(x)  (x & 0x00000001u)

/*! \def MEM_REGION_PRIVILEDGE_MODE_KERNEL
    \brief Macro representing the kernel privileged mode value
*/
#define MEM_REGION_PRIVILEDGE_MODE_KERNEL  0x0

/*! \def MEM_REGION_PRIVILEDGE_MODE_USER
    \brief MAcro representing the user privileged mode value
*/
#define MEM_REGION_PRIVILEDGE_MODE_USER    0x1

/*! \def MEM_REGION_NODE_ACCESSIBLE_SET(x)
    \brief Macro that sets the priviledge mode for a memory region
*/
#define MEM_REGION_NODE_ACCESSIBLE_SET(x)          (((x) << 1) & 0x00000006u)

/*! \def MEM_REGION_NODE_ACCESSIBLE_NONE
    \brief Macro representing the not accessible node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_NONE            0x0

/*! \def MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT
    \brief Macro representing the management node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT      0x1

/*! \def MEM_REGION_NODE_ACCESSIBLE_OPS
    \brief Macro representing the OPS node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_OPS             0x2

/*! \def MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT_OPS
    \brief Macro representing the management and OPS node value
*/
#define MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT_OPS  0x3

/*! \def MEM_REGION_DMA_ALIGNMENT_SET(x)
    \brief Macro that sets the priviledge mode for a memory region
*/
#define MEM_REGION_DMA_ALIGNMENT_SET(x)  (((x) << 3) & 0x00000018u)

/*! \def MEM_REGION_DMA_ALIGNMENT_NONE
    \brief Macro representing the none DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_NONE    0x0

/*! \def MEM_REGION_DMA_ALIGNMENT_8_BIT
    \brief Macro representing the 8-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_8_BIT   0x1

/*! \def MEM_REGION_DMA_ALIGNMENT_32_BIT
    \brief Macro representing the 32-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_32_BIT  0x2

/*! \def MEM_REGION_DMA_ALIGNMENT_64_BIT
    \brief Macro representing the 64-bit DMA alignment value
*/
#define MEM_REGION_DMA_ALIGNMENT_64_BIT  0x3

/***************************/
/* SP DIRs data structures */
/***************************/

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
    SP_DEV_INTF_SP_BOOT_STATUS_PM_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY,
    SP_DEV_INTF_SP_BOOT_STATUS_EVENT_HANDLER_READY,
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
    SP_DEV_INTF_MEM_REGION_TYPE_NUM
};

/*! \enum SP_DEV_INTF_form_factor
    \brief Values representing the available types of
    memory regions supported by the Service Processor
*/
enum SP_DEV_INTF_FORM_FACTOR_e {
	SP_DEV_CONFIG_FORM_FACTOR_PCIE = 1,
	SP_DEV_CONFIG_FORM_FACTOR_M_2
};

/*! \struct SP_DEV_INTF_MEM_REGION_ATTR
    \brief Holds the information of Service Processor interface memory region.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_MEM_REGION_ATTR {
    uint16_t attributes_size;
    uint8_t type;
    uint8_t bar;
    uint32_t access_attr;
    uint64_t bar_offset;
    uint64_t bar_size;
    uint64_t dev_address;
} SP_DEV_INTF_MEM_REGION_ATTR_s;

/*! \struct SP_DEV_INTF_VQ_ATTR
    \brief Holds the information of Service Processor Virtual Queues.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_VQ_ATTR {
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
} SP_DEV_INTF_VQ_ATTR_s;

/*! \struct SP_DEV_INTF_GENERIC_ATTR
    \brief Holds the general information of Service Processor.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_GENERIC_ATTR {
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
    uint8_t reserved[2];
} SP_DEV_INTF_GENERIC_ATTR_s;


/*! \struct SP_DEV_INTF_REG
    \brief Service Processor DIRs which will be used to public device capability to Host.
    \warning Must be 64-bit aligned.
*/
typedef struct __attribute__((__packed__)) SP_DEV_INTF_REG {
    SP_DEV_INTF_GENERIC_ATTR_s generic_attr;
    SP_DEV_INTF_VQ_ATTR_s vq_attr;
    /* Memory regions can be extended by the FW. The host will read it as
    flexible array. Hence, always place this array at the end of structure.
    The count of this array is dictated by num_mem_regions */
    SP_DEV_INTF_MEM_REGION_ATTR_s mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_NUM];
} SP_DEV_INTF_REG_s;

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that SP SQs are within limits */
static_assert(sizeof(SP_DEV_INTF_REG_s) <= SP_DEV_INTF_SIZE,
    "DIRs size is not within allowed limits.");

#endif /* __ASSEMBLER__ */

/***********************/
/* Function prototypes */
/***********************/

/*! \fn void DIR_Init(void)
    \brief Initialize Device Interface Registers
    \param None
*/
void DIR_Init(void);

/*! \fn void DIR_Set_Service_Processor_Status(uint8_t status)
    \brief Set Service Processor ready status
    \param status Value of DIRs status field
*/
void DIR_Set_Service_Processor_Status(int16_t status);

/*! \fn void DIR_Cache_Size_Init(void)
    \brief Initialize Device Cache Size Registers
    \param None
*/
void DIR_Cache_Size_Init(void);

/*! \fn void DIR_Generic_Attributes_Init(void)
    \brief Populate the device generic attributes
    \param None
*/
void DIR_Generic_Attributes_Init(void);

#endif /* MGMT_DIR_REGS_H */
