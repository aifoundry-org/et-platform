#ifndef __ESPERANTO_CERTIFICATE_REQUEST_H__
#define __ESPERANTO_CERTIFICATE_REQUEST_H__

#include "public_key_data.h"

typedef uint32_t HEADER_TAG_t;
typedef uint32_t VERSION_TAG_t;

#define MAX_ORGANIZATION_LENGTH     64
#define MAX_ORG_UNIT_LENGTH         64
#define MAX_CITY_LENGTH             24
#define MAX_STATE_LENGTH            8
#define MAX_COMMON_NAME_LENGTH      72
#define MAX_SERIAL_NUMBER_LENGTH    16

typedef struct KEY_IDENTIFIER {
    uint8_t bytes[32];
} KEY_IDENTIFIER_t;

typedef struct SECURITY_CERTIFICATE_NAME_SEQUENCE {
    char country[2];
    uint8_t organization_length;
    char organization[MAX_ORGANIZATION_LENGTH];
    uint8_t org_unit_length;
    char org_unit[MAX_ORG_UNIT_LENGTH];
    uint8_t city_length;
    char city[MAX_CITY_LENGTH];
    uint8_t state_length;
    char state[MAX_STATE_LENGTH];
    uint8_t common_name_length;
    char common_name[MAX_COMMON_NAME_LENGTH];
    uint8_t serial_number_length;
    char serial_number[MAX_SERIAL_NUMBER_LENGTH];
} SECURITY_CERTIFICATE_NAME_SEQUENCE_t;

static_assert(256 == sizeof(SECURITY_CERTIFICATE_NAME_SEQUENCE_t), "sizeof(SECURITY_CERTIFICATE_NAME_SEQUENCE_t) is not 256!");

#define CURRENT_CERTIFICATE_REQUEST_HEADER_TAG 0x51524345 // 'ECRQ'
#define CURRENT_CERTIFICATE_REQUEST_VERSION_TAG 0

typedef struct SECURITY_CERTIFICATE_REQUEST_INFO {
    HEADER_TAG_t header_tag;
    VERSION_TAG_t version_tag;
    SECURITY_CERTIFICATE_NAME_SEQUENCE_t subject;
    PUBLIC_KEY_t subject_public_key;
    HASH_ALG_t hash_algorithm;
    uint32_t reserved[3];
    KEY_IDENTIFIER_t subject_key_identifier;
} SECURITY_CERTIFICATE_REQUEST_INFO_t;

typedef struct ESPERANTO_CERTIFICATE_REQUEST {
    SECURITY_CERTIFICATE_REQUEST_INFO_t request_info;
    PUBLIC_SIGNATURE_t request_info_signature;
} ESPERANTO_CERTIFICATE_REQUEST_t;

#endif //__ESPERANTO_CERTIFICATE_REQUEST_H__
