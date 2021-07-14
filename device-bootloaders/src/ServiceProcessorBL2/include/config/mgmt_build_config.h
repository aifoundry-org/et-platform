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
*       Header/Interface to access all Service Processor global build
*       time configuration parameters
*
***********************************************************************/

#ifndef __MGMT_BUILD_CONFIG_H__
#define __MGMT_BUILD_CONFIG_H__

#include <common_defs.h>
#include "hwinc/hal_device.h"
#include "layout.h"

/************************************************/
/*      - PC_SP secure mailbox layout (4K) -    */
/*      user         base-offset      size      */
/*     SP DIRS          0x0           0x200     */
/*     SP SQ            0x200         0x700     */
/*     SP CQ            0x900         0x700     */
/************************************************/

/**********************/
/* DIR Configuration  */
/**********************/

/*! \def SP_DEV_INTF_BASE_ADDR
    \brief Macro that provides the base address of the DIRs
    SP DEV Interface Register at PC_SP Mailbox
*/
#define SP_DEV_INTF_BASE_ADDR      R_PU_MBOX_PC_SP_BASEADDR

/*! \def SP_DEV_INTF_SIZE
    \brief Macro that provides the total allowed size of the SP DIRs
    at PC_SP Mailbox
*/
#define SP_DEV_INTF_SIZE           0x100

/*! \def SP_DEV_INTF_SIZE
    \brief This is the region dedicated to device's MBOX triggers
*/
#define SP_INTERRUPT_TRG_REGION_BASE_ADDR  R_PU_TRG_PCIE_BASEADDR

/*! \def SP_DEV_INTF_SIZE
    \brief Total size of the region dedicated to device's MBOX triggers
*/
#define SP_INTERRUPT_TRG_REGION_SIZE       R_PU_TRG_PCIE_SIZE

/*! \def SP_INTERRUPT_TRG_OFFSET
    \brief A macro that provides the offset for triggering the interrupt for SP
    in Interrupt Trigger Region.
*/
#define SP_INTERRUPT_TRG_OFFSET     0U

/*! \def SP_INTERRUPT_TRG_SIZE
    \brief A macro that provides the size of the field for SP
    in Interrupt Trigger Region.
*/
#define SP_INTERRUPT_TRG_SIZE       4U

/*! \def SP_INTERRUPT_TRG_ID
    \brief A macro that provides the ID/value to write for SP
    in Interrupt Trigger Region.
*/
#define SP_INTERRUPT_TRG_ID         1U

/* DDR Region 0 DEV_MANAGEMENT_SCRATCH (BAR=0, Offset=0, Size=4MB) */

/*! \def SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_BAR
    \brief A macro that provides the PCI BAR region using which
    the Service Processor scratch space can be accessed
*/
#define SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_BAR    0

/*! \def SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_OFFSET
    \brief A macro that provides the offset of Service Processor scratch space
    on PCI BAR
*/
#define SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_OFFSET 0x0UL

/*! \def SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_SIZE
    \brief A macro that provides the total size of Service Processor scratch space
    on PCI BAR.
*/
#define SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_SIZE   SP_DM_SCRATCH_REGION_SIZE

/* DDR Region 1 TRACE_BUFFER (BAR=0, Offset=0x400000, Size=4KB) */

/*! \def SP_DEV_INTF_TRACE_BUFFER_BAR
    \brief A macro that provides the PCI BAR region using which
    the Service Processor trace buffer space can be accessed
*/
#define SP_DEV_INTF_TRACE_BUFFER_BAR               0

/*! \def SP_DEV_INTF_TRACE_BUFFER_OFFSET
    \brief A macro that provides the offset of Service Processor
    trace buffer space on PCI BAR
*/
#define SP_DEV_INTF_TRACE_BUFFER_OFFSET            0x400000UL

/*! \def SP_DEV_INTF_TRACE_BUFFER_SIZE
    \brief A macro that provides the total size of Service Processor
    trace buffer space on PCI BAR.
*/
#define SP_DEV_INTF_TRACE_BUFFER_SIZE              SP_TRACE_BUFFER_SIZE

/* Interrupt Trigger Region (BAR=2, Offset=0x2000, Size=8KB) */

/*! \def SP_DEV_INTF_INTERRUPT_TRG_REGION_BAR
    \brief A macro that provides the PCI BAR region using which
    the interrupt trigger region space can be accessed
*/
#define SP_DEV_INTF_INTERRUPT_TRG_REGION_BAR        2

/*! \def SP_DEV_INTF_INTERRUPT_TRG_REGION_OFFSET
    \brief A macro that provides the offset of Service Processor
    interrupt trigger region space on PCI BAR
*/
#define SP_DEV_INTF_INTERRUPT_TRG_REGION_OFFSET     0x2000UL

/*! \def SP_DEV_INTF_INTERRUPT_TRG_REGION_SIZE
    \brief A macro that provides the total size of Service Processor
    interrupt trigger region space on PCI BAR.
*/
#define SP_DEV_INTF_INTERRUPT_TRG_REGION_SIZE       SP_INTERRUPT_TRG_REGION_SIZE

/*****************************************************/
/* Definitions to locate and manage Host to SP SQ/CQ */
/*****************************************************/

/*! \def SP_PC_MAILBOX_BAR_OFFSET
    \brief This is the offset from the BAR 2 mapping where the location
    of the PC_SP_Mailbox is placed
    R_PU_MBOX_PC_SP   BAR2 + 0x1000   4k     Mailbox shared memory
*/
#define SP_PC_MAILBOX_BAR_OFFSET  0x1000UL

/*! \def SP_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Service Processor virtual queues can be accessed
*/
#define SP_VQ_BAR          2

/*! \def SP_VQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using which
    the Service Processor virtual queues can be accessed
*/
#define SP_VQ_OFFSET       0x100UL

/*! \def SP_SQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    SP_VQ_BAR using which the Service Processor submission queues can be accessed
*/
#define SP_SQ_OFFSET       0x0UL

/*! \def SP_SQS_BASE_ADDR
    \brief A macro that provides the Service Processor's 32 bit base address
    for submission queues relative to the PC_SP_Mailbox
*/
#define SP_SQ_BASE_ADDRESS (R_PU_MBOX_PC_SP_BASEADDR + SP_VQ_OFFSET + SP_SQ_OFFSET)

/*! \def SP_VQ_BAR_SIZE
    \brief A macro that provides the total size for SP VQs (SQs + CQs)
    on PCI BAR.
*/
#define SP_VQ_BAR_SIZE     0xF00UL

/*! \def SP_SQ_COUNT
    \brief A macro that provides the Service Processor submission queue
    count
*/
#define SP_SQ_COUNT         1

/*! \def SP_MAX_SUPPORTED_SQS
    \brief Maximum supported submission queues by Service Processor
*/
#define SP_SQ_MAX_SUPPORTED 1

/*! \def SP_SQ_SIZE
    \brief A macro that provides size of the Service Processor
    submission queue. All submision queues will be of same size.
*/
#define SP_SQ_SIZE          (SP_VQ_BAR_SIZE / 2)

/*! \def SP_SQ_MEM_TYPE
    \brief A macro that provides the memory type for SP submission queues
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define SP_SQ_MEM_TYPE      UNCACHED

/*! \def SP_CQS_BASE_ADDRESS
    \brief A macro that provides the Service Processor's base address
    for completion queues
*/
#define SP_CQ_BASE_ADDRESS (SP_SQ_BASE_ADDRESS + (SP_SQ_COUNT * SP_SQ_SIZE))

/*! \def SP_CQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    SP_VQ_BAR using which the Service Processor completion queues can be accessed
*/
#define SP_CQ_OFFSET       (SP_SQ_OFFSET + (SP_SQ_COUNT * SP_SQ_SIZE))

/*! \def SP_CQ_SIZE
    \brief A macro that provides size of the Service Processor
    completion queue.
*/
#define SP_CQ_SIZE          (SP_VQ_BAR_SIZE / 2)

/*! \def SP_CQ_COUNT
    \brief A macro that provides the Service Processor completion queue
    count
*/
#define SP_CQ_COUNT         1

/*! \def SP_MAX_SUPPORTED_CQS
    \brief Maximum supported completion queues by Service Processor
*/
#define SP_CQ_MAX_SUPPORTED 1

/*! \def SP_CQ_NOTIFY_VECTOR
    \brief A macro that provides the starting PCIe interrupt vector for
    CQ notifications
*/
#define SP_CQ_NOTIFY_VECTOR 0

/*! \def SP_CQ_MEM_TYPE
    \brief A macro that provides the memory type for SP completion queue
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define SP_CQ_MEM_TYPE      UNCACHED

/*! \def SP_CMD_MAX_SIZE
    \brief A macro that provides the maximum command size on the host <> mm
    communication interface in bytes
*/
/* TODO: Fine tune this value according to the final device-ops-api spec */
#define SP_CMD_MAX_SIZE      64U

/*! \def SP_CMD_MAX_TIMEOUT
    \brief A macro that provides the timeout for SP to MM commands. SP will
           wait for response from MM for this much time at max.
           NOTE: This value is in terms of OS ticks i.e. TickType_t.
*/
#define SP2MM_CMD_TIMEOUT      200

/*! \def PMIC_TEMP_LOWER_SET_LIMIT
    \brief A macro that provides pmic minimum temperature threshold value
*/
#define PMIC_TEMP_LOWER_SET_LIMIT      55

/*! \def PMIC_TEMP_UPPER_SET_LIMIT
    \brief A macro that provides pmic maximum temperature threshold value
*/
#define PMIC_TEMP_UPPER_SET_LIMIT      85

/*! \def PMIC_TEMP_THRESHOLD_HI
    \brief A macro that provides pmic temperature threshold value
*/
#define PMIC_TEMP_THRESHOLD_HI      80

/*! \def PMIC_TEMP_THRESHOLD_LO
    \brief A macro that provides early indication temperature threshold value
*/
#define PMIC_TEMP_THRESHOLD_LO      70

/*! \def DELTA_TEMP_UPDATE_PERIOD 
    \brief A macro that provides dTj/dt - time(uS) for Junction temperature to be updated
           This value needs to be characterized in Silicon
*/
#define DELTA_TEMP_UPDATE_PERIOD   1000

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that DIRs and SP SQs base address don't overlap */
static_assert((SP_DEV_INTF_BASE_ADDR + SP_DEV_INTF_SIZE - 1) < SP_SQ_BASE_ADDRESS,
    "DIRs and SQs base address overlapping.");

/* Ensure that DIRs and SP SQs base address don't overlap */
static_assert((SP_SQ_BASE_ADDRESS + (SP_SQ_COUNT * SP_SQ_SIZE) - 1) < SP_CQ_BASE_ADDRESS,
    "SQs and CQs base address overlapping.");

/* Ensure that SP SQs and CQs size is within limits */
static_assert(((SP_SQ_COUNT * SP_SQ_SIZE) + (SP_CQ_COUNT * SP_CQ_SIZE)) <= SP_VQ_BAR_SIZE,
    "SP VQs size not within limits.");

#endif /* __ASSEMBLER__ */

#endif /* __MGMT_BUILD_CONFIG_H__ */
