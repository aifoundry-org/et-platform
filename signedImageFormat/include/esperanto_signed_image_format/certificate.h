#ifndef __ESPERANTO_CERTIFICATE_H__
#define __ESPERANTO_CERTIFICATE_H__

#include "certificate_request.h"

typedef struct DATE_AND_TIME_STAMP {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t seconds_frac;
} DATE_AND_TIME_STAMP_t;

static_assert(8 == sizeof(DATE_AND_TIME_STAMP_t), "sizeof(DATE_AND_TIME_STAMP_t) is not 8!");

#define CURRENT_CERTIFICATE_HEADER_TAG 0x54524345 // 'ECRT'
#define CURRENT_CERTIFICATE_VERSION_TAG 0

typedef struct ESPERANTO_CERTIFICATE_INFO {
    HEADER_TAG_t header_tag;
    VERSION_TAG_t version_tag;
    uint32_t serial_number;
    SECURITY_CERTIFICATE_NAME_SEQUENCE_t issuer;
    HASH_ALG_t hash_algorithm;
    PUBLIC_KEY_TYPE_t signing_key_type;
    union {
        uint32_t keySize;
        EC_KEY_CURVE_ID_t curveID;
    };
    struct {
        uint32_t special_customer_id : 8;
        uint32_t reserved : 24;
    } special;
    uint32_t reserved;
    DATE_AND_TIME_STAMP_t valid_from;
    DATE_AND_TIME_STAMP_t valid_until;
    SECURITY_CERTIFICATE_NAME_SEQUENCE_t subject;
    PUBLIC_KEY_t subject_public_key;
    uint32_t is_CA;
    uint32_t ca_depth;
    uint32_t x509_attributes;
    uint32_t esperanto_attributes;
    uint32_t esperanto_designation;
    uint32_t revocation_counter;
    KEY_IDENTIFIER_t issuer_key_identifier;
    KEY_IDENTIFIER_t subject_key_identifier;
} ESPERANTO_CERTIFICATE_INFO_t;

static_assert(1184 == sizeof(ESPERANTO_CERTIFICATE_INFO_t), "sizeof(ESPERANTO_CERTIFICATE_INFO_t) is not 1184!");

#define ESPERANTO_CERTIFICATE_ATTRBUTES_DEVELOPMENT_CERTIFICATE (1u <<  0)

#define ESPERANTO_CERTIFICATE_DESIGNATION_ROOT_CA               (1u <<  0)
#define ESPERANTO_CERTIFICATE_DESIGNATION_ISSUING_CA            (1u <<  1)
#define ESPERANTO_CERTIFICATE_DESIGNATION_BL1_CA                (1u <<  2)
#define ESPERANTO_CERTIFICATE_DESIGNATION_BL2_CA                (1u <<  3)
#define ESPERANTO_CERTIFICATE_DESIGNATION_PCIE_CFG_CA           (1u <<  4)
#define ESPERANTO_CERTIFICATE_DESIGNATION_DRAM_CFG_CA           (1u <<  5)
#define ESPERANTO_CERTIFICATE_DESIGNATION_MACHINE_MINION_CA     (1u <<  6)
#define ESPERANTO_CERTIFICATE_DESIGNATION_MASTER_MINION_CA      (1u <<  7)
#define ESPERANTO_CERTIFICATE_DESIGNATION_WORKER_MINION_CA      (1u <<  8)
#define ESPERANTO_CERTIFICATE_DESIGNATION_MAXION_BL1_CA         (1u <<  9)

typedef struct ESPERANTO_CERTIFICATE {
    ESPERANTO_CERTIFICATE_INFO_t certificate_info;
    PUBLIC_SIGNATURE_t certificate_info_signature;
} ESPERANTO_CERTIFICATE_t;

static_assert(1712 == sizeof(ESPERANTO_CERTIFICATE_t), "sizeof(ESPERANTO_CERTIFICATE_t) is not 1712!");

#endif // __ESPERANTO_CERTIFICATE_H__
