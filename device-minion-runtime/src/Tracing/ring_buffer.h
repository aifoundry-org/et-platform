/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_RING_BUFFER_H
#define ET_RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
namespace device_api {
#endif

void *ring_buffer_alloc_space(uint16_t hartid, size_t size);

#ifdef __cplusplus
} // namespace device_api
#endif

#endif // ET_RING_BUFFER_H
