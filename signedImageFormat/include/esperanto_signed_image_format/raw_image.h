#ifndef __ESPERANTO_RAW_IMAGE_H__
#define __ESPERANTO_RAW_IMAGE_H__

#include "executable_image.h"

#define CURRENT_RAW_IMAGE_HEADER_TAG 0x57415245 // 'ERAW'
#define CURRENT_RAW_IMAGE_VERSION_TAG 0

#define CURRENT_RAW_FILE_HEADER_TAG 0x48465245 // 'ERFH'
#define CURRENT_RAW_FILE_VERSION_TAG 0

// NOTE:  Be very careful about the enum below!
//        In geenral, none of the already defined values should be changed.
//        Reordering or removing existing values is not acceptable!
//        Adding new values is OK.
//        Violating the above rules might lead to a major security exploit!!!
typedef enum ESPERANTO_RAW_IMAGE_TYPE {
    ESPERANTO_RAW_IMAGE_TYPE_INVALID = 0,
    ESPERANTO_RAW_IMAGE_TYPE_PCIE_PHY_FW,
    ESPERANTO_RAW_IMAGE_TYPE_DRAM_CONTROLLER_FW,
    ESPERANTO_RAW_IMAGE_TYPE_ASSET_CONFIG,
    ESPERANTO_RAW_IMAGE_TYPE_COUNT,
    ESPERANTO_RAW_IMAGE_TYPE_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} ESPERANTO_RAW_IMAGE_TYPE_t;

static_assert(4 == sizeof(ESPERANTO_RAW_IMAGE_TYPE_t), "sizeof(ESPERANTO_RAW_IMAGE_TYPE_t) is not 4!");

typedef struct ESPERANTO_RAW_IMAGE_INFO {
    HEADER_TAG_t header_tag;
    VERSION_TAG_t version_tag;
    uint32_t image_info_flags;
    uint32_t file_version;                          // monotonic version of the executable image
    ESPERANTO_RAW_IMAGE_TYPE_t image_type;
    uint32_t revocation_counter;
    DATE_AND_TIME_STAMP_t fileDateAndTimeStamp;
    HASH_ALG_t raw_image_hash_algorithm;
    uint8_t  raw_image_hash[64];
    uint32_t raw_image_size; // true size in bytes, file size will be rounded up to the next 128 bytes
    uint8_t  git_hash[32];
    uint8_t  git_version[MAX_GIT_VERSION_LENGTH];
    uint8_t  reserved[8];
} ESPERANTO_RAW_IMAGE_INFO_t;

static_assert(256 == sizeof(ESPERANTO_RAW_IMAGE_INFO_t), "sizeof(ESPERANTO_RAW_IMAGE_INFO_t) is not 256!");

typedef struct ESPERANTO_RAW_IMAGE_INFO_AND_SIGNATURE {
    ESPERANTO_RAW_IMAGE_INFO_t info;
    PUBLIC_SIGNATURE_t info_signature;
    uint8_t reserved[112];
} ESPERANTO_RAW_IMAGE_INFO_AND_SIGNATURE_t;

static_assert(896 == sizeof(ESPERANTO_RAW_IMAGE_INFO_AND_SIGNATURE_t), "sizeof(ESPERANTO_RAW_IMAGE_INFO_AND_SIGNATURE_t) is not 896!");

typedef struct ESPERANTO_RAW_IMAGE_FILE_HEADER_INFO {
    HEADER_TAG_t file_header_tag;
    VERSION_TAG_t file_version_tag;
    uint32_t file_header_flags;
    uint32_t reserved;
    ESPERANTO_CERTIFICATE_t signing_certificate;
    uint32_t reserved2[44/sizeof(uint32_t)];
    ESPERANTO_MAC_TYPE_t mac_type;
    uint8_t encryption_IV[16];
    ESPERANTO_RAW_IMAGE_INFO_AND_SIGNATURE_t image_info_and_signaure;
    uint8_t encrypted_code_and_data_hash[64];
} ESPERANTO_RAW_IMAGE_FILE_HEADER_INFO_t;

static_assert(2752 == sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_INFO_t), "sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_INFO_t) is not 2752!");

#define ESPERANTO_RAW_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED  1

typedef struct ESPERANTO_RAW_IMAGE_FILE_HEADER {
    ESPERANTO_RAW_IMAGE_FILE_HEADER_INFO_t info;
    uint8_t MAC[64];
} ESPERANTO_RAW_IMAGE_FILE_HEADER_t;

static_assert(2816 == sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t), "sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t) is not 2816!");

#endif // __ESPERANTO_RAW_IMAGE_H__
