/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __JEDEC_SFDP_H__
#define __JEDEC_SFDP_H__

typedef struct SFDP_HEADER {
    union {
        char signature[4];
        uint32_t dw0;
    };
    union {
        uint32_t dw1;
        struct {
            uint8_t minor_revision;
            uint8_t major_revision;
            uint8_t number_of_parameter_headers;
            uint8_t access_protocol;
        };
    };
} SFDP_HEADER_t;

#define SFDP_HEADER_SIGNATURE 0x50444653

typedef struct SFDP_PARAMETER_HEADER {
    union {
        uint32_t dw0;
        struct {
            uint8_t parameter_id_lsb;
            uint8_t minor_revision;
            uint8_t major_revision;
            uint8_t parameter_length;
        };
    };
    union {
        uint32_t dw1;
        struct {
            uint32_t parameter_pointer : 24;
            uint32_t parameter_id_msb : 8;
        };
    };
} SFDP_PARAMETER_HEADER_t;

typedef struct SFDP_BASIC_FLASH_PARAMETER_TABLE {
    uint32_t dword[20];
} SFDP_BASIC_FLASH_PARAMETER_TABLE_t;

#endif
