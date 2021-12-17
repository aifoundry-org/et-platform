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

SERVICES
- Trace ------------------------ [-1500, -1599]
- SP Interface ----------------- [-1600, -1699]
- CM Interface  ---------------- [-1700, -1799]
- Host Interface --------------- [-1800, -1899]
- Host Command Handler --------- [-1900, -1999]
- SW Timer --------------------- [-2000, -2099]
- Log -------------------------- [-2100, -2199]

DISPATCHER
- Dispatcher ------------------- [-2200, -2299]

CONFIG
- DIR Registers ---------------- [-2300, -2399]

RESERVED
- Reserved --------------------- [-2400, -2999]

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

/*************************************
 * Define Trace error codes.         *
 *************************************/

/*! \def TRACE_ERROR_CM_TRACE_CONFIG_FAILED
    \brief Trace - Failed to configure CM S-Mode Trace
*/
#define TRACE_ERROR_CM_TRACE_CONFIG_FAILED -1500

/*! \def TRACE_ERROR_MM_TRACE_CONFIG_FAILED
    \brief Trace - Failed to configure MM S-Mode Trace
*/
#define TRACE_ERROR_MM_TRACE_CONFIG_FAILED -1501

/*! \def TRACE_ERROR_CW_SHIRE_NOT_BOOTED
    \brief Trace - Trace enabled Compute Worker shires are not booted.
*/
#define TRACE_ERROR_CW_SHIRE_NOT_BOOTED -1502

/*! \def TRACE_ERROR_INVALID_SHIRE_MASK
    \brief Trace - Invalid shire mask
*/
#define TRACE_ERROR_INVALID_SHIRE_MASK -1503

/*! \def TRACE_ERROR_INVALID_THREAD_MASK
    \brief Trace - Invalid thread mask
*/
#define TRACE_ERROR_INVALID_THREAD_MASK -1504

/*! \def TRACE_ERROR_INVALID_RUNTIME_TYPE
    \brief Trace - Invalid Trace runtime component
*/
#define TRACE_ERROR_INVALID_RUNTIME_TYPE -1505

/*! \def TRACE_ERROR_CM_IFACE_MULTICAST_FAILED
    \brief Trace - MM to CM multicast message failed
*/
#define TRACE_ERROR_CM_IFACE_MULTICAST_FAILED -1506

/*************************************
 * Define SP Interface error codes.  *
 *************************************/

/*! \def SP_IFACE_INVALID_SHIRE_MASK
    \brief SP iface error code - Invalid shire mask
*/
#define SP_IFACE_INVALID_SHIRE_MASK -1600

/*! \def SP_IFACE_INVALID_BOOT_FREQ
    \brief SP iface error code - Invalid boot frequency
*/
#define SP_IFACE_INVALID_BOOT_FREQ -1601

/*! \def SP_IFACE_TIMER_REGISTER_FAILED
    \brief SP iface error code - Timer resgistration failure
*/
#define SP_IFACE_TIMER_REGISTER_FAILED -1602

/*! \def SP_IFACE_SP_RSP_TIMEDOUT
    \brief SP iface error code - SP response timeout occurred
*/
#define SP_IFACE_SP_RSP_TIMEDOUT -1603

/*! \def SP_IFACE_INVALID_FW_VERSION
    \brief SP iface error code - Invalid fw version
*/
#define SP_IFACE_INVALID_FW_VERSION -1604

/*! \def SP_IFACE_SP2MM_CMD_POP_FAILED
    \brief SP iface error code - SP2MM cmd pop failure
*/
#define SP_IFACE_SP2MM_CMD_POP_FAILED -1605

/*! \def SP_IFACE_SP2MM_RSP_POP_FAILED
    \brief SP iface error code - SP2MM rsp pop failure
*/
#define SP_IFACE_SP2MM_RSP_POP_FAILED -1606

/*! \def SP_IFACE_INVALID_RSP_ID
    \brief SP iface error code - Invalid response ID
*/
#define SP_IFACE_INVALID_RSP_ID -1607

/*************************************
 * Define CM Interface error codes.  *
 *************************************/

/*! \def CM_IFACE_MULTICAST_INAVLID_SHIRE_MASK
    \brief CM Iface error - Invalid multicast shire mask
*/
#define CM_IFACE_MULTICAST_INAVLID_SHIRE_MASK -1700

/***********************************************
 * Define Host command handler's error codes.  *
 ***********************************************/

/*! \def HOST_CMD_STATUS_ABORTED
    \brief Host command handler - Command aborted
*/
#define HOST_CMD_STATUS_ABORTED -1900

/*! \def HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE
    \brief Host command handler - Invalid firmware type
*/
#define HOST_CMD_ERROR_FW_VER_INVALID_FW_TYPE -1901

/*! \def HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED
    \brief Host command handler - Firmware version query to SP (MM to SP Interface)
           is failed.
*/
#define HOST_CMD_ERROR_SP_IFACE_FW_QUERY_FAILED -1902

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_MAJOR
    \brief Host command handler - Invalid Major firmware version
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_MAJOR -1903

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_MINOR
    \brief Host command handler - Invalid Minor firmware version
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_MINOR -1904

/*! \def HOST_CMD_ERROR_API_COMP_INVALID_PATCH
    \brief Host command handler - Invalid patch for current firmware
*/
#define HOST_CMD_ERROR_API_COMP_INVALID_PATCH -1905

/********************************** MASTER MINION ERROR CODES - END *************************************/

#endif
