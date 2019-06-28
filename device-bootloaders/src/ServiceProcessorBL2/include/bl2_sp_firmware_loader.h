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

#ifndef __BL1_SP_FIRMWARE_LOADER_H__
#define __BL1_SP_FIRMWARE_LOADER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "service_processor_BL2_data.h"

int load_minion_machine_minion_firmware(void);
int load_minion_master_minion_firmware(void);
int load_minion_worker_minion_firmware(void);

#endif
