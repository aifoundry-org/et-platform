/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file rt_errors.h
    \brief A C header that defines error codes for device runtime firmware
    components which includes Master Minion Firmware and Compute Minion Firmware.
*/
/***********************************************************************/

#ifndef _ERROR_CODES_H_
#define _ERROR_CODES_H_

#include <etsoc/common/common_defs.h>

/*! \def STATUS_SUCCESS
    \brief Generic status success
    TODO: Decide the right place for this. Currently it is defined in et-common-lib.
*/

/********************************** MASTER MINION ERROR CODES - START *************************************/

/****************************************************
Master Minion Error Codes Ranges.

NOTE: This is table below is in sync with error code design page.
      Any changes in these ranges should be reflected in design page
      as well.

GENERIC
- Generic ---------------------- [-1, -499]

DRIVERS
- Console ---------------------- [-500, -599]
- DMA Driver ------------------- [-600, -699]
- PLIC ------------------------- [-700, -799]
- Timer Driver ----------------- [-800, -899]

WORKERS
- DMA Worker ------------------- [-900, -999]
- Kernel Worker ---------------- [-1000, -1099]
- Compute Worker --------------- [-1100, -1199]
- Serive Processor Worker ------ [-1200, -1299]
- Submission Queue Worker ------ [-1300, -1399]
- Submission Queue HP Worker --- [-1400, -1499]
- Stat Worker ------------------ [-1500, -1599]

SERVICES
- Trace ------------------------ [-1600, -1699]
- SP Interface ----------------- [-1700, -1799]
- CM Interface  ---------------- [-1800, -1899]
- Host Interface --------------- [-1900, -1999]
- Host Command Handler --------- [-2000, -2099]
- Software Timer --------------- [-2100, -2199]
- Log -------------------------- [-2200, -2299]

DISPATCHER
- Dispatcher ------------------- [-2300, -2399]

CONFIG
- DIR Registers ---------------- [-2400, -2499]
- MM Config -------------------- [-2500, -2599]

RESERVED
- Reserved --------------------- [-2600, -2999]

****************************************************/

/*************************************
 * Define DMA Driver error codes.    *
 *************************************/

/*! \def DMA_DRIVER_ERROR_INVALID_CHAN_ID
    \brief Invalid DMA channel ID
*/
#define DMA_DRIVER_ERROR_INVALID_CHAN_ID -600

/*! \def DMA_DRIVER_ERROR_CHANNEL_NOT_AVAILABLE
    \brief DMA Channel is not available
*/
#define DMA_DRIVER_ERROR_CHANNEL_NOT_AVAILABLE -601

/*! \def DMA_DRIVER_ERROR_INVALID_ADDRESS
    \brief Invalid address for DMA operation
*/
#define DMA_DRIVER_ERROR_INVALID_ADDRESS -602

/*! \def DMA_DRIVER_CONFIG_MEM_REGION_FAILED
    \brief Mem regions configuration failed due to invalid capacity
*/
#define DMA_DRIVER_CONFIG_MEM_REGION_FAILED -603

/*************************************
 * Define DMA Worker error codes.    *
 *************************************/

/*! \def DMAW_ABORTED_IDLE_CHANNEL_SEARCH
    \brief DMA Worker - Find DMA idle channel aborted
*/
#define DMAW_ABORTED_IDLE_CHANNEL_SEARCH -900

/*! \def DMAW_ERROR_INVALID_XFER_COUNT
    \brief DMA Worker - DMA transfer count invalid
*/
#define DMAW_ERROR_INVALID_XFER_COUNT -901

/*! \def DMAW_ERROR_INVALID_XFER_SIZE
    \brief DMA Worker - DMA transfer size invalid
*/
#define DMAW_ERROR_INVALID_XFER_SIZE -902

/*! \def DMAW_ERROR_CM_IFACE_MULTICAST_FAILED
    \brief DMA Worker - CM Multicast message failed
*/
#define DMAW_ERROR_CM_IFACE_MULTICAST_FAILED -903

/*! \def DMAW_ERROR_DRIVER_DATA_CONFIG_FAILED
    \brief DMA Worker - DMA driver data node config failed
*/
#define DMAW_ERROR_DRIVER_DATA_CONFIG_FAILED -905

/*! \def DMAW_ERROR_DRIVER_LINK_CONFIG_FAILED
    \brief DMA Worker - DMA driver link node config failed
*/
#define DMAW_ERROR_DRIVER_LINK_CONFIG_FAILED -906

/*! \def DMAW_ERROR_DRIVER_INAVLID_DEV_ADDRESS
    \brief DMA Worker - Invalid device memory address
*/
#define DMAW_ERROR_DRIVER_INAVLID_DEV_ADDRESS -907

/*! \def DMAW_ERROR_DRIVER_ABORT_FAILED
    \brief DMA Worker - DMA chanel abort failed
*/
#define DMAW_ERROR_DRIVER_ABORT_FAILED -908

/*! \def DMAW_ERROR_DRIVER_CHAN_START_FAILED
    \brief DMA Worker - DMA Chanel failed to start
*/
#define DMAW_ERROR_DRIVER_CHAN_START_FAILED -909

/*************************************
 * Define Kernel Worker error codes. *
 *************************************/

/*! \def KW_ERROR_CW_MINIONS_BOOT_FAILED
    \brief Kernel Worker - Failed to Boot the Minions
*/
#define KW_ERROR_CW_MINIONS_BOOT_FAILED -1000

/*! \def KW_ABORTED_KERNEL_SLOT_SEARCH
    \brief Kernel Worker - Kernel slot search aborted
*/
#define KW_ABORTED_KERNEL_SLOT_SEARCH -1001

/*! \def KW_ERROR_KERNEL_SLOT_NOT_USED
    \brief Kernel Worker - Kernel used slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_USED -1002

/*! \def KW_ERROR_KERNEL_SLOT_NOT_FOUND
    \brief Kernel Worker - Kernel slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_FOUND -1003

/*! \def KW_ERROR_CW_SHIRES_NOT_READY
    \brief Kernel Worker - Kernel shires not ready
*/
#define KW_ERROR_CW_SHIRES_NOT_READY -1004

/*! \def KW_ERROR_KERNEL_INVALID_ADDRESS
    \brief Kernel Worker - Kernel invalid device address
*/
#define KW_ERROR_KERNEL_INVALID_ADDRESS -1005

/*! \def KW_ERROR_KERNEL_INVALID_ARGS_SIZE
    \brief Kernel Worker - Kernel invalid argument payload size
*/
#define KW_ERROR_KERNEL_INVALID_ARGS_SIZE -1006

/*! \def KW_ERROR_SW_TIMER_REGISTER_FAIL
    \brief Kernel Worker - Unable to register SW timer
*/
#define KW_ERROR_SW_TIMER_REGISTER_FAIL -1007

/*! \def KW_ERROR_TIMEDOUT_ABORT_WAIT
    \brief Kernel Worker - Timed out waiting for sending the abort to CM
*/
#define KW_ERROR_TIMEDOUT_ABORT_WAIT -1008

/*! \def KW_ERROR_KERNEL_INAVLID_SHIRE_MASK
    \brief Kernel Worker - Inavlid kernel shire mask
*/
#define KW_ERROR_KERNEL_INAVLID_SHIRE_MASK -1009

/*! \def KW_ABORTED_KERNEL_SHIRES_SEARCH
    \brief Kernel Worker - Kernel shires search aborted
*/
#define KW_ABORTED_KERNEL_SHIRES_SEARCH -1010

/*! \def KW_ERROR_CM_IFACE_UNICAST_FAILED
    \brief Kernel Worker - Failed to send a Unicast message to Compute Worker
*/
#define KW_ERROR_CM_IFACE_UNICAST_FAILED -1011

/*! \def KW_ERROR_CM_IFACE_MULTICAST_FAILED
    \brief Kernel Worker - Failed to send a Multicast message to Compute Worker
*/
#define KW_ERROR_CM_IFACE_MULTICAST_FAILED -1012

/*! \def KW_ERROR_SP_IFACE_RESET_FAILED
    \brief Kernel Worker - Failed to Reset the Minions
*/
#define KW_ERROR_SP_IFACE_RESET_FAILED -1013

/*! \def KW_ERROR_CM_ABORT_TIMEOUT
    \brief Kernel Worker - CM abort timeout occurred
*/
#define KW_ERROR_CM_ABORT_TIMEOUT -1014

/**************************************
 * Define Compute Worker error codes. *
 **************************************/

/*! \def CW_SHIRE_UNAVAILABLE
    \brief Compute Worker - Shires unavailable
*/
#define CW_SHIRE_UNAVAILABLE -1100

/*! \def CW_SHIRES_NOT_FREE
    \brief Compute Worker - Shires not free
*/
#define CW_SHIRES_NOT_FREE -1101

/*! \def CW_ERROR_INIT_TIMEOUT
    \brief Compute Worker - Init timeout occured
*/
#define CW_ERROR_INIT_TIMEOUT -1102

/****************************************
 * Define Submission Queue error codes. *
 ****************************************/

/*! \def SQW_STATUS_BARRIER_ABORTED
    \brief A macro that provide the status code for barrier abort
*/
#define SQW_STATUS_BARRIER_ABORTED -1300

/*************************************
 * Define Stat Worker error codes.   *
 *************************************/

/*! \def STATW_ERROR_GET_MM_STATS_INVALID_ARG
    \brief Stat Worker - Get MM Stats invalid argument
*/
#define STATW_ERROR_GET_MM_STATS_INVALID_ARG -1500

/*! \def STATW_ERROR_GET_MM_STATS_EVENT_COPY
    \brief Stat Worker - Get MM Stats invalid event error
*/
#define STATW_ERROR_GET_MM_STATS_INVALID_EVENT -1501

/*! \def STATW_ERROR_UPDATE_PMU_SAMPLING_STATE_TIMEOUT
    \brief Stat Worker - PMU sampling update timeout
*/
#define STATW_ERROR_UPDATE_PMU_SAMPLING_STATE_TIMEOUT -1502

/*************************************
 * Define Trace error codes.         *
 *************************************/

/*! \def TRACE_ERROR_CM_TRACE_CONFIG_FAILED
    \brief Trace - Failed to configure CM S-Mode Trace
*/
#define TRACE_ERROR_CM_TRACE_CONFIG_FAILED -1600

/*! \def TRACE_ERROR_MM_TRACE_CONFIG_FAILED
    \brief Trace - Failed to configure MM S-Mode Trace
*/
#define TRACE_ERROR_MM_TRACE_CONFIG_FAILED -1601

/*! \def TRACE_ERROR_INVALID_SHIRE_MASK
    \brief Trace - Invalid shire mask
*/
#define TRACE_ERROR_INVALID_SHIRE_MASK -1603

/*! \def TRACE_ERROR_INVALID_THREAD_MASK
    \brief Trace - Invalid thread mask
*/
#define TRACE_ERROR_INVALID_THREAD_MASK -1604

/*! \def TRACE_ERROR_INVALID_RUNTIME_TYPE
    \brief Trace - Invalid Trace runtime component
*/
#define TRACE_ERROR_INVALID_RUNTIME_TYPE -1605

/*! \def TRACE_ERROR_CM_IFACE_MULTICAST_FAILED
    \brief Trace - MM to CM multicast message failed
*/
#define TRACE_ERROR_CM_IFACE_MULTICAST_FAILED -1606

/*! \def TRACE_ERROR_INVALID_TRACE_CONFIG_INFO
    \brief Trace - Invalid Trace configuration info
*/
#define TRACE_ERROR_INVALID_TRACE_CONFIG_INFO -1607

/*************************************
 * Define SP Interface error codes.  *
 *************************************/

/*! \def SP_IFACE_INVALID_SHIRE_MASK
    \brief SP iface error code - Invalid shire mask
*/
#define SP_IFACE_INVALID_SHIRE_MASK -1700

/*! \def SP_IFACE_INVALID_BOOT_FREQ
    \brief SP iface error code - Invalid boot frequency
*/
#define SP_IFACE_INVALID_BOOT_FREQ -1701

/*! \def SP_IFACE_TIMER_REGISTER_FAILED
    \brief SP iface error code - Timer resgistration failure
*/
#define SP_IFACE_TIMER_REGISTER_FAILED -1702

/*! \def SP_IFACE_SP_RSP_TIMEDOUT
    \brief SP iface error code - SP response timeout occurred
*/
#define SP_IFACE_SP_RSP_TIMEDOUT -1703

/*! \def SP_IFACE_INVALID_FW_VERSION
    \brief SP iface error code - Invalid fw version
*/
#define SP_IFACE_INVALID_FW_VERSION -1704

/*! \def SP_IFACE_SP2MM_CMD_POP_FAILED
    \brief SP iface error code - SP2MM cmd pop failure
*/
#define SP_IFACE_SP2MM_CMD_POP_FAILED -1705

/*! \def SP_IFACE_SP2MM_RSP_POP_FAILED
    \brief SP iface error code - SP2MM rsp pop failure
*/
#define SP_IFACE_SP2MM_RSP_POP_FAILED -1706

/*! \def SP_IFACE_INVALID_RSP_ID
    \brief SP iface error code - Invalid response ID
*/
#define SP_IFACE_INVALID_RSP_ID -1707

/*************************************
 * Define CM Interface error codes.  *
 *************************************/

/*! \def CM_IFACE_MULTICAST_INAVLID_SHIRE_MASK
    \brief CM Iface error - Invalid multicast shire mask
*/
#define CM_IFACE_MULTICAST_INAVLID_SHIRE_MASK -1800

/*! \def CM_IFACE_MULTICAST_TIMEOUT_EXPIRED
    \brief CM Iface error - Wait timeout expired
*/
#define CM_IFACE_MULTICAST_TIMEOUT_EXPIRED -1801

/*! \def CM_IFACE_MULTICAST_TIMER_REGISTER_FAILED
    \brief CM Iface error - Failed to register timeout
*/
#define CM_IFACE_MULTICAST_TIMER_REGISTER_FAILED -1802

/*! \def CM_IFACE_CM_IN_BAD_STATE
    \brief CM Iface error - CM is in bad state.
*/
#define CM_IFACE_CM_IN_BAD_STATE -1803

/***********************************************
 * Define Host command handler's error codes.  *
 ***********************************************/

/*! \def HOST_CMD_STATUS_ABORTED
    \brief Host command handler - Command aborted
*/
#define HOST_CMD_STATUS_ABORTED -2000

/*! \def HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE
    \brief Host command handler - Invalid firmware type
*/
#define HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE -2001

/*! \def HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED
    \brief Host command handler - Firmware version query to SP (MM to SP Interface)
           is failed.
*/
#define HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED -2002

/*! \def HOST_CMD_ERROR_INVALID_CMD_ID
    \brief Host command handler - Invalid Command ID
*/
#define HOST_CMD_ERROR_INVALID_CMD_ID -2006

/*! \def HOST_CMD_ERROR_CM_RESET_FAILED
    \brief Host command handler - Failed to reset CM shires
*/
#define HOST_CMD_ERROR_CM_RESET_FAILED -2008

/**************************************
 * Define Software Timer error codes. *
 **************************************/

/*! \def SW_TIMER_NO_FREE_TIMESLOT_AVAILABLE
    \brief Software Timer error - No free slot available to create timer.
*/
#define SW_TIMER_NO_FREE_TIMESLOT_AVAILABLE -2100

/*************************************
 * Define MM Configuration error codes.    *
 *************************************/

/*! \def MM_CONFIG_INIT_FAILED
    \brief Failed to initialize MM configurations
*/
#define MM_CONFIG_INIT_FAILED -2500

/*! \def MM_CONFIG_INVALID_DDR_SIZE
    \brief MM config - Invalid DDR size
*/
#define MM_CONFIG_INVALID_DDR_SIZE -2501

/********************************** MASTER MINION ERROR CODES - END *************************************/

/********************************** COMPUTE MINION ERROR CODES - START ***********************************/

/***********************************************
 * Define CM MM Interface error codes.         *
 ***********************************************/

/*! \def CM_INVALID_MM_TO_CM_MESSAGE_ID
    \brief Compute worker error - Invalid MM to CM message ID.
*/
#define CM_INVALID_MM_TO_CM_MESSAGE_ID -3000

/***********************************************
 * Define CM FW (S-mode) error codes.          *
 ***********************************************/

/*! \def CM_FW_BUS_ERROR_RECEIVED
    \brief Compute worker error - Bus Error
*/
#define CM_FW_BUS_ERROR_RECEIVED -3100

/********************************** COMPUTE MINION ERROR CODES - END *************************************/

#endif
