/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
