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

#ifndef BL2_SCRATCH_BUFFER__H
#define BL2_SCRATCH_BUFFER__H

#include <stdint.h>

void *get_scratch_buffer(uint32_t *size);

#endif  //BL2_SCRATCH_BUFFER__H
