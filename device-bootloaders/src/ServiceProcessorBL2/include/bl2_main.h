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

#ifndef __BL2_MAIN_H__
#define __BL2_MAIN_H__

#include "service_processor_BL2_data.h"

SERVICE_PROCESSOR_BL2_DATA_t * get_service_processor_bl2_data(void);

#ifdef RELEASE_BUILD
#define MESSAGE_ERROR_DEBUG(cformat, ...)
#define MESSAGE_INFO_DEBUG(cformat, ...)
#else
#define MESSAGE_ERROR_DEBUG(cformat, ...) printf(cformat, ##__VA_ARGS__)
#define MESSAGE_INFO_DEBUG(cformat, ...) printf(cformat, ##__VA_ARGS__)
#endif

#define MESSAGE_ERROR(cformat, ...) printf(cformat, ##__VA_ARGS__)
#define MESSAGE_INFO(cformat, ...) printf(cformat, ##__VA_ARGS__)

#endif
