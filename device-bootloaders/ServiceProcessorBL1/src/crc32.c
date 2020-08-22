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

#include "crc32.h"

#pragma GCC push_options
//#pragma GCC optimize ("O3")

// Simple public domain implementation of the standard CRC32 checksum.
static uint32_t crc32_for_byte(uint32_t r)
{
    for (int j = 0; j < 8; ++j) {
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    }
    return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t *crc)
{
    static uint32_t table[0x100];
    if (!*table) {
        for (size_t i = 0; i < 0x100; ++i) {
            table[i] = crc32_for_byte((uint32_t)i);
        }
    }
    for (size_t i = 0; i < n_bytes; ++i) {
        *crc = table[(const uint8_t)*crc ^ ((const uint8_t *)data)[i]] ^ *crc >> 8;
    }
}

//    uint32_t crc = 0;
//    crc32(data, firmware_size, &crc);

#pragma GCC pop_options
