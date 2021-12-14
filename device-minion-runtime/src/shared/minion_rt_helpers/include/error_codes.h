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

/* Defines DMA Driver error codes. Its range is -600 to -699. */
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

/********************************** MASTER MINION ERROR CODES - END *************************************/

#endif
