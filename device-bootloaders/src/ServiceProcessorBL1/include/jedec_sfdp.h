#ifndef __JEDEC_SFDP_H__
#define __JEDEC_SFDP_H__

typedef struct SFDP_HEADER {
    char signature[4];
    uint8_t minor_revision;
    uint8_t major_revision;
    uint8_t number_of_parameter_headers;
    uint8_t access_protocol;
} SFDP_HEADER_t;

typedef struct SFDP_PARAMETER_HEADER {
    uint8_t parameter_id_lsb;
    uint8_t minor_revision;
    uint8_t major_revision;
    uint8_t parameter_length;
    uint8_t parameter_pointer_23_16;
    uint8_t parameter_pointer_15_08;
    uint8_t parameter_pointer_07_00;
    uint8_t parameter_id_msb;
} SFDP_PARAMETER_HEADER_t;

typedef struct SFDP_BASIC_FLASH_PARAMETER_TABLE {
    uint32_t dword[20];
} SFDP_BASIC_FLASH_PARAMETER_TABLE_t;

#endif
