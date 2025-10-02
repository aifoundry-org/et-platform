/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL1_MAIN_H__
#define __BL1_MAIN_H__

//#define RELEASE_BUILD

#include "service_processor_BL1_data.h"

SERVICE_PROCESSOR_BL1_DATA_t *get_service_processor_bl1_data(void);

bool is_vaultip_disabled(void);

#ifdef RELEASE_BUILD
#define MESSAGE_ERROR_DEBUG(cformat, ...)
#define MESSAGE_INFO_DEBUG(cformat, ...)
#else
#define MESSAGE_ERROR_DEBUG(cformat, ...) printx(cformat, ##__VA_ARGS__)
#define MESSAGE_INFO_DEBUG(cformat, ...)  printx(cformat, ##__VA_ARGS__)
#endif

#define MESSAGE_ERROR(cformat, ...) printx(cformat, ##__VA_ARGS__)
#define MESSAGE_INFO(cformat, ...)  printx(cformat, ##__VA_ARGS__)

#endif
