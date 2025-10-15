/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __BL1_SP_FIRMWARE_LOADER_H__
#define __BL1_SP_FIRMWARE_LOADER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "service_processor_BL1_data.h"

int load_bl2_firmware(void);

#endif
