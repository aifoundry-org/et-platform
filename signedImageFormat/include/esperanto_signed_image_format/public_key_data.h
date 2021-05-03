#ifndef __ESPERANTO_PUBLIC_KEY_DATA_H__
#define __ESPERANTO_PUBLIC_KEY_DATA_H__

#include <stdint.h>
#include <assert.h>

#define ECC_KEY_MAX_POINT_DATA_SIZE 68

typedef enum EC_KEY_CURVE_ID {
    EC_KEY_CURVE_INVALID = 0,
    EC_KEY_CURVE_NIST_P256,
    EC_KEY_CURVE_NIST_P384,
    EC_KEY_CURVE_NIST_P521,
    EC_KEY_CURVE_CURVE25519,
    EC_KEY_CURVE_EDWARDS25519,
    EC_KEY_CURVE_AFTER_LAST_SUPPORTED_VALUE,
    EC_KEY_CURVE_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} EC_KEY_CURVE_ID_t;

static_assert(4 == sizeof(EC_KEY_CURVE_ID_t), "sizeof(EC_KEY_CURVE_ID_t) is not 4!");

typedef struct PUBLIC_KEY_EC {
    EC_KEY_CURVE_ID_t curveID;
    uint32_t pXsize;
    uint32_t pYsize;
    uint8_t pX[ECC_KEY_MAX_POINT_DATA_SIZE];
    uint8_t pY[ECC_KEY_MAX_POINT_DATA_SIZE];
} PUBLIC_KEY_EC_t;

#define RSA_KEY_MAX_PUB_EXP_DATA_SIZE 8
#define RSA_KEY_MAX_MODULUS_DATA_SIZE (4096/8)

typedef struct PUBLIC_KEY_RSA {
    uint32_t keySize;
    uint32_t pubExpSize;
    uint32_t pubModSize;
    uint8_t pubExp[RSA_KEY_MAX_PUB_EXP_DATA_SIZE];
    uint8_t pubMod[RSA_KEY_MAX_MODULUS_DATA_SIZE];
} PUBLIC_KEY_RSA_t;

typedef enum PUBLIC_KEY_TYPE {
    PUBLIC_KEY_TYPE_INVALID = 0,
    PUBLIC_KEY_TYPE_RSA,
    PUBLIC_KEY_TYPE_EC,
    PUBLIC_KEY_TYPE_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} PUBLIC_KEY_TYPE_t;

static_assert(4 == sizeof(PUBLIC_KEY_TYPE_t), "sizeof(PUBLIC_KEY_TYPE_t) is not 4!");

typedef struct PUBLIC_KEY {
    PUBLIC_KEY_TYPE_t keyType;
    union {
        PUBLIC_KEY_RSA_t rsa;
        PUBLIC_KEY_EC_t ec;
    };
} PUBLIC_KEY_t;

typedef struct EC_SIGNATURE {
    EC_KEY_CURVE_ID_t curveID;
    uint32_t rSize;
    uint32_t sSize;
    uint8_t r[ECC_KEY_MAX_POINT_DATA_SIZE];
    uint8_t s[ECC_KEY_MAX_POINT_DATA_SIZE];
} EC_SIGNATURE_t;

typedef enum HASH_ALG {
    HASH_ALG_INVALID = 0,
    HASH_ALG_SHA2_256,
    HASH_ALG_SHA2_384,
    HASH_ALG_SHA2_512,
    HASH_ALG_SHA3_256,
    HASH_ALG_SHA3_384,
    HASH_ALG_SHA3_512,
    HASH_ALG_AFTER_LAST_SUPPORTED_VALUE,
    HASH_ALG_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} HASH_ALG_t;

static_assert(4 == sizeof(HASH_ALG_t), "sizeof(HASH_ALG_t) is not 4!");

typedef struct RSA_SIGNATURE {
    uint32_t keySize;
    uint32_t sigSize;
    uint8_t signature[RSA_KEY_MAX_MODULUS_DATA_SIZE];
} RSA_SIGNATURE_t;

typedef struct PUBLIC_SIGNATURE {
    HASH_ALG_t hashAlg;
    PUBLIC_KEY_TYPE_t keyType;
    union {
        RSA_SIGNATURE_t rsa;
        EC_SIGNATURE_t ec;
    };
} PUBLIC_SIGNATURE_t;

#endif // __ESPERANTO_PUBLIC_KEY_DATA_H__
