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
/*! \file crypto_rot.c
    \brief A C module that implements the crypto functions

    Public interfaces:
        watchdog_init
        watchdog_error_init
        watchdog_start
        watchdog_stop
        watchdog_kick
        get_watchdog_timeout
        get_watchdog_max_timeout
*/
/***********************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bl2_main.h"
#include "bl2_crypto.h"
#include "vaultip_hw.h"
#include "vaultip_sw.h"
#include "vaultip_sw_asset.h"
#include "bl2_vaultip_controller.h"
#include "bl2_vaultip_driver.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

/*! \def SUPPORT_RSA_2048
    \brief RSA 2048 support definition.
*/
#define SUPPORT_RSA_2048

/*! \def SUPPORT_RSA_3072
    \brief RSA 3072 support definition.
*/
#define SUPPORT_RSA_3072

/*! \def SUPPORT_RSA_4096
    \brief RSA 4096 support definition.
*/
#define SUPPORT_RSA_4096

/*! \def SUPPORT_EC_P256
    \brief EC P256 support definition.
*/
#define SUPPORT_EC_P256

/*! \def SUPPORT_EC_P384
    \brief EC P384 support definition.
*/
#define SUPPORT_EC_P384

/*! \def SUPPORT_EC_P521
    \brief EC P521 support definition.
*/
#define SUPPORT_EC_P521

//#define SUPPORT_EC_CURVE25519

/*! \def SUPPORT_EC_EDWARDS25519
    \brief EC EDWARDS25519 support definition.
*/
#define SUPPORT_EC_EDWARDS25519

#include "ec_domain_parameters.h"

typedef union ASSET_POLICY_u 
{
    uint64_t u64;
    struct 
    {
        uint32_t lo;
        uint32_t hi;
    };
} ASSET_POLICY_t;

static SemaphoreHandle_t gs_mutex_crypto_create_ec_parameters_asset;
static SemaphoreHandle_t gs_mutex_crypto_create_ec_public_key_asset;
static SemaphoreHandle_t gs_mutex_crypto_ecdsa_verify;
static SemaphoreHandle_t gs_mutex_crypto_create_rsa_public_key_asset;
static SemaphoreHandle_t gs_mutex_crypto_rsa_verify;
static SemaphoreHandle_t gs_mutex_crypto_get_monotonic_counter_value;

static StaticSemaphore_t gs_mutex_buffer_crypto_create_ec_parameters_asset;
static StaticSemaphore_t gs_mutex_buffer_crypto_create_ec_public_key_asset;
static StaticSemaphore_t gs_mutex_buffer_crypto_ecdsa_verify;
static StaticSemaphore_t gs_mutex_buffer_crypto_create_rsa_public_key_asset;
static StaticSemaphore_t gs_mutex_buffer_crypto_rsa_verify;
static StaticSemaphore_t gs_mutex_buffer_crypto_get_monotonic_counter_value;

static bool coid_provisioned = false;
#define ESPERANTO_COID 0x4F435445 /* 'ETCO' */

uint32_t get_rom_identity(void)
{
    if (coid_provisioned) 
    {
        return ESPERANTO_COID;
    }
    else
    {
        return 0x0;
    }
}

int crypto_init(uint32_t vaultip_coid_set)
{
    coid_provisioned = vaultip_coid_set ? true : false;

    gs_mutex_crypto_create_ec_parameters_asset =
        xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_create_ec_parameters_asset);
    gs_mutex_crypto_create_ec_public_key_asset =
        xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_create_ec_public_key_asset);
    gs_mutex_crypto_ecdsa_verify =
        xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_ecdsa_verify);
    gs_mutex_crypto_create_rsa_public_key_asset =
        xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_create_rsa_public_key_asset);
    gs_mutex_crypto_rsa_verify = xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_rsa_verify);
    gs_mutex_crypto_get_monotonic_counter_value =
        xSemaphoreCreateMutexStatic(&gs_mutex_buffer_crypto_get_monotonic_counter_value);

    return 0;
}

int crypto_derive_kdk_key(const void *kdk_derivation_data, size_t kdk_derivation_data_size,
                          uint32_t *kdk_asset_id)
{
    uint32_t huk_asset_id;
    uint32_t huk_asset_length;
    ASSET_POLICY_t kdk_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_TRUSTED_KEY_DERIVE,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t kdk_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .DataLength = 256 / 8,
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };
    uint32_t kdk_lifetime = 0;

    /* find HUK */
    if (0 != vaultip_drv_static_asset_search(get_rom_identity(), VAULTIP_STATIC_ASSET_HUK,
                                             &huk_asset_id, &huk_asset_length))
    {
        MESSAGE_ERROR_DEBUG("derive_kdk: vaultip_drv_static_asset_search(HUK) failed!\n");
        return -1;
    }

    /* create KDK */
    if (0 != vaultip_drv_asset_create(get_rom_identity(), kdk_policy.lo, kdk_policy.hi,
                                      kdk_other_settings, kdk_lifetime, kdk_asset_id)) 
    {
        MESSAGE_ERROR_DEBUG("derive_kdk: vaultip_drv_asset_create() failed!\n");
        return -1;
    }

    /* derive KDK */
    if (0 != vaultip_drv_asset_load_derive(get_rom_identity(), *kdk_asset_id, huk_asset_id, NULL,
                                           0, (const uint8_t *)kdk_derivation_data,
                                           (uint32_t)kdk_derivation_data_size, NULL, 0)) 
    {
        MESSAGE_ERROR_DEBUG("derive_kdk: vaultip_drv_asset_load_derive() failed!\n");
        return -1;
    }

    return 0;
}

int crypto_derive_mac_key(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t kdk_asset_id,
                          const void *mack_derivation_data, size_t mack_derivation_data_size,
                          uint32_t *mack_asset_id)
{
    ASSET_POLICY_t mack_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t mack_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .DataLength = 256 / 8,
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };

    uint32_t mack_lifetime = 0;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mack_policy.u64 = mack_policy.u64 | VAL_POLICY_CMAC | VAL_POLICY_ALGO_CIPHER_AES;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mack_policy.u64 = mack_policy.u64 | VAL_POLICY_SHA256;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mack_policy.u64 = mack_policy.u64 | VAL_POLICY_SHA384;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mack_policy.u64 = mack_policy.u64 | VAL_POLICY_SHA512;
            break;
        default:
            return -1;
    }

    /* create MACK */
    if (0 != vaultip_drv_asset_create(get_rom_identity(), mack_policy.lo, mack_policy.hi,
                                      mack_other_settings, mack_lifetime, mack_asset_id))
    {
        MESSAGE_ERROR_DEBUG("derive_mack: vaultip_drv_asset_create() failed!\n");
        return -1;
    }

    /* derive MACK */
    if (0 != vaultip_drv_asset_load_derive(get_rom_identity(), *mack_asset_id, kdk_asset_id, NULL,
                                           0, (const uint8_t *)mack_derivation_data,
                                           (uint32_t)mack_derivation_data_size, NULL, 0))
    {
        MESSAGE_ERROR_DEBUG("derive_mack: vaultip_drv_asset_load_derive() failed!\n");
        return -1;
    }

    return 0;
}

int crypto_derive_enc_key(uint32_t kdk_asset_id, const void *enck_derivation_data,
                          size_t enck_derivation_data_size, uint32_t *enck_asset_id)
{
    ASSET_POLICY_t enck_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT | VAL_POLICY_ALGO_CIPHER_AES |
               VAL_POLICY_AES_MODE_CBC,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t enck_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .DataLength = 256 / 8,
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };
    uint32_t enck_lifetime = 0;

    /* create ENCK */
    if (0 != vaultip_drv_asset_create(get_rom_identity(), enck_policy.lo, enck_policy.hi,
                                      enck_other_settings, enck_lifetime, enck_asset_id))
    {
        MESSAGE_ERROR_DEBUG("derive_enck: vaultip_drv_asset_create() failed!\n");
        return -1;
    }

    /* derive ENCK */
    if (0 != vaultip_drv_asset_load_derive(get_rom_identity(), *enck_asset_id, kdk_asset_id, NULL,
                                           0, (const uint8_t *)enck_derivation_data,
                                           (uint32_t)enck_derivation_data_size, NULL, 0))
    {
        MESSAGE_ERROR_DEBUG("derive_enck: vaultip_drv_asset_load_derive() failed!\n");
        return -1;
    }

    return 0;
}

int crypto_delete_key(uint32_t key)
{
    if (0 != vaultip_drv_asset_delete(get_rom_identity(), key)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_delete_key: vaultip_drv_asset_delete() failed!\n");
        return -1;
    }
    return 0;
}

int crypto_verify_public_key_params(const PUBLIC_KEY_t *public_key)
{
    uint32_t maxXsize, maxYsize;

    switch (public_key->keyType)
    {
        case PUBLIC_KEY_TYPE_RSA:
            if (public_key->rsa.pubExpSize > sizeof(public_key->rsa.pubExp))
            {
                MESSAGE_ERROR_DEBUG("verify_public_key_params: invalid RSA public \
                    exponent size!\n");
                return -1;
            }

            switch (public_key->rsa.keySize) 
            {
                case 2048:
                case 3072:
                case 4096:
                    if (public_key->rsa.pubModSize > (public_key->rsa.keySize / 8)) 
                    {
                        MESSAGE_ERROR_DEBUG("verify_public_key_params: invalid RSA public \
                            modulus size!\n");
                        return -1;
                    }
                    break;
                default:
                    MESSAGE_ERROR_DEBUG(
                        "verify_public_key_params: invalid or not supported RSA key size!\n");
                    return -1;
            }
            break;

        case PUBLIC_KEY_TYPE_EC:
            switch (public_key->ec.curveID) 
            {
#ifdef SUPPORT_EC_P256
            case EC_KEY_CURVE_NIST_P256:
                maxXsize = 256 / 8;
                maxYsize = 256 / 8;
                break;
#endif
            case EC_KEY_CURVE_NIST_P384:
                maxXsize = 384 / 8;
                maxYsize = 384 / 8;
                break;
            case EC_KEY_CURVE_NIST_P521:
                maxXsize = 68;
                maxYsize = 68;
                break;
            case EC_KEY_CURVE_CURVE25519:
                maxXsize = 256 / 8;
                maxYsize = 0;
                break;
            case EC_KEY_CURVE_EDWARDS25519:
                maxXsize = 256 / 8;
                maxYsize = 0;
                break;
            default:
                MESSAGE_ERROR_DEBUG("verify_public_key_params: invalid or not supported \
                    EC curve!\n");
                return -1;
            }

        if (public_key->ec.pXsize > maxXsize)
        {
            MESSAGE_ERROR_DEBUG(
                "verify_public_key_params: invalid EC public point X component size!\n");
            return -1;
        }
        if (public_key->ec.pYsize > maxYsize)
        {
            MESSAGE_ERROR_DEBUG(
                "verify_public_key_params: invalid EC public point Y component size!\n");
            return -1;
        }
        break;

    default:
        MESSAGE_ERROR_DEBUG("verify_public_key_params: invalid or not supported key type!\n");
        return -1;
    }

    return 0;
}

int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t *signature)
{
    uint32_t maxRsize, maxSsize;

    switch (signature->hashAlg)
    {
        case HASH_ALG_SHA2_256:
        case HASH_ALG_SHA2_384:
        case HASH_ALG_SHA2_512:
            break;
        default:
            MESSAGE_ERROR_DEBUG(
                "verify_signature_params: invalid or not supported signature hash algorithm!\n");
            return -1;
    }

    switch (signature->keyType)
    {
        case PUBLIC_KEY_TYPE_RSA:
            switch (signature->rsa.keySize) 
            {
                case 2048:
                case 3072:
                case 4096:
                    if (signature->rsa.sigSize > (signature->rsa.keySize / 8)) 
                    {
                        MESSAGE_ERROR_DEBUG("verify_signature_params: invalid RSA \
                            signature size!\n");
                        return -1;
                    }
                    break;
                default:
                    MESSAGE_ERROR_DEBUG(
                        "verify_signature_params: invalid or not supported RSA key size!\n");
                    return -1;
            }
            break;

        case PUBLIC_KEY_TYPE_EC:
            switch (signature->ec.curveID)
            {
#ifdef SUPPORT_EC_P256
                case EC_KEY_CURVE_NIST_P256:
                    maxRsize = 256 / 8;
                    maxSsize = 256 / 8;
                    break;
#endif
                case EC_KEY_CURVE_NIST_P384:
                    maxRsize = 384 / 8;
                    maxSsize = 384 / 8;
                    break;
                case EC_KEY_CURVE_NIST_P521:
                    maxRsize = 68;
                    maxSsize = 68;
                    break;
                case EC_KEY_CURVE_CURVE25519:
                    maxRsize = 256 / 8;
                    maxSsize = 0;
                    break;
                case EC_KEY_CURVE_EDWARDS25519:
                    maxRsize = 256 / 8;
                    maxSsize = 0;
                    break;
                default:
                    MESSAGE_ERROR_DEBUG("verify_signature_params: invalid or not supported \
                        EC curve!\n");
                    return -1;
            }

            if (signature->ec.rSize > maxRsize) 
            {
                MESSAGE_ERROR_DEBUG(
                    "verify_signature_params: invalid EC signature R component size!\n");
                return -1;
            }
            if (signature->ec.sSize > maxSsize) 
            {
                MESSAGE_ERROR_DEBUG(
                    "verify_signature_params: invalid EC signature S component size!\n");
                return -1;
            }
            break;

        default:
            MESSAGE_ERROR_DEBUG(
                "verify_signature_params: invalid or not supported signature key algorithm!\n");
            return -1;
        }

    return 0;
}

int crypto_hash(HASH_ALG_t hash_alg, const void *msg, size_t msg_size, uint8_t *hash)
{
    return vaultip_drv_hash(get_rom_identity(), hash_alg, msg, msg_size, hash);
}

int crypto_hash_init(CRYPTO_HASH_CONTEXT_t *hash_context, HASH_ALG_t hash_alg)
{
    ASSET_POLICY_t temp_digest_asset_policy;
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t temp_digest_asset_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };

    if (NULL == hash_context) 
    {
        MESSAGE_ERROR_DEBUG("crypto_hash_init: invalid arguments!\n");
        return -1;
    }

    hash_context->hash_alg = HASH_ALG_INVALID;
    switch (hash_alg) 
    {
        case HASH_ALG_SHA2_256:
            temp_digest_asset_policy.u64 = VAL_POLICY_SHA256 | VAL_POLICY_TEMP_MAC;
            temp_digest_asset_other_settings.DataLength = 32;
            break;
        case HASH_ALG_SHA2_384:
            temp_digest_asset_policy.u64 = VAL_POLICY_SHA384 | VAL_POLICY_TEMP_MAC;
            temp_digest_asset_other_settings.DataLength = 64;
            break;
        case HASH_ALG_SHA2_512:
            temp_digest_asset_policy.u64 = VAL_POLICY_SHA512 | VAL_POLICY_TEMP_MAC;
            temp_digest_asset_other_settings.DataLength = 64;
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_hash_init: invalid hash algorithm!\n");
            return -1;
    }

    if (0 != vaultip_drv_asset_create(get_rom_identity(), temp_digest_asset_policy.lo,
                                      temp_digest_asset_policy.hi, 
                                      temp_digest_asset_other_settings,
                                      0, /* lifetime, */
                                      &(hash_context->temp_digest_asset_id))) 
    {
        MESSAGE_ERROR_DEBUG("crypto_hash_init: vaultip_drv_asset_create() failed!\n");
        return -1;
    }

    hash_context->hash_alg = hash_alg;
    hash_context->init_done = false;
    return 0;
}

int crypto_hash_abort(CRYPTO_HASH_CONTEXT_t *hash_context)
{
    if (NULL == hash_context)
    {
        return -1;
    }

    if (hash_context->hash_alg != HASH_ALG_INVALID)
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), hash_context->temp_digest_asset_id))
        {
            MESSAGE_ERROR_DEBUG("crypto_hash_abort: vaultip_drv_asset_delete failed!");
            return -1;
        }
    }

    hash_context->hash_alg = HASH_ALG_INVALID;
    hash_context->temp_digest_asset_id = 0;
    hash_context->init_done = false;

    return 0;
}

int crypto_hash_update(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size)
{
    int rv;

    if (NULL == hash_context)
    {
        return -1;
    }

    if (hash_context->init_done)
    {
        rv = vaultip_drv_hash_update(get_rom_identity(), hash_context->hash_alg,
                                     hash_context->temp_digest_asset_id, msg, msg_size, false);
    }
    else
    {
        rv = vaultip_drv_hash_update(get_rom_identity(), hash_context->hash_alg,
                                     hash_context->temp_digest_asset_id, msg, msg_size, true);
        if (0 == rv)
        {
            hash_context->init_done = true;
        }
    }
    return rv;
}

int crypto_hash_final(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size,
                      size_t total_msg_length, uint8_t *hash)
{
    int rv;

    if (NULL == hash_context)
    {
        return -1;
    }

    if (hash_context->init_done)
    {
        rv = vaultip_drv_hash_final(get_rom_identity(), hash_context->hash_alg,
                                    hash_context->temp_digest_asset_id, msg, msg_size, false,
                                    total_msg_length, hash);
    }
    else
    {
        rv = vaultip_drv_hash_final(get_rom_identity(), hash_context->hash_alg,
                                    hash_context->temp_digest_asset_id, msg, msg_size, true,
                                    total_msg_length, hash);
    }

    if (0 == rv)
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), hash_context->temp_digest_asset_id))
        {
            MESSAGE_ERROR_DEBUG("crypto_hash_final: vaultip_drv_asset_delete failed!");
            rv = -1;
        }

        hash_context->hash_alg = HASH_ALG_INVALID;
        hash_context->temp_digest_asset_id = 0;
        hash_context->init_done = false;
    }

    return rv;
}

static void crypto_reverse_copy(void *dst, const void *src, size_t size)
{
    uint8_t *pd = (uint8_t *)dst;
    const uint8_t *ps = (const uint8_t *)src;
    const uint8_t *p = ps + size;

    while (p != ps) 
    {
        p--;
        *pd = *p;
        pd++;
    }
}

int crypto_mac_verify(ESPERANTO_MAC_TYPE_t mac_alg, const uint32_t mack_key, const void *data,
                      size_t data_size, const void *mac)
{
    if (NULL == data || 0 == data_size) 
    {
        return -1;
    }

    /* compute MAC */
    if (0 != vaultip_drv_mac_verify(get_rom_identity(), mac_alg, mack_key, data, data_size, mac))
    {
        MESSAGE_ERROR_DEBUG("test_kdk: vaultip_drv_mac_verify() failed!\n");
        return -1;
    }

    return 0;
}

int crypto_aes_decrypt_init(CRYPTO_AES_CONTEXT_t *aes_context, const uint32_t enck_key,
                            const uint8_t *IV)
{
    if (NULL == aes_context)
    {
        return -1;
    }

    if (NULL == IV)
    {
        return -1;
    }

    aes_context->aes_key_asset_id = enck_key;
    memcpy(aes_context->IV, IV, 16);
    return 0;
}

int crypto_aes_decrypt_update(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size)
{
    if (0 != vaultip_drv_aes_cbc_decrypt(get_rom_identity(), aes_context->aes_key_asset_id,
                                         aes_context->IV, data, data_size))
    {
        MESSAGE_ERROR_DEBUG("crypto_aes_decrypt_update: vaultip_drv_aes_cbc_encrypt() failed!\n");
        return -1;
    }

    return 0;
}

int crypto_aes_decrypt_final(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size,
                             uint8_t *IV)
{
    if (NULL != data && data_size > 0)
    {
        if (0 != vaultip_drv_aes_cbc_decrypt(get_rom_identity(), aes_context->aes_key_asset_id,
                                             aes_context->IV, data, data_size))
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_aes_decrypt_final: vaultip_drv_aes_cbc_encrypt() failed!\n");
            return -1;
        }
    }

    if (NULL != IV)
    {
        memcpy(IV, aes_context->IV, 16);
    }

    return 0;
}

static int crypto_write_subvector_32(VAULTIP_SUBVECTOR_32_t *const subvector,
                                     uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                     const void *data_addr, size_t data_size, uint16_t bits)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data) || 0 == bits || bits > 32)
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_32: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = bits;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}

static int crypto_write_subvector_64(VAULTIP_SUBVECTOR_64_t *const subvector,
                                     uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                     const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_64: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 64;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}

#if defined(SUPPORT_EC_P256) || defined(SUPPORT_EC_CURVE25519) || defined(SUPPORT_EC_EDWARDS25519)
static int crypto_write_subvector_256(VAULTIP_SUBVECTOR_256_t *const subvector,
                                      uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                      const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_256: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 256;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P384)
static int crypto_write_subvector_384(VAULTIP_SUBVECTOR_384_t *const subvector,
                                      uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                      const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_384: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 384;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P521)
static int crypto_write_subvector_521(VAULTIP_SUBVECTOR_521_t *const subvector,
                                      uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                      const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_521: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 384;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_RSA_2048)
static int crypto_write_subvector_2048(VAULTIP_SUBVECTOR_2048_t *const subvector,
                                       uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                       const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_2048: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 2048;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_RSA_3072)
static int crypto_write_subvector_3072(VAULTIP_SUBVECTOR_3072_t *const subvector,
                                       uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                       const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_3072: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 3072;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_RSA_4096)
static int crypto_write_subvector_4096(VAULTIP_SUBVECTOR_4096_t *const subvector,
                                       uint8_t nrOfSubvectors, uint8_t subvectorIndex,
                                       const void *data_addr, size_t data_size)
{
    uint32_t bytes_remaining;
    if (NULL == subvector || NULL == data_addr || 0 == data_size ||
        data_size > sizeof(subvector->data))
    {
        MESSAGE_ERROR_DEBUG("crypto_write_subvector_4096: invalid arguments!\n");
        return -1;
    }

    subvector->header.SubVectorLength = 4096;
    subvector->header.SubVectorIndex = subvectorIndex;
    subvector->header.NrOfSubVectors = nrOfSubvectors;

    crypto_reverse_copy(subvector->data.u8, data_addr, data_size);

    if (data_size > sizeof(subvector->data))
    {
        bytes_remaining = (uint32_t)(data_size - sizeof(subvector->data));
        memset(subvector->data.u8 + data_size, 0, bytes_remaining);
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P256) || defined(SUPPORT_EC_CURVE25519) || defined(SUPPORT_EC_EDWARDS25519)
static int crypto_create_ec_256_parameters_asset(VAULTIP_EC_256_DOMAIN_PARAMETERS_t *ec_256,
                                                 const ECC_DOMAIN_PARAMETERS_t *domain_parameters)
{
    uint8_t bits = 1;
    uint8_t CoFactor[4] = { 0, 0, 0, 1 };

    if (NULL == ec_256 || NULL == domain_parameters)
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_256_parameters_asset: invalid arguments!\n");
        return -1;
    }

    /* copy curve modulus (p) */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_modulus_p), 7, 0, domain_parameters->P_p,
                                        domain_parameters->PLen))
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_256_parameters_asset: \
                                crypto_write_subvector_256(modulus_p) failed!\n");
        return -1;
    }

    /* copy curve constant a */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_constant_a), 7, 1, domain_parameters->A_p,
                                        domain_parameters->ALen)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_256(const_A) \
                failed!\n");
        return -1;
    }

    /* copy curve constant b */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_constant_b), 7, 2, domain_parameters->B_p,
                                        domain_parameters->BLen)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_256(const_B) \
                failed!\n");
        return -1;
    }

    /* copy curve order (n) */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_order_n), 7, 3, domain_parameters->Order_p,
                                        domain_parameters->OrderLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_256(order_n) \
                failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate X */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_base_point_x), 7, 4,
                                        domain_parameters->ECPointX_p,
                                        domain_parameters->ECPointXLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_256(point_x) \
                failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate Y */
    if (0 != crypto_write_subvector_256(&(ec_256->curve_base_point_y), 7, 5,
                                        domain_parameters->ECPointY_p,
                                        domain_parameters->ECPointYLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_256(point_y) \
                failed!\n");
        return -1;
    }

    /* copy curve cofactor */
    if (0 != domain_parameters->Cofactor && 1 != domain_parameters->Cofactor) 
    {
        uint8_t n = domain_parameters->Cofactor;

        bits = 0;
        while (n != 0) 
        {
            bits++;
            n >>= 1;
        }
        CoFactor[3] = domain_parameters->Cofactor;
    }
    if (0 != crypto_write_subvector_32(&(ec_256->curve_cofactor), 7, 6, CoFactor, sizeof(CoFactor),
                                       bits)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_256_parameters_asset: crypto_write_subvector_32(cofactor) \
                failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P384)
static int crypto_create_ec_384_parameters_asset(VAULTIP_EC_384_DOMAIN_PARAMETERS_t *ec_384,
                                                 const ECC_DOMAIN_PARAMETERS_t *domain_parameters)
{
    uint8_t bits = 1;
    uint8_t CoFactor[4] = { 0, 0, 0, 1 };

    if (NULL == ec_384 || NULL == domain_parameters) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_384_parameters_asset: invalid arguments!\n");
        return -1;
    }

    /* copy curve modulus (p) */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_modulus_p), 7, 0, domain_parameters->P_p,
                                        domain_parameters->PLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(modulus_p) \
                 failed!\n");
        return -1;
    }

    /* copy curve constant a */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_constant_a), 7, 1, domain_parameters->A_p,
                                        domain_parameters->ALen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(const_A) \
                 failed!\n");
        return -1;
    }

    /* copy curve constant b */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_constant_b), 7, 2, domain_parameters->B_p,
                                        domain_parameters->BLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(const_B) \
                 failed!\n");
        return -1;
    }

    /* copy curve order (n) */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_order_n), 7, 3, domain_parameters->Order_p,
                                        domain_parameters->OrderLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(order_n) \
                failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate X */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_base_point_x), 7, 4,
                                        domain_parameters->ECPointX_p,
                                        domain_parameters->ECPointXLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(point_x) \
                 failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate Y */
    if (0 != crypto_write_subvector_384(&(ec_384->curve_base_point_y), 7, 5,
                                        domain_parameters->ECPointY_p,
                                        domain_parameters->ECPointYLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_384(point_y) \
                 failed!\n");
        return -1;
    }

    /* copy curve cofactor */
    if (0 != domain_parameters->Cofactor && 1 != domain_parameters->Cofactor)
    {
        uint8_t n = domain_parameters->Cofactor;

        bits = 0;
        while (n != 0)
        {
            bits++;
            n >>= 1;
        }
        CoFactor[3] = domain_parameters->Cofactor;
    }
    if (0 != crypto_write_subvector_32(&(ec_384->curve_cofactor), 7, 6, CoFactor, sizeof(CoFactor),
                                       bits))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_384_parameters_asset: crypto_write_subvector_32(cofactor) \
                 failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P521)
static int crypto_create_ec_521_parameters_asset(VAULTIP_EC_521_DOMAIN_PARAMETERS_t *ec_521,
                                                 const ECC_DOMAIN_PARAMETERS_t *domain_parameters)
{
    uint8_t bits = 1;
    uint8_t CoFactor[4] = { 0, 0, 0, 1 };

    if (NULL == ec_521 || NULL == domain_parameters)
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_521_parameters_asset: invalid arguments!\n");
        return -1;
    }

    /* copy curve modulus (p) */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_modulus_p), 7, 0, domain_parameters->P_p,
                                        domain_parameters->PLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(modulus_p) \
                 failed!\n");
        return -1;
    }

    /* copy curve constant a */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_constant_a), 7, 1, domain_parameters->A_p,
                                        domain_parameters->ALen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(const_A) \
                 failed!\n");
        return -1;
    }

    /* copy curve constant b */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_constant_b), 7, 2, domain_parameters->B_p,
                                        domain_parameters->BLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(const_B) \
                 failed!\n");
        return -1;
    }

    /* copy curve order (n) */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_order_n), 7, 3, domain_parameters->Order_p,
                                        domain_parameters->OrderLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(order_n) \
                failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate X */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_base_point_x), 7, 4,
                                        domain_parameters->ECPointX_p,
                                        domain_parameters->ECPointXLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(point_x) \
                failed!\n");
        return -1;
    }

    /* copy curve base point (g) coordinate Y */
    if (0 != crypto_write_subvector_521(&(ec_521->curve_base_point_y), 7, 5,
                                        domain_parameters->ECPointY_p,
                                        domain_parameters->ECPointYLen))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_521(point_y) \
                 failed!\n");
        return -1;
    }

    /* copy curve cofactor */
    if (0 != domain_parameters->Cofactor && 1 != domain_parameters->Cofactor)
    {
        uint8_t n = domain_parameters->Cofactor;

        bits = 0;
        while (n != 0)
        {
            bits++;
            n >>= 1;
        }
        CoFactor[3] = domain_parameters->Cofactor;
    }
    if (0 != crypto_write_subvector_32(&(ec_521->curve_cofactor), 7, 6, CoFactor, sizeof(CoFactor),
                                       bits))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_521_parameters_asset: crypto_write_subvector_32(cofactor) \
                 failed!\n");
        return -1;
    }

    return 0;
}
#endif

/* static void dump_asset(const void * asset_addr, uint32_t asset_size) {
     const uint32_t * pwords = (const uint32_t *)asset_addr;
     uint32_t count = asset_size / 4;
     MESSAGE_INFO_DEBUG("Asset size: %08x, data:", asset_size);
     while (count > 0) {
         MESSAGE_INFO_DEBUG(" %08x", *pwords);
         pwords++;
         count--;
     }
     MESSAGE_INFO_DEBUG("\n");
 }
*/
static int crypto_create_ec_parameters_asset(EC_KEY_CURVE_ID_t curve_id,
                                             uint32_t *ec_parameters_asset_id)
{
    int rv;

    static union
    {
#if defined(SUPPORT_EC_P256) || defined(SUPPORT_EC_CURVE25519) || defined(SUPPORT_EC_EDWARDS25519)
        VAULTIP_EC_256_DOMAIN_PARAMETERS_t ec_256;
#endif
#if defined(SUPPORT_EC_P384)
        VAULTIP_EC_384_DOMAIN_PARAMETERS_t ec_384;
#endif
#if defined(SUPPORT_EC_P521)
        VAULTIP_EC_521_DOMAIN_PARAMETERS_t ec_521;
#endif
    } domain_parameters_data;
    const void *domain_parameters_data_ptr;
    uint32_t domain_parameters_size;
    uint32_t asset_id;

    ASSET_POLICY_t public_key_parameters_asset_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_PUBLIC_KEY_PARAM,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t public_key_parameters_asset_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_create_ec_parameters_asset, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_parameters_asset: xSemaphoreTake() failed!\n");
        return -1;
    }

    switch (curve_id) 
    {
#if defined(SUPPORT_EC_P256)
        case EC_KEY_CURVE_NIST_P256:
            if (0 != crypto_create_ec_256_parameters_asset(&(domain_parameters_data.ec_256),
                                                        &ECurve_NIST_P256)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_parameters_asset: crypto_create_ec_256_parameters_asset() \
                         failed!\n");
                rv = -1;
                goto DONE;
            }
            domain_parameters_data_ptr = &(domain_parameters_data.ec_256);
            domain_parameters_size = sizeof(domain_parameters_data.ec_256);
            break;
#endif
#if defined(SUPPORT_EC_P384)
        case EC_KEY_CURVE_NIST_P384:
            if (0 != crypto_create_ec_384_parameters_asset(&(domain_parameters_data.ec_384),
                                                        &ECurve_NIST_P384)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_parameters_asset: crypto_create_ec_384_parameters_asset() \
                         failed!\n");
                rv = -1;
                goto DONE;
            }
            domain_parameters_data_ptr = &(domain_parameters_data.ec_384);
            domain_parameters_size = sizeof(domain_parameters_data.ec_384);
            break;
#endif
#if defined(SUPPORT_EC_P521)
        case EC_KEY_CURVE_NIST_P521:
            if (0 != crypto_create_ec_521_parameters_asset(&(domain_parameters_data.ec_521),
                                                        &ECurve_NIST_P521)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_parameters_asset: crypto_create_ec_521_parameters_asset() \
                         failed!\n");
                rv = -1;
                goto DONE;
            }
            domain_parameters_data_ptr = &(domain_parameters_data.ec_521);
            domain_parameters_size = sizeof(domain_parameters_data.ec_521);
            break;
#endif
#if defined(SUPPORT_EC_CURVE25519)
        case EC_KEY_CURVE_CURVE25519:
            if (0 != crypto_create_ec_256_parameters_asset(&(domain_parameters_data.ec_256),
                                                        &ECurve_25519)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_parameters_asset: crypto_create_ec_256_parameters_asset() \
                         failed!\n");
                rv = -1;
                goto DONE;
            }
            domain_parameters_data_ptr = &(domain_parameters_data.ec_256);
            domain_parameters_size = sizeof(domain_parameters_data.ec_256);
            break;
#endif
#if defined(SUPPORT_EC_EDWARDS25519)
        case EC_KEY_CURVE_EDWARDS25519:
            if (0 != crypto_create_ec_256_parameters_asset(&(domain_parameters_data.ec_256),
                                                        &ECurve_Ed25519)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_parameters_asset: crypto_create_ec_256_parameters_asset() \
                        failed!\n");
                rv = -1;
                goto DONE;
            }
            domain_parameters_data_ptr = &(domain_parameters_data.ec_256);
            domain_parameters_size = sizeof(domain_parameters_data.ec_256);
            break;
#endif
        default:
            MESSAGE_ERROR_DEBUG("crypto_create_ec_parameters_asset: invalid curve_id!\n");
            rv = -1;
            goto DONE;
    }

    /* MESSAGE_INFO_DEBUG("crypto_create_ec_parameters_asset: asset size = 0x%x\n", 
        domain_parameters_size); */
    public_key_parameters_asset_other_settings.DataLength = domain_parameters_size & 0x3FFu;
    if (0 != vaultip_drv_asset_create(get_rom_identity(), public_key_parameters_asset_policy.lo,
                                      public_key_parameters_asset_policy.hi,
                                      public_key_parameters_asset_other_settings, 0, &asset_id)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_parameters_asset: vaultip_drv_asset_create() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != vaultip_drv_asset_load_plaintext(get_rom_identity(), asset_id,
                                              domain_parameters_data_ptr, domain_parameters_size))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_parameters_asset: vaultip_drv_asset_load_plaintext() failed!\n");
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), asset_id))
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_create_ec_parameters_asset: vaultip_drv_asset_delete() failed!\n");
        }
        rv = -1;
        goto DONE;
    }
    /* MESSAGE_INFO_DEBUG("Domain parameters asset data @%p:", domain_parameters_data_ptr);
     dump_asset(domain_parameters_data_ptr, domain_parameters_size); */

    *ec_parameters_asset_id = asset_id;
    rv = 0;

DONE:
    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_create_ec_parameters_asset))
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_parameters_asset: xSemaphoreGive() failed!\n");
        rv = -1;
    }
    return rv;
}

#if defined(SUPPORT_EC_P256)
static int crypto_create_ec_p256_public_key_asset(VAULTIP_PUBLIC_KEY_ECDSA_P256_t *ec_256,
                                                  const PUBLIC_KEY_EC_t *public_key)
{
    if (NULL == ec_256 || NULL == public_key || EC_KEY_CURVE_NIST_P256 != public_key->curveID)
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_p256_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy point X coordinate */
    if (0 !=
        crypto_write_subvector_256(&(ec_256->point_x), 2, 0, public_key->pX, public_key->pXsize))
    {
            MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p256_public_key_asset: crypto_write_subvector_256(pX) failed!\n");
        return -1;
    }

    /* copy point Y coordinate */
    if (0 !=
        crypto_write_subvector_256(&(ec_256->point_y), 2, 1, public_key->pY, public_key->pYsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p256_public_key_asset: crypto_write_subvector_256(pY) failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P384)
static int crypto_create_ec_p384_public_key_asset(VAULTIP_PUBLIC_KEY_ECDSA_P384_t *ec_384,
                                                  const PUBLIC_KEY_EC_t *public_key)
{
    if (NULL == ec_384 || NULL == public_key || EC_KEY_CURVE_NIST_P384 != public_key->curveID) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_p384_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy point X coordinate */
    if (0 !=
        crypto_write_subvector_384(&(ec_384->point_x), 2, 0, public_key->pX, public_key->pXsize))
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p384_public_key_asset: crypto_write_subvector_384(pX) failed!\n");
        return -1;
    }

    /* copy point Y coordinate */
    if (0 !=
        crypto_write_subvector_384(&(ec_384->point_y), 2, 1, public_key->pY, public_key->pYsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p384_public_key_asset: crypto_write_subvector_384(pY) failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_P521)
static int crypto_create_ec_p521_public_key_asset(VAULTIP_PUBLIC_KEY_ECDSA_P521_t *ec_521,
                                                  const PUBLIC_KEY_EC_t *public_key)
{
    if (NULL == ec_521 || NULL == public_key || EC_KEY_CURVE_NIST_P521 != public_key->curveID) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_p521_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy point X coordinate */
    if (0 !=
        crypto_write_subvector_521(&(ec_521->point_x), 2, 0, public_key->pX, public_key->pXsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p521_public_key_asset: crypto_write_subvector_521(pX) failed!\n");
        return -1;
    }

    /* copy point Y coordinate */
    if (0 !=
        crypto_write_subvector_521(&(ec_521->point_y), 2, 1, public_key->pY, public_key->pYsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_p521_public_key_asset: crypto_write_subvector_521(pY) failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_CURVE25519)
static int crypto_create_ec_curve25519_public_key_asset(VAULTIP_PUBLIC_KEY_ECDSA_25519_t *ec_25519,
                                                        const PUBLIC_KEY_EC_t *public_key)
{
    if (NULL == ec_25519 || NULL == public_key || EC_KEY_CURVE_CURVE25519 != public_key->curveID) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_curve25519_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy point X coordinate */
    if (0 != crypto_write_subvector_256(&(ec_25519->point_x), 1, 0, public_key->pX,
                                        public_key->pXsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_curve25519_public_key_asset: crypto_write_subvector_256(pX) \
                failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_EC_EDWARDS25519)
static int
crypto_create_ec_edwards25519_public_key_asset(VAULTIP_PUBLIC_KEY_ECDSA_25519_t *ec_25519,
                                               const PUBLIC_KEY_EC_t *public_key)
{
    if (NULL == ec_25519 || NULL == public_key ||
        EC_KEY_CURVE_EDWARDS25519 != public_key->curveID) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_edwards25519_public_key_asset: invalid \
            arguments!\n");
        return -1;
    }

    /* copy point X coordinate */
    if (0 != crypto_write_subvector_256(&(ec_25519->point_x), 1, 0, public_key->pX,
                                        public_key->pXsize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_edwards25519_public_key_asset: crypto_write_subvector_256(pX) \
                failed!\n");
        return -1;
    }

    return 0;
}
#endif

static int crypto_create_ec_public_key_asset(HASH_ALG_t hash_alg,
                                             const PUBLIC_KEY_EC_t *ec_public_key,
                                             uint32_t *ec_public_key_asset_id)
{
    int rv;
    static union 
    {
#if defined(SUPPORT_EC_P256)
        VAULTIP_PUBLIC_KEY_ECDSA_P256_t p256;
#endif
#if defined(SUPPORT_EC_P256)
        VAULTIP_PUBLIC_KEY_ECDSA_P384_t p384;
#endif
#if defined(SUPPORT_EC_P256)
        VAULTIP_PUBLIC_KEY_ECDSA_P521_t p521;
#endif
#if defined(SUPPORT_EC_CURVE25519)
        VAULTIP_PUBLIC_KEY_ECDSA_25519_t curve25519;
#endif
#if defined(SUPPORT_EC_EDWARDS25519)
        VAULTIP_PUBLIC_KEY_ECDSA_25519_t edwards25519;
#endif
    } public_key_data;
    const void *public_key_data_ptr;
    uint32_t public_key_data_size;
    uint32_t asset_id;

    ASSET_POLICY_t public_key_asset_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_PUBLIC_KEY | VAL_POLICY_PK_ECC_ECDSA_SIGN,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t public_key_asset_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };

    if (NULL == ec_public_key || NULL == ec_public_key_asset_id) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_public_key_asset: invalid arguments!\n");
        return -1;
    }

    switch (hash_alg) 
    {
        case HASH_ALG_SHA2_256:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA256;
            break;
        case HASH_ALG_SHA2_384:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA384;
            break;
        case HASH_ALG_SHA2_512:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA512;
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_create_ec_public_key_asset: invalid hash algorithm!\n");
            return -1;
    }

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_create_ec_public_key_asset, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_public_key_asset: xSemaphoreTake() failed!\n");
        return -1;
    }

    switch (ec_public_key->curveID) 
    {
#if defined(SUPPORT_EC_P256)
        case EC_KEY_CURVE_NIST_P256:
            if (HASH_ALG_SHA2_256 != hash_alg) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: curve p256 can only be used with \
                        SHA256 hash!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_create_ec_p256_public_key_asset(&(public_key_data.p256), ec_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: crypto_create_ec_p256_public_key_asset() \
                        failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.p256);
            public_key_data_size = sizeof(public_key_data.p256);
            break;
#endif
#if defined(SUPPORT_EC_P384)
        case EC_KEY_CURVE_NIST_P384:
            if (HASH_ALG_SHA2_256 != hash_alg && HASH_ALG_SHA2_384 != hash_alg) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: curve p384 can only be used with SHA256 \
                        or SHA384 hash!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_create_ec_p384_public_key_asset(&(public_key_data.p384), ec_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: crypto_create_ec_p384_public_key_asset() \
                        failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.p384);
            public_key_data_size = sizeof(public_key_data.p384);
            break;
#endif
#if defined(SUPPORT_EC_P521)
        case EC_KEY_CURVE_NIST_P521:
            if (0 != crypto_create_ec_p521_public_key_asset(&(public_key_data.p521), ec_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: crypto_create_ec_p521_public_key_asset() \
                        failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.p521);
            public_key_data_size = sizeof(public_key_data.p521);
            break;
#endif
#if defined(SUPPORT_EC_CURVE25519)
        case EC_KEY_CURVE_CURVE25519:
            /* todo: verify if this is true */
            if (HASH_ALG_SHA2_256 != hash_alg) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: curve 25519 can only be used with SHA256 \
                        hash!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_create_ec_curve25519_public_key_asset(&(public_key_data.curve25519),
                                                                ec_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: \
                       crypto_create_ec_curve25519_public_key_asset() failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.curve25519);
            public_key_data_size = sizeof(public_key_data.curve25519);
            break;
#endif
#if defined(SUPPORT_EC_EDWARDS25519)
        case EC_KEY_CURVE_EDWARDS25519:
            if (HASH_ALG_SHA2_512 != hash_alg) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: curve ed25519 can only be used with \
                        SHA512 hash!\n");
                return -1;
            }
            if (0 != crypto_create_ec_edwards25519_public_key_asset(&(public_key_data.edwards25519),
                                                                    ec_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_ec_public_key_asset: \
                        crypto_create_ec_edwards25519_public_key_asset() failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.edwards25519);
            public_key_data_size = sizeof(public_key_data.edwards25519);
            break;
#endif
            default:
            MESSAGE_ERROR_DEBUG("crypto_create_ec_public_key_asset: invalid curve_id!\n");
            rv = -1;
            goto DONE;
    }

    /* MESSAGE_INFO_DEBUG("crypto_create_ec_public_key_asset: key_data_size=0x%x\n",
                             public_key_data_size); */
    public_key_asset_other_settings.DataLength = public_key_data_size & 0x3FFu;
    if (0 != vaultip_drv_asset_create(get_rom_identity(), public_key_asset_policy.lo,
                                      public_key_asset_policy.hi, public_key_asset_other_settings,
                                      0, &asset_id)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_public_key_asset: vaultip_drv_asset_create() failed!\n");
        rv = -1;
        goto DONE;
    }
    /* MESSAGE_INFO_DEBUG("crypto_create_ec_public_key_asset: asset created.\n"); */
    if (0 != vaultip_drv_asset_load_plaintext(get_rom_identity(), asset_id, public_key_data_ptr,
                                              public_key_data_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_ec_public_key_asset: vaultip_drv_asset_load_plaintext() failed!\n");
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), asset_id)) 
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_create_ec_public_key_asset: vaultip_drv_asset_delete() failed!\n");
        }
        rv = -1;
        goto DONE;
    }
    /* MESSAGE_INFO_DEBUG("crypto_create_ec_public_key_asset: asset loaded.\n");
     MESSAGE_INFO_DEBUG("Key asset data @ %p:", public_key_data_ptr);
     dump_asset(public_key_data_ptr, public_key_data_size); */

    *ec_public_key_asset_id = asset_id;
    rv = 0;

DONE:
    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_create_ec_public_key_asset)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_ec_public_key_asset: xSemaphoreGive() failed!\n");
        rv = -1;
    }
    return rv;
}

static int crypto_ecdsa_verify(const PUBLIC_KEY_EC_t *ecdsa_public_key,
                               const PUBLIC_SIGNATURE_t *signature, uint32_t temp_hash_asset_id,
                               const void *data, size_t data_size, uint32_t total_data_size)
{
    int rv;
    uint32_t public_key_parameters_asset_id = 0;
    uint32_t public_key_asset_id = 0;
    const void *signature_data_ptr;
    uint32_t signature_data_size;

    static union 
    {
#if defined(SUPPORT_EC_P256)
        VAULTIP_SIGNATURE_EC_P256_t p256;
#endif
#if defined(SUPPORT_EC_P384)
        VAULTIP_SIGNATURE_EC_P384_t p384;
#endif
#if defined(SUPPORT_EC_P521)
        VAULTIP_SIGNATURE_EC_P521_t p521;
#endif
#if defined(SUPPORT_EC_CURVE25519) || defined(SUPPORT_EC_EDWARDS25519)
        VAULTIP_SIGNATURE_EC_25519_t c25519;
#endif
    } signature_data;

    if (NULL == signature) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: invalid arguments!\n");
        return -1;
    }

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_ecdsa_verify, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: xSemaphoreTake() failed!\n");
        return -1;
    }

    switch (signature->ec.curveID) 
    {
#if defined(SUPPORT_EC_P256)
        case EC_KEY_CURVE_NIST_P256:
            if (0 != crypto_write_subvector_256(&(signature_data.p256.r), 2, 0, signature->ec.r,
                                                signature->ec.rSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_256(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_write_subvector_256(&(signature_data.p256.s), 2, 1, signature->ec.s,
                                                signature->ec.sSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_256(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.p256);
            signature_data_size = sizeof(signature_data.p256);
            break;
#endif
#if defined(SUPPORT_EC_P384)
        case EC_KEY_CURVE_NIST_P384:
            if (0 != crypto_write_subvector_384(&(signature_data.p384.r), 2, 0, signature->ec.r,
                                                signature->ec.rSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_384(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_write_subvector_384(&(signature_data.p384.s), 2, 1, signature->ec.s,
                                                signature->ec.sSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_384(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.p384);
            signature_data_size = sizeof(signature_data.p384);
            break;
#endif
#if defined(SUPPORT_EC_P521)
        case EC_KEY_CURVE_NIST_P521:
            if (0 != crypto_write_subvector_521(&(signature_data.p521.r), 2, 0, signature->ec.r,
                                                signature->ec.rSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_521(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != crypto_write_subvector_521(&(signature_data.p521.s), 2, 1, signature->ec.s,
                                                signature->ec.sSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_521(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.p521);
            signature_data_size = sizeof(signature_data.p521);
            break;
#endif
#if defined(SUPPORT_EC_CURVE25519)
        case EC_KEY_CURVE_CURVE25519:
#endif
#if defined(SUPPORT_EC_EDWARDS25519)
        case EC_KEY_CURVE_EDWARDS25519:
#endif
#if defined(SUPPORT_EC_CURVE25519) || defined(SUPPORT_EC_EDWARDS25519)
        if (0 != crypto_write_subvector_256(&(signature_data.c25519.r), 1, 0, signature->ec.r,
                                            signature->ec.rSize)) 
        {
            MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_write_subvector_256(r) failed!\n");
            rv = -1;
            goto DONE;
        }
        signature_data_ptr = &(signature_data.c25519);
        signature_data_size = sizeof(signature_data.c25519);
        break;
#endif
        default:
            MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: invalid curve_id!\n");
            rv = -1;
            goto DONE;
    }

    /* create the ecdsa parameters asset */
    if (0 != crypto_create_ec_parameters_asset(ecdsa_public_key->curveID,
                                               &public_key_parameters_asset_id)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_create_ec_parameters_asset() failed!\n");
        rv = -1;
        goto DONE;
    }
    MESSAGE_INFO_DEBUG("crypto_ecdsa_verify: ec parameters created and loaded.\n");

    /* create the ecdsa public key asset */
    if (0 != crypto_create_ec_public_key_asset(signature->hashAlg, ecdsa_public_key,
                                               &public_key_asset_id)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: crypto_create_ec_public_key_asset() failed!\n");
        rv = -1;
        goto DONE;
    }
    /* MESSAGE_INFO_DEBUG("crypto_ecdsa_verify: ec public key created and loaded.\n");

       MESSAGE_INFO_DEBUG("Signature data @ %p:", signature_data_ptr);
                         dump_asset(signature_data_ptr, signature_data_size);

       MESSAGE_INFO_DEBUG("Message data addr: %p, size: 0x%x\n", data, data_size);

     verify the signature */
    if (0 != vaultip_drv_public_key_ecdsa_verify(
                 ecdsa_public_key->curveID, get_rom_identity(), public_key_asset_id,
                 public_key_parameters_asset_id, temp_hash_asset_id, data, (uint32_t)data_size,
                 total_data_size, signature_data_ptr, signature_data_size)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: vaultip_drv_public_key_ecdsa_verify() \
                                failed!\n");
        rv = -1;
        goto DONE;
    }

    rv = 0;

DONE:

    /* delete the public key asset */
    if (0 != public_key_parameters_asset_id)
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), public_key_parameters_asset_id)) 
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_ecdsa_verify: vaultip_drv_asset_delete(public_key_parameters_asset_id) \
                     failed!\n");
        }
    }
    if (0 != public_key_asset_id) 
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), public_key_asset_id)) 
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_ecdsa_verify: vaultip_drv_asset_delete(public_key_asset_id) failed!\n");
        }
    }

    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_ecdsa_verify)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_ecdsa_verify: xSemaphoreGive() failed!\n");
        rv = -1;
    }

    return rv;
}

#if defined(SUPPORT_RSA_2048)
static int crypto_create_rsa_2048_public_key_asset(VAULTIP_PUBLIC_KEY_RSA_2048_t *rsa_2048,
                                                   const PUBLIC_KEY_RSA_t *public_key)
{
    if (NULL == rsa_2048 || NULL == public_key || 2048 != public_key->keySize) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_2048_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy modulus */
    if (0 != crypto_write_subvector_2048(&(rsa_2048->modulus), 2, 0, public_key->pubMod,
                                         public_key->pubModSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_2048_public_key_asset: crypto_write_subvector_2048(modulus) failed!\n");
        return -1;
    }

    /* copy exponent */
    if (0 != crypto_write_subvector_64(&(rsa_2048->exponent), 2, 1, public_key->pubExp,
                                       public_key->pubExpSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_2048_public_key_asset: crypto_write_subvector_64(exponent) failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_RSA_3072)
static int crypto_create_rsa_3072_public_key_asset(VAULTIP_PUBLIC_KEY_RSA_3072_t *rsa_3072,
                                                   const PUBLIC_KEY_RSA_t *public_key)
{
    if (NULL == rsa_3072 || NULL == public_key || 3072 != public_key->keySize) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_3072_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy modulus */
    if (0 != crypto_write_subvector_3072(&(rsa_3072->modulus), 2, 0, public_key->pubMod,
                                         public_key->pubModSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_3072_public_key_asset: crypto_write_subvector_3072(modulus) \
                 failed!\n");
        return -1;
    }

    /* copy exponent */
    if (0 != crypto_write_subvector_64(&(rsa_3072->exponent), 2, 1, public_key->pubExp,
                                       public_key->pubExpSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_3072_public_key_asset: crypto_write_subvector_64(exponent) \
                failed!\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(SUPPORT_RSA_4096)
static int crypto_create_rsa_4096_public_key_asset(VAULTIP_PUBLIC_KEY_RSA_4096_t *rsa_4096,
                                                   const PUBLIC_KEY_RSA_t *public_key)
{
    if (NULL == rsa_4096 || NULL == public_key || 4096 != public_key->keySize) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_4096_public_key_asset: invalid arguments!\n");
        return -1;
    }

    /* copy modulus */
    if (0 != crypto_write_subvector_4096(&(rsa_4096->modulus), 2, 0, public_key->pubMod,
                                         public_key->pubModSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_4096_public_key_asset: crypto_write_subvector_4096(modulus) \
                failed!\n");
        return -1;
    }

    /* copy exponent */
    if (0 != crypto_write_subvector_64(&(rsa_4096->exponent), 2, 1, public_key->pubExp,
                                       public_key->pubExpSize)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_4096_public_key_asset: crypto_write_subvector_64(exponent) \
                failed!\n");
        return -1;
    }

    return 0;
}
#endif

static int crypto_create_rsa_public_key_asset(HASH_ALG_t hash_alg,
                                              const PUBLIC_KEY_RSA_t *rsa_public_key,
                                              uint32_t *rsa_public_key_asset_id)
{
    int rv;
    static union 
    {
#if defined(SUPPORT_RSA_2048)
        VAULTIP_PUBLIC_KEY_RSA_2048_t rsa2048;
#endif
#if defined(SUPPORT_RSA_3072)
        VAULTIP_PUBLIC_KEY_RSA_3072_t rsa3072;
#endif
#if defined(SUPPORT_RSA_4096)
        VAULTIP_PUBLIC_KEY_RSA_4096_t rsa4096;
#endif
    } public_key_data;
    const void *public_key_data_ptr;
    uint32_t public_key_data_size;
    uint32_t asset_id;

    ASSET_POLICY_t public_key_asset_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_PUBLIC_KEY | VAL_POLICY_PK_RSA_PSS_SIGN,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t public_key_asset_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };

    if (NULL == rsa_public_key || NULL == rsa_public_key_asset_id) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: invalid arguments!\n");
        return -1;
    }

    switch (hash_alg) 
    {
        case HASH_ALG_SHA2_256:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA256;
            break;
        case HASH_ALG_SHA2_384:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA384;
            break;
        case HASH_ALG_SHA2_512:
            public_key_asset_policy.u64 |= VAL_POLICY_SHA512;
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: invalid hash algorithm!\n");
            return -1;
    }

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_create_rsa_public_key_asset, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: xSemaphoreTake() failed!\n");
        return -1;
    }

    switch (rsa_public_key->keySize) 
    {
#if defined(SUPPORT_RSA_2048)
        case 2048:
            if (0 !=
                crypto_create_rsa_2048_public_key_asset(&(public_key_data.rsa2048), rsa_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_rsa_public_key_asset: \
                    crypto_create_rsa_2048_public_key_asset() failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.rsa2048);
            public_key_data_size = sizeof(public_key_data.rsa2048);
            break;
#endif
#if defined(SUPPORT_RSA_3072)
        case 3072:
            if (0 !=
                crypto_create_rsa_3072_public_key_asset(&(public_key_data.rsa3072), rsa_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_rsa_public_key_asset: \
                    crypto_create_rsa_3072_public_key_asset() failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.rsa3072);
            public_key_data_size = sizeof(public_key_data.rsa3072);
            break;
#endif
#if defined(SUPPORT_RSA_4096)
        case 4096:
            if (0 !=
                crypto_create_rsa_4096_public_key_asset(&(public_key_data.rsa4096), rsa_public_key)) 
            {
                MESSAGE_ERROR_DEBUG(
                    "crypto_create_rsa_public_key_asset: crypto_create_rsa_4096_public_key_asset()\
                        failed!\n");
                rv = -1;
                goto DONE;
            }
            public_key_data_ptr = &(public_key_data.rsa4096);
            public_key_data_size = sizeof(public_key_data.rsa4096);
            break;
#endif
        default:
            MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: invalid key size!\n");
            rv = -1;
            goto DONE;
    }

    MESSAGE_INFO_DEBUG("crypto_create_rsa_public_key_asset: asset length=0x%x\n",
                       public_key_data_size);
    public_key_asset_other_settings.DataLength = public_key_data_size & 0x3FFu;
    if (0 != vaultip_drv_asset_create(get_rom_identity(), public_key_asset_policy.lo,
                                      public_key_asset_policy.hi, public_key_asset_other_settings,
                                      0, &asset_id)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_public_key_asset: vaultip_drv_asset_create() failed!\n");
        rv = -1;
        goto DONE;
    }

    MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: asset created, assetid=0x%x\n",
                        asset_id);
    if (0 != vaultip_drv_asset_load_plaintext(get_rom_identity(), asset_id, public_key_data_ptr,
                                              public_key_data_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_create_rsa_public_key_asset: vaultip_drv_asset_load_plaintext() failed!\n");
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), asset_id)) 
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_create_rsa_public_key_asset: vaultip_drv_asset_delete() failed!\n");
        }
        rv = -1;
        goto DONE;
    }
    /* MESSAGE_INFO_DEBUG("crypto_create_rsa_public_key_asset: asset loaded\n");
          dump_asset(public_key_data_ptr, public_key_data_size); */

    *rsa_public_key_asset_id = asset_id;
    rv = 0;

DONE:
    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_create_rsa_public_key_asset)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_create_rsa_public_key_asset: xSemaphoreGivee() failed!\n");
        rv = -1;
    }
    return rv;
}

static int crypto_rsa_verify(const PUBLIC_KEY_RSA_t *rsa_public_key,
                             const PUBLIC_SIGNATURE_t *signature, uint32_t temp_hash_asset_id,
                             const void *data, size_t data_size, uint32_t total_data_size)
{
    int rv;
    uint32_t public_key_asset_id = 0;
    uint32_t salt_length;

    const void *signature_data_ptr;
    uint32_t signature_data_size;

    static union 
    {
#if defined(SUPPORT_RSA_2048)
        VAULTIP_SIGNATURE_RSA_2048_t rsa2048;
#endif
#if defined(SUPPORT_RSA_3072)
        VAULTIP_SIGNATURE_RSA_3072_t rsa3072;
#endif
#if defined(SUPPORT_RSA_4096)
        VAULTIP_SIGNATURE_RSA_4096_t rsa4096;
#endif
    } signature_data;

    if (NULL == signature) 
    {
        MESSAGE_ERROR_DEBUG("crypto_rsa_verify: invalid arguments!\n");
        return -1;
    }

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_rsa_verify, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_rsa_verify: xSemaphoreTake() failed!\n");
        return -1;
    }

    switch (signature->rsa.keySize) 
    {
#if defined(SUPPORT_RSA_2048)
        case 2048:
            if (0 != crypto_write_subvector_2048(&(signature_data.rsa2048.s), 1, 0,
                                                signature->rsa.signature, signature->rsa.sigSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_rsa_verify: crypto_write_subvector_2048(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.rsa2048);
            signature_data_size = sizeof(signature_data.rsa2048);
            break;
#endif
#if defined(SUPPORT_RSA_3072)
        case 3072:
            if (0 != crypto_write_subvector_3072(&(signature_data.rsa3072.s), 1, 0,
                                                signature->rsa.signature, signature->rsa.sigSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_rsa_verify: crypto_write_subvector_3072(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.rsa3072);
            signature_data_size = sizeof(signature_data.rsa3072);
            break;
#endif
#if defined(SUPPORT_RSA_4096)
        case 4096:
            if (0 != crypto_write_subvector_4096(&(signature_data.rsa4096.s), 1, 0,
                                                signature->rsa.signature, signature->rsa.sigSize)) 
            {
                MESSAGE_ERROR_DEBUG("crypto_rsa_verify: crypto_write_subvector_4096(r) failed!\n");
                rv = -1;
                goto DONE;
            }
            signature_data_ptr = &(signature_data.rsa4096);
            signature_data_size = sizeof(signature_data.rsa4096);
            break;
#endif
        default:
            MESSAGE_ERROR_DEBUG("crypto_rsa_verify: invalid key_size!\n");
            rv = -1;
            goto DONE;
    }

    switch (signature->hashAlg) 
    {
        case HASH_ALG_SHA2_256:
            salt_length = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
            salt_length = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
            salt_length = 512 / 8;
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_rsa_verify: invalid hash alg!\n");
            rv = -1;
            goto DONE;
    }

    /* create the rsa public key asset */
    if (0 != crypto_create_rsa_public_key_asset(signature->hashAlg, rsa_public_key,
                                                &public_key_asset_id)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_rsa_verify: crypto_create_rsa_public_key_asset() failed!\n");
        rv = -1;
        goto DONE;
    }

    /* verify the signature */
    if (0 != vaultip_drv_public_key_rsa_pss_verify(
                 rsa_public_key->keySize, get_rom_identity(), public_key_asset_id,
                 temp_hash_asset_id, data, (uint32_t)data_size, total_data_size, 
                 signature_data_ptr, signature_data_size, salt_length)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_rsa_verify: vaultip_drv_public_key_rsa_pss_verify() \
             failed!\n");
        rv = -1;
        goto DONE;
    }

    rv = 0;

DONE:

    /* delete the public key asset */
    if (0 != public_key_asset_id) 
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), public_key_asset_id)) 
        {
            MESSAGE_ERROR_DEBUG(
                "crypto_rsa_verify: vaultip_drv_asset_delete(public_key_asset_id) failed!\n");
        }
    }

    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_rsa_verify)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_rsa_verify: xSemaphoreGive() failed!\n");
        rv = -1;
    }

    return rv;
}

int crypto_verify_pk_signature(const PUBLIC_KEY_t *public_key, const PUBLIC_SIGNATURE_t *signature,
                               const void *data, size_t data_size)
{
    int rv;
    ASSET_POLICY_t temp_digest_asset_policy = (ASSET_POLICY_t)
    {
        .u64 = VAL_POLICY_TEMP_MAC,
    };
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t temp_digest_asset_other_settings =
        (VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t)
        {
            .LifetimeUse = VAL_ASSET_LIFETIME_INFINITE,
        };
    uint32_t temp_digest_asset_id = 0;
    uint32_t prehash_length = 0;
    uint32_t total_message_length = (uint32_t)data_size;

    if (NULL == data || 0 == data_size) 
    {
        MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: invalid arguments!\n");
        return -1;
    }

    switch (public_key->keyType) 
    {
        case PUBLIC_KEY_TYPE_EC:
        case PUBLIC_KEY_TYPE_RSA:
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: not supported key type!\n");
            return -1;
    }
    if (signature->keyType != public_key->keyType) 
    {
        MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: key type does not match signature!\n");
        return -1;
    }
    if (data_size > 4096) 
    {
        /* prehash the first part of the data up to the last 4096 bytes (or less) */

        prehash_length = (uint32_t)(data_size - 4096u);
        prehash_length = (prehash_length + 0x7Fu) & 0xFFFFFF80;
        MESSAGE_INFO_DEBUG("crypto_verify_pk_signature: prehash_length=0x%x, \n", prehash_length);

        switch (signature->hashAlg) 
        {
            case HASH_ALG_SHA2_256:
                temp_digest_asset_policy.u64 |= VAL_POLICY_SHA256;
                temp_digest_asset_other_settings.DataLength = 256 / 8;
                break;
            case HASH_ALG_SHA2_384:
                temp_digest_asset_policy.u64 |= VAL_POLICY_SHA384;
                temp_digest_asset_other_settings.DataLength = 384 / 8;
                break;
            case HASH_ALG_SHA2_512:
                temp_digest_asset_policy.u64 |= VAL_POLICY_SHA512;
                temp_digest_asset_other_settings.DataLength = 512 / 8;
                break;
            default:
                MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: invalid hash algorithm!\n");
                return -1;
        }

        /* create temp_digest asset */
        if (0 != vaultip_drv_asset_create(
                     get_rom_identity(), temp_digest_asset_policy.lo, temp_digest_asset_policy.hi,
                     temp_digest_asset_other_settings, 0, &temp_digest_asset_id)) 
        {
            MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: vaultip_drv_asset_create() \
                failed!\n");
            return -1;
        }

        /* pre-hash the data */
        if (0 != vaultip_drv_hash_update(get_rom_identity(), signature->hashAlg,
                                         temp_digest_asset_id, data, prehash_length, true)) 
        {
            MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: vaultip_drv_hash_update() failed!\n");
            rv = -1;
            goto DONE;
        }

        data = ((const uint8_t *)data) + prehash_length;
        data_size -= prehash_length;
    }

    MESSAGE_INFO_DEBUG(
        "crypto_verify_pk_signature: message_length=0x%lx, total_message_length=0x%x\n", data_size,
        total_message_length);
    switch (public_key->keyType) 
    {
        case PUBLIC_KEY_TYPE_EC:
            rv = crypto_ecdsa_verify(&(public_key->ec), signature, temp_digest_asset_id, data,
                                    data_size, total_message_length);
            break;
        case PUBLIC_KEY_TYPE_RSA:
            rv = crypto_rsa_verify(&(public_key->rsa), signature, temp_digest_asset_id, data,
                                data_size, total_message_length);
            break;
        default:
            MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: not supported key type!\n");
            rv = -1;
    }

DONE:
    if (0 != temp_digest_asset_id) 
    {
        if (0 != vaultip_drv_asset_delete(get_rom_identity(), temp_digest_asset_id)) 
        {
            MESSAGE_ERROR_DEBUG("crypto_verify_pk_signature: vaultip_drv_asset_delete() \
                failed!\n");
        }
    }
    return rv;
}

int crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                         size_t buffer_size, uint32_t *hash_size)
{
    uint32_t asset_id;
    uint32_t asset_size;

    if (NULL == hash_size) 
    {
        return -1;
    }

    if ((NULL == buffer && 0 != buffer_size) || (NULL != buffer && 0 == buffer_size)) 
    {
        return -1;
    }

    if (0 != vaultip_drv_static_asset_search(get_rom_identity(), static_asset_id, &asset_id,
                                             &asset_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "load_public_key_hash_from_otp: vaultip_drv_static_asset_search(%u) failed!\n",
            static_asset_id);
        return -1;
    }

    *hash_size = asset_size;
    if (NULL == buffer && 0 == buffer_size) 
    {
        return 0;
    }

    if (asset_size > buffer_size) 
    {
        return -1;
    }

    if (0 != vaultip_drv_public_data_read(get_rom_identity(), asset_id, buffer,
                                          (uint32_t)buffer_size, hash_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "load_public_key_hash_from_otp: vaultip_drv_public_data_read() failed!\n");
        return -1;
    }
    return 0;
}

int crypto_load_monotonic_counter_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                           size_t buffer_size, uint32_t *counter_size)
{
    uint32_t asset_id;
    uint32_t asset_size;

    if (NULL == counter_size) 
    {
        return -1;
    }

    if ((NULL == buffer && 0 != buffer_size) || (NULL != buffer && 0 == buffer_size)) 
    {
        return -1;
    }

    if (0 != vaultip_drv_static_asset_search(get_rom_identity(), static_asset_id, &asset_id,
                                             &asset_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_load_monotonic_counter_from_otp: vaultip_drv_static_asset_search(%u) \
                failed!\n",
            static_asset_id);
        return -1;
    }

    *counter_size = asset_size;
    if (NULL == buffer && 0 == buffer_size) 
    {
        return 0;
    }

    if (asset_size > buffer_size) 
    {
        return -1;
    }

    if (0 != vaultip_drv_monotonic_counter_read(get_rom_identity(), asset_id, buffer,
                                                (uint32_t)buffer_size, counter_size)) 
    {
        MESSAGE_ERROR_DEBUG(
            "crypto_load_monotonic_counter_from_otp: vaultip_drv_monotonic_counter_read() \
                 failed!\n");
        return -1;
    }
    return 0;
}

#define MAXIMUM_COUNTER_WORDS 8
#define MAXIMUM_COUNTER_BYTES (MAXIMUM_COUNTER_WORDS * 8)

static int crypto_get_monotonic_counter_value(VAULTIP_STATIC_ASSET_ID_t static_asset_id,
                                              uint32_t *value)
{
    int rv = 0;

    static union 
    {
        uint8_t u8[MAXIMUM_COUNTER_BYTES];
        uint64_t u64[MAXIMUM_COUNTER_WORDS];
    } buffer;
    uint32_t counter_size;
    uint32_t n, w, count;

    if (NULL == value) 
    {
        return -1;
    }

    if (pdPASS != xSemaphoreTake(gs_mutex_crypto_get_monotonic_counter_value, portMAX_DELAY)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_get_monotonic_counter_value: xSemaphoreTake() failed!\n");
        return -1;
    }

    if (0 != crypto_load_monotonic_counter_from_otp(static_asset_id, buffer.u8,
                                                    MAXIMUM_COUNTER_BYTES, &counter_size)) 
    {
        *value = 0;
    }
    else
    {
        count = 0;
        for (w = 0; w < MAXIMUM_COUNTER_WORDS; w++) 
        {
            for (n = 0; n < 64; n++) 
            {
                if (0 != (buffer.u64[w] & 1)) 
                {
                    count++;
                }
                buffer.u64[w] = buffer.u64[w] >> 1u;
            }
        }

        *value = count;
    }

    if (pdPASS != xSemaphoreGive(gs_mutex_crypto_get_monotonic_counter_value)) 
    {
        MESSAGE_ERROR_DEBUG("crypto_get_monotonic_counter_value: xSemaphoreGive() failed!\n");
        rv = -1;
    }

    return rv;
}

int crypto_get_sp_bl1_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(VAULTIP_STATIC_ASSET_SP_BL1_REVOCATION_COUNTER,
                                              value);
}

int crypto_get_pcie_cfg_data_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(
                    VAULTIP_STATIC_ASSET_PCIE_CFG_DATA_REVOCATION_COUNTER, value);
}

int crypto_get_sp_bl2_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(VAULTIP_STATIC_ASSET_SP_BL2_REVOCATION_COUNTER,
                                              value);
}

int crypto_get_machine_minion_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(
        VAULTIP_STATIC_ASSET_MACHINE_MINION_REVOCATION_COUNTER, value);
}

int crypto_get_master_minion_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(
                    VAULTIP_STATIC_ASSET_MASTER_MINION_REVOCATION_COUNTER, value);
}

int crypto_get_worker_minion_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(
                    VAULTIP_STATIC_ASSET_WORKER_MINION_REVOCATION_COUNTER, value);
}

int crypto_get_maxion_bl1_monotonic_counter_value(uint32_t *value)
{
    return crypto_get_monotonic_counter_value(VAULTIP_STATIC_ASSET_MAXION_BL1_REVOCATION_COUNTER,
                                              value);
}
