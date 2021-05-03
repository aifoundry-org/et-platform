#ifndef __ESPERANTO_EXECUTABLE_IMAGE_H__
#define __ESPERANTO_EXECUTABLE_IMAGE_H__

#include "certificate.h"

#define MAX_EXECUTABLE_IMAGE_LOAD_REGIONS_COUNT 8

#define CURRENT_EXEC_IMAGE_HEADER_TAG 0x474D4945 // 'EIMG'
#define CURRENT_EXEC_IMAGE_VERSION_TAG 0

#define CURRENT_EXEC_FILE_HEADER_TAG 0x48464545 // 'EEFH'
#define CURRENT_EXEC_FILE_VERSION_TAG 0

// NOTE:  Be very careful about the enum below!
//        In geenral, none of the already defined values should be changed.
//        Reordering or removing existing values is not acceptable!
//        Adding new values is OK.
//        Violating the above rules might lead to a major security exploit!!!
typedef enum ESPERANTO_IMAGE_TYPE {
    ESPERANTO_IMAGE_TYPE_INVALID = 0,
    ESPERANTO_IMAGE_TYPE_SP_BL1,
    ESPERANTO_IMAGE_TYPE_SP_BL2,
    ESPERANTO_IMAGE_TYPE_MACHINE_MINION,
    ESPERANTO_IMAGE_TYPE_MASTER_MINION,
    ESPERANTO_IMAGE_TYPE_WORKER_MINION,
    ESPERANTO_IMAGE_TYPE_COMPUTE_KERNEL,
    ESPERANTO_IMAGE_TYPE_MAXION_BL1,
    ESPERANTO_IMAGE_TYPE_COUNT,
    ESPERANTO_IMAGE_TYPE_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} ESPERANTO_IMAGE_TYPE_t;

static_assert(4 == sizeof(ESPERANTO_IMAGE_TYPE_t), "sizeof(ESPERANTO_IMAGE_TYPE_t) is not 4!");

#define ESPERANTO_EXECUTABLE_LOAD_REGION_FLAGS_LOAD 0x00000001

typedef struct ESPERANTO_EXECUTABLE_LOAD_REGION {
    uint32_t load_address_lo;
    uint32_t load_address_hi;
    uint32_t region_flags;
    uint32_t region_offset;
    uint32_t load_size;
    uint32_t memory_size;
} ESPERANTO_EXECUTABLE_LOAD_REGION_t;

static_assert(24 == sizeof(ESPERANTO_EXECUTABLE_LOAD_REGION_t), "sizeof(ESPERANTO_EXECUTABLE_LOAD_REGION_t) is not 24!");

#define MAX_GIT_VERSION_LENGTH 112

typedef struct ESPERANTO_IMAGE_INFO_PUBLIC {
    HEADER_TAG_t header_tag;
    VERSION_TAG_t version_tag;
    uint32_t image_info_flags;
    uint32_t file_version;                          // monotonic version of the executable image
    ESPERANTO_IMAGE_TYPE_t image_type;
    uint32_t revocation_counter;
    DATE_AND_TIME_STAMP_t fileDateAndTimeStamp;
    HASH_ALG_t code_and_data_hash_algorithm;
    uint8_t  code_and_data_hash[64];
    uint32_t code_and_data_size;
    uint8_t  git_hash[32];
    uint8_t  git_version[MAX_GIT_VERSION_LENGTH];
    uint8_t  reserved[8];
} ESPERANTO_IMAGE_INFO_PUBLIC_t;

static_assert(256 == sizeof(ESPERANTO_IMAGE_INFO_PUBLIC_t), "sizeof(ESPERANTO_IMAGE_INFO_PUBLIC_t) is not 256!");

#define IMAGE_INFO_SECRET_EXEC_FLAGS_64BIT  1

typedef struct ESPERANTO_IMAGE_INFO_SECRET {
    uint32_t load_regions_count;
    uint32_t exec_flags;
    uint32_t exec_address_lo;
    uint32_t exec_address_hi;
    uint8_t reserved[48];
    ESPERANTO_EXECUTABLE_LOAD_REGION_t load_regions[MAX_EXECUTABLE_IMAGE_LOAD_REGIONS_COUNT];
} ESPERANTO_IMAGE_INFO_SECRET_t;

static_assert(256 == sizeof(ESPERANTO_IMAGE_INFO_SECRET_t), "sizeof(ESPERANTO_IMAGE_INFO_SECRET_t) is not 256!");

typedef struct ESPERANTO_IMAGE_INFO {
    ESPERANTO_IMAGE_INFO_PUBLIC_t public_info;
    ESPERANTO_IMAGE_INFO_SECRET_t secret_info;
} ESPERANTO_IMAGE_INFO_t;

static_assert(512 == sizeof(ESPERANTO_IMAGE_INFO_t), "sizeof(ESPERANTO_IMAGE_INFO_t) is not 512!");

typedef struct ESPERANTO_IMAGE_INFO_AND_SIGNATURE {
    ESPERANTO_IMAGE_INFO_t info;
    PUBLIC_SIGNATURE_t info_signature;
    uint8_t reserved[112];
} ESPERANTO_IMAGE_INFO_AND_SIGNATURE_t;

static_assert(1152 == sizeof(ESPERANTO_IMAGE_INFO_AND_SIGNATURE_t), "sizeof(ESPERANTO_IMAGE_INFO_AND_SIGNATURE_t) is not 1152!");

typedef enum ESPERANTO_MAC_TYPE {
    ESPERANTO_MAC_TYPE_INVALID = 0,
    ESPERANTO_MAC_TYPE_AES_CMAC,
    ESPERANTO_MAC_TYPE_HMAC_SHA2_256,
    ESPERANTO_MAC_TYPE_HMAC_SHA2_384,
    ESPERANTO_MAC_TYPE_HMAC_SHA2_512,
    ESPERANTO_MAC_TYPE_HMAC_SHA3_256,
    ESPERANTO_MAC_TYPE_HMAC_SHA3_384,
    ESPERANTO_MAC_TYPE_HMAC_SHA3_512,
    ESPERANTO_MAC_TYPE_COUNT,
    ESPERANTO_MAC_TYPE_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} ESPERANTO_MAC_TYPE_t;

typedef struct IMAGE_VERSION_INFO {
    unsigned int prolog_tag;
    union {
        uint32_t file_version;
        struct {
            uint8_t file_version_revision;
            uint8_t file_version_minor;
            uint8_t file_version_major;
            uint8_t file_version_reserved;
        };
    };
    unsigned char git_hash[32];
    char git_version[MAX_GIT_VERSION_LENGTH];
    unsigned int epilog_tag;
} IMAGE_VERSION_INFO_t;


typedef struct ESPERANTO_IMAGE_FILE_HEADER_INFO {
    HEADER_TAG_t file_header_tag;
    VERSION_TAG_t file_version_tag;
    IMAGE_VERSION_INFO_t image_version_info;
    uint32_t file_header_flags;
    uint32_t reserved;
    ESPERANTO_CERTIFICATE_t signing_certificate;
    uint32_t reserved2[44/sizeof(uint32_t)];
    ESPERANTO_MAC_TYPE_t mac_type;
    uint8_t encryption_IV[16];
    ESPERANTO_IMAGE_INFO_AND_SIGNATURE_t image_info_and_signaure;
    uint8_t encrypted_code_and_data_hash[64];
} ESPERANTO_IMAGE_FILE_HEADER_INFO_t;

static_assert(3164 == sizeof(ESPERANTO_IMAGE_FILE_HEADER_INFO_t), "sizeof(ESPERANTO_IMAGE_FILE_HEADER_INFO_t) is not 3164!");

#define ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED  1

typedef struct ESPERANTO_IMAGE_FILE_HEADER {
    ESPERANTO_IMAGE_FILE_HEADER_INFO_t info;
    uint8_t MAC[64];
} ESPERANTO_IMAGE_FILE_HEADER_t;

static_assert(3228 == sizeof(ESPERANTO_IMAGE_FILE_HEADER_t), "sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) is not 3228!");

#define _MACRO_TO_STRING(x) #x
#define MACRO_TO_STRING(x) _MACRO_TO_STRING(x)
#define IMAGE_VERSION_INFO_SYMBOL g_image_version_info
#define IMAGE_VERSION_INFO_SYMBOL_NAME MACRO_TO_STRING(IMAGE_VERSION_INFO_SYMBOL)
#define IMAGE_VERSION_INFO_EPILOG_TAG 0x54455645 // 'EVET'
#define IMAGE_VERSION_INFO_PROLOG_TAG 0x54505645 // 'EVPT'

#endif // __ESPERANTO_EXECUTABLE_IMAGE_H__
