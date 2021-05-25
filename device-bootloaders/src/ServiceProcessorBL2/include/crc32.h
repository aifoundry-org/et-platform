/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>
#include <stdlib.h>

/*! \fn void crc32(const void *data, size_t n_bytes, uint32_t *crc)
    \brief This function calculates crc of input data
    \param None
    \return Status indicating success or negative error
*/
void crc32(const void *data, size_t n_bytes, uint32_t *crc);

#endif
