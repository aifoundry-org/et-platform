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
/*! \file mm_config.h
    \brief This is the only build configuration file for the MasterMinion.

    Allows for configuration of all key attributes of MM runtime incl;
    1. Location and configuration of Virtual Queues, incl,
        Host Interface Virtual Queues
            Submission Queues
            Completion Queue
        SP Interface Virtual Queues
            Submission Queue
            Completion Queue
    2. Location and configuration of Device Interface Registers
    3. Configuration of Master Minion Workers such as;
        kernel worker
        DMA worker
        Submission Queue Workers
        Completion Queue Workers
*/
/***********************************************************************/

#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

#include <common_defs.h>
#include "layout.h"
#include "hal_device.h"

/************************************************/
/*      - PC_MM secure mailbox layout (4K) -    */
/*      user         base-offset      size      */
/*     MM DIRS          0x0           0x200     */
/*     MM SQs           0x200         0x800     */
/*     MM CQs           0xA00         0x600     */
/************************************************/

/**********************/
/* DIR Configuration  */
/**********************/

/*! \def MM_DEV_INTF_BASE_ADDR
    \brief Macro that provides the base address of the DIRs
    MM DEV Interface Register at PC_MM Mailbox
*/
#define MM_DEV_INTF_BASE_ADDR      R_PU_MBOX_PC_MM_BASEADDR

/*! \def MM_DEV_INTF_SIZE
    \brief Macro that provides the total allowed size of the MM DIRs
    at PC_MM Mailbox
*/
#define MM_DEV_INTF_SIZE           0x200

/*! \def MM_INTERRUPT_TRG_OFFSET
    \brief A macro that provides the offset for triggering the interrupt for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_OFFSET     4U

/*! \def MM_INTERRUPT_TRG_SIZE
    \brief A macro that provides the size of the field for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_SIZE       4U

/*! \def MM_INTERRUPT_TRG_ID
    \brief A macro that provides the ID/value to write for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_ID         1U

/* DDR Region 0 MMFW_TRACE_REGION (BAR=0, Offset=0x401000, Size=512K) */

/*! \def MM_DEV_INTF_MMFW_TRACE_REGION_BAR
    \brief A macro that provides the PCI BAR region using which
    the MMFW trace region can be accessed
*/
#define MM_DEV_INTF_MMFW_TRACE_REGION_BAR    0

/*! \def MM_DEV_INTF_MMFW_TRACE_REGION_OFFSET
    \brief A macro that provides the offset of MMFW trace region
    on PCI BAR
*/
#define MM_DEV_INTF_MMFW_TRACE_REGION_OFFSET 0x401000

/*! \def MM_DEV_INTF_MMFW_TRACE_REGION_SIZE
    \brief A macro that provides the total size of MMFW trace region
    on PCI BAR.
*/
#define MM_DEV_INTF_MMFW_TRACE_REGION_SIZE   MM_TRACE_BUFFER_SIZE

/* DDR Region 1 CMFW_TRACE_REGION (BAR=0, Offset=0x481000, Size=512K) */

/*! \def MM_DEV_INTF_CMFW_TRACE_REGION_BAR
    \brief A macro that provides the PCI BAR region using which
    the CMFW trace region can be accessed
*/
#define MM_DEV_INTF_CMFW_TRACE_REGION_BAR    0

/*! \def MM_DEV_INTF_CMFW_TRACE_REGION_OFFSET
    \brief A macro that provides the offset of CMFW trace region
    on PCI BAR
*/
#define MM_DEV_INTF_CMFW_TRACE_REGION_OFFSET 0x481000

/*! \def MM_DEV_INTF_CMFW_TRACE_REGION_SIZE
    \brief A macro that provides the total size of CMFW trace region
    on PCI BAR.
*/
#define MM_DEV_INTF_CMFW_TRACE_REGION_SIZE   CM_TRACE_BUFFER_SIZE

/* DDR Region 2 USER_KERNEL_SPACE (BAR=0, Offset=0x600000, Size~=12GB) */

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion DDR User Kernel space can be accessed
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_BAR    0

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET
    \brief A macro that provides the offset of User Kernel space
    on PCI BAR
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET 0x600000

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_SIZE
    \brief A macro that provides the total size of User Kernel space
    on PCI BAR.
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_SIZE   HOST_MANAGED_DRAM_SIZE

/******************************************************/
/* Definitions to locate and manage Host to MM SQs/CQ */
/******************************************************/

/*! \def MM_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion virtual queues can be accessed
*/
#define MM_VQ_BAR           2

/*! \def MM_VQ_SIZE
    \brief A macro that provides the total size for MM VQs (SQs + CQs)
    on PCI BAR.
*/
#define MM_VQ_SIZE          0xE00UL

/*! \def MM_VQ_OFFSET
    \brief A macro that provides the offset for MM VQs (SQs + CQs)
    on PCI BAR.
*/
#define MM_VQ_OFFSET        0x200UL

/*! \def MM_SQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    MM_VQ_BAR using which the Master Minion submission queues can be accessed
*/
#define MM_SQ_OFFSET        0x0UL

/*! \def MM_SQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's 32 bit base address
    for submission queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_SQS_BASE_ADDRESS (R_PU_MBOX_PC_MM_BASEADDR + MM_VQ_OFFSET + MM_SQ_OFFSET)

/*! \def MM_SQ_COUNT
    \brief A macro that provides the Master Minion submission queue
    count
*/
#define MM_SQ_COUNT         2

/*! \def MM_SQ_MAX_SUPPORTED
    \brief Maximum supported submission queues by Master Minion
*/
#define MM_SQ_MAX_SUPPORTED 4

/*! \def MM_SQ_SIZE
    \brief A macro that provides size of the Master Minion
    submission queue. All submision queues will be of same size.
*/
#define MM_SQ_SIZE          (MM_VQ_SIZE - (MM_CQ_SIZE * MM_CQ_COUNT)) / MM_SQ_COUNT

/*! \def MM_SQ_HP_INDEX
    \brief A macro that provides the Master Minion high priority
    submission queue index
*/
#define MM_SQ_HP_INDEX      0

/*! \def MM_SQ_MEM_TYPE
    \brief A macro that provides the memory type for MM submission queues
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_SQ_MEM_TYPE      UNCACHED

/*! \def MM_CQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's base address
    for completion queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_CQS_BASE_ADDRESS (MM_SQS_BASE_ADDRESS + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_CQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    MM_VQ_BAR using which the Master Minion completion queues can be accessed
*/
#define MM_CQ_OFFSET        (MM_SQ_OFFSET + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_CQ_SIZE
    \brief A macro that provides size of the Master Minion
    completion queue.
*/
#define MM_CQ_SIZE          0x600UL

/*! \def MM_CQ_COUNT
    \brief A macro that provides the Master Minion completion queue
    count
*/
#define MM_CQ_COUNT         1

/*! \def MM_CQ_MAX_SUPPORTED
    \brief Maximum supported completion queues by Master Minion
*/
#define MM_CQ_MAX_SUPPORTED 1

/*! \def MM_CQ_NOTIFY_VECTOR
    \brief A macro that provides the starting PCIe interrupt vector for
    CQ notifications
*/
#define MM_CQ_NOTIFY_VECTOR 0

/*! \def MM_CQ_MEM_TYPE
    \brief A macro that provides the memory type for MM completion queue
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_CQ_MEM_TYPE      UNCACHED

/*! \def MM_CMD_MAX_SIZE
    \brief A macro that provides the maximum command size on the host <> mm
    communication interface in bytes
*/
/* TODO: Fine tune this value according to the final device-ops-api spec */
#define MM_CMD_MAX_SIZE      264U /* = sizeof(struct dma_write/read_node) * Max Nodes + Command header
                                     = 32 * 8 + 8
                                     = 264 bytes */

/*******************************************************************/
/* Definitions for MM Shire and its Harts                          */
/*******************************************************************/
/*! \def MM_SHIRE_MASK
    \brief Master Minion Shire mask
*/
#define MM_SHIRE_MASK            (1UL << 32)

/*! \def MM_HART_MASK
    \brief Master Minion Hart mask
*/
#define MM_HART_MASK             (0xFFFFFFFFUL)

/*! \def MM_HARTS_COUNT
    \brief Number of Harts running MMFW.
*/
#define MM_HART_COUNT           32U

/*******************************************************************/
/* Definitions for MM dispatcher, and workers - SQW, KW, DMAW, CQW */
/*******************************************************************/

/*! \def WORKER_HARTS_EVEN
    \brief Macro to switch between even or odd Harts for MM Workers.
    If this macro is defined, MM will use even Harts for its workers and vice versa.
*/
#define WORKER_HARTS_EVEN

#ifdef WORKER_HARTS_EVEN
/*! \def WORKER_HART_FACTOR
    \brief Divider for the caclulations performed on even Harts
*/
#define WORKER_HART_FACTOR 2U
#else
/*! \def WORKER_HART_FACTOR
    \brief Divider for the caclulations performed on odd Harts
*/
#define WORKER_HART_FACTOR 1U
#endif

/*! \def MM_BASE_ID
    \brief Base HART ID for the Master Minion
*/
#define MM_BASE_ID               2048U

/*! \def MM_MAX_PARALLEL_KERNELS
    \brief Maximum number of kerenls in parallel supported by MM runtime
*/
#define MM_MAX_PARALLEL_KERNELS  1

/*! \def DISPATCHER_BASE_HART_ID
    \brief Base HART ID for the Dispatcher
*/
#define DISPATCHER_BASE_HART_ID  MM_BASE_ID

/*! \def DISPATCHER_NUM
    \brief Number of Dispatchers
    \warning DO NOT MODIFY!
*/
#define DISPATCHER_NUM           1

/*! \def SQW_BASE_HART_ID
    \brief Base HART ID for the Submission Queue Worker
*/
#define SQW_BASE_HART_ID         2050U

/*! \def SQW_NUM
    \brief Number of Submission Queue Workers
*/
#define SQW_NUM                  MM_SQ_COUNT

/*! \def KW_BASE_HART_ID
    \brief Base HART ID for the Kernel Worker
*/
#define KW_BASE_HART_ID          2054U

/*! \def KW_MS_BASE_HART
    \brief Base HART number in Master Shire for the kernel workers
*/
#define KW_MS_BASE_HART          (KW_BASE_HART_ID - MM_BASE_ID)

/*! \def KW_NUM
    \brief Number of Kernel Workers
*/
#define KW_NUM                   MM_MAX_PARALLEL_KERNELS

/*! \def DMAW_BASE_HART_ID
    \brief Base HART ID for the DMA Worker
*/
#define DMAW_BASE_HART_ID        2058U

/*! \def DMAW_NUM
    \brief Number of DMA Workers
*/
#define DMAW_NUM                  2

/***************************************************/
/* Definitions to locate and manage MM to SP SQ/CQ */
/***************************************************/
/* TODO: This data is same as data defined/used by sp_mm_iface in SP BL2 runtime,
move this defined to a abstraction common to SP and MM runtimes*/

#define     SP2MM_SQ_BASE        R_PU_MBOX_MM_SP_BASEADDR
#define     SP2MM_SQ_COUNT       10U
#define     SP2MM_SQ_SIZE        256 /* 1 KB */
#define     SP2MM_SQ_MEM_TYPE    UNCACHED

#define     SP2MM_CQ_BASE        (SP2MM_SQ_BASE + SP2MM_SQ_SIZE)
#define     SP2MM_CQ_SIZE        256U
#define     SP2MM_CQ_COUNT       10U
#define     SP2MM_CQ_MEM_TYPE    UNCACHED

#define     MM2SP_SQ_BASE        (SP2MM_CQ_BASE + SP2MM_CQ_SIZE)
#define     MM2SP_SQ_SIZE        256U
#define     MM2SP_SQ_COUNT       10U
#define     MM2SP_SQ_MEM_TYPE    UNCACHED

#define     MM2SP_CQ_BASE        (MM2SP_SQ_BASE + MM2SP_SQ_SIZE)
#define     MM2SP_CQ_SIZE        256U
#define     MM2SP_CQ_COUNT       10U
#define     MM2SP_CQ_MEM_TYPE    UNCACHED

#define     MM_SP_CMD_SIZE       64

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that DIRs and MM SQs base address don't overlap */
static_assert((MM_DEV_INTF_BASE_ADDR + MM_DEV_INTF_SIZE - 1) < MM_SQS_BASE_ADDRESS,
    "DIRs and SQs base address overlapping.");

/* Ensure that DIRs and MM SQs base address don't overlap */
static_assert((MM_SQS_BASE_ADDRESS + (MM_SQ_COUNT * MM_SQ_SIZE) - 1) < MM_CQS_BASE_ADDRESS,
    "SQs and CQs base address overlapping.");

/* Ensure that MM SQs are within limits */
static_assert(MM_SQ_COUNT <= MM_SQ_MAX_SUPPORTED,
    "Number of MM Submission Queues not within limits.");

/* Ensure that MM SQs and CQs size is within limits */
static_assert(((MM_SQ_COUNT * MM_SQ_SIZE) + (MM_CQ_COUNT * MM_CQ_SIZE)) <= MM_VQ_SIZE,
    "MM VQs size not within limits.");

#endif /* __ASSEMBLER__ */

#endif /* __MM_CONFIG_H__ */
