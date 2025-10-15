/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __BL2_MAIN_H__
#define __BL2_MAIN_H__

#include "bl_error_code.h"
#include "service_processor_BL2_data.h"
#include "log.h"

SERVICE_PROCESSOR_BL2_DATA_t *get_service_processor_bl2_data(void);

#include "config/mgmt_dir_regs.h"

bool is_vaultip_disabled(void);

#ifdef RELEASE_BUILD
#define MESSAGE_ERROR_DEBUG(cformat, ...)
#define MESSAGE_INFO_DEBUG(cformat, ...)
#else
#define MESSAGE_ERROR_DEBUG(cformat, ...) Log_Write(LOG_LEVEL_DEBUG, cformat, ##__VA_ARGS__)
#define MESSAGE_INFO_DEBUG(cformat, ...)  Log_Write(LOG_LEVEL_INFO, cformat, ##__VA_ARGS__)
#endif

#define MESSAGE_ERROR(cformat, ...) Log_Write(LOG_LEVEL_ERROR, cformat, ##__VA_ARGS__)
#define MESSAGE_INFO(cformat, ...)  Log_Write(LOG_LEVEL_INFO, cformat, ##__VA_ARGS__)

#endif
