/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef __VAULTIP_SW_ASSET_H__
#define __VAULTIP_SW_ASSET_H__

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  The following code was copied from 
//  926-130000-200_DDK-130_v2.3/DDK-130_v2.3/Integration/Adapter_VAL/incl/api_val_asset.h
//  which Esperanto has received under both GPL and non-GPL license
//
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


#define API_VAL_ASSET_LIFETIME_MANAGEMENT

#define VAL_ASSETID_INVALID  0


/** The maximum value for the AssetSize parameter used in this API.\n
 *  Note: DH/DSA Domain parameters are taken as maximum reference */
#define VAL_ASSET_SIZE_MAX   (((3 * 32) + 3072 + 3072 + 256) / 8)


/** The maximum value for the AssetNumber parameter used in this API. */
#define VAL_ASSET_NUMBER_MAX   40

/** The maximum value for the PolicyNumber parameter used in this API. */
#define VAL_POLICY_NUMBER_MAX   30


/** The minimum length of the label used in the Key Derivation Function (KDF). */
#define VAL_KDF_LABEL_MIN_SIZE       53

/** The maximum length of the label used in the Key Derivation Function (KDF). */
#define VAL_KDF_LABEL_MAX_SIZE       (224 - 20)


/** The minimum length of the label used in the AES-SIV keyblob export/import. */
#define VAL_KEYBLOB_AAD_MIN_SIZE     33

/** The maximum length of the label used in the AES-SIV keyblob export/import. */
#define VAL_KEYBLOB_AAD_MAX_SIZE     224


/** The expected size of an AES-SIV keyblob.\n
 *  Note: AssetSize is the size of the Asset in octects (bytes). */
#ifdef API_VAL_ASSET_LIFETIME_MANAGEMENT
#define VAL_KEYBLOB_SIZE(AssetSize) ((128/8) + AssetSize + (32/8))
#else
#define VAL_KEYBLOB_SIZE(AssetSize) ((128/8) + AssetSize)
#endif
#define VAL_OTP_KEYBLOB_SIZE(AssetSize) ((128/8) + AssetSize)

typedef enum
{
    VAL_ASSET_LIFETIME_INFINITE = 0,        /* Not used */
#ifdef API_VAL_ASSET_LIFETIME_MANAGEMENT
    VAL_ASSET_LIFETIME_SECONDS,
    VAL_ASSET_LIFETIME_MILLISECONDS,
    VAL_ASSET_LIFETIME_COUNTER,
#endif
} ValAssetLifetimeUse_t;

/** Asset Policy is a bitmask that defines the type and use of an Asset.\n
 *  The VAL_POLICY_* defines should be used to construct the Asset Policy
 *  bitmask.\n
 *  Note: The Asset Policy limitations depend on the implementation. So, please
 *        check the documentation. */
typedef uint64_t ValPolicyMask_t;

/** Asset policies related to hash/HMAC and CMAC algorithms */
#define VAL_POLICY_SHA1                       0x0000000000000001ULL
#define VAL_POLICY_SHA224                     0x0000000000000002ULL
#define VAL_POLICY_SHA256                     0x0000000000000004ULL
#define VAL_POLICY_SHA384                     0x0000000000000008ULL
#define VAL_POLICY_SHA512                     0x0000000000000010ULL
#define VAL_POLICY_CMAC                       0x0000000000000020ULL
#define VAL_POLICY_POLY1305                   0x0000000000000040ULL

/** Asset policies related to symmetric cipher algorithms */
#define VAL_POLICY_ALGO_CIPHER_MASK           0x0000000000000300ULL
#define VAL_POLICY_ALGO_CIPHER_AES            0x0000000000000100ULL
#define VAL_POLICY_ALGO_CIPHER_TRIPLE_DES     0x0000000000000200ULL
#define VAL_POLICY_ALGO_CIPHER_CHACHA20       0x0000000000002000ULL
#define VAL_POLICY_ALGO_CIPHER_SM4            0x0000000000004000ULL
#define VAL_POLICY_ALGO_CIPHER_ARIA           0x0000000000008000ULL

/** Asset policies related to symmetric cipher modes */
#define VAL_POLICY_MODE1                      0x0000000000010000ULL
#define VAL_POLICY_MODE2                      0x0000000000020000ULL
#define VAL_POLICY_MODE3                      0x0000000000040000ULL
#define VAL_POLICY_MODE4                      0x0000000000080000ULL
#define VAL_POLICY_MODE5                      0x0000000000100000ULL
#define VAL_POLICY_MODE6                      0x0000000000200000ULL
#define VAL_POLICY_MODE7                      0x0000000000400000ULL
#define VAL_POLICY_MODE8                      0x0000000000800000ULL
#define VAL_POLICY_MODE9                      0x0000000001000000ULL
#define VAL_POLICY_MODE10                     0x0000000002000000ULL

/** Asset policies specialized per symmetric cipher algorithm */
#define VAL_POLICY_AES_MODE_ECB               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE1)
#define VAL_POLICY_AES_MODE_CBC               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE2)
#define VAL_POLICY_AES_MODE_CTR               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE4)
#define VAL_POLICY_AES_MODE_CTR32             (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE4)
#define VAL_POLICY_AES_MODE_ICM               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE5)
#define VAL_POLICY_AES_MODE_CCM               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE7|VAL_POLICY_CMAC)
#define VAL_POLICY_AES_MODE_F8                (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE8)
#define VAL_POLICY_AES_MODE_XTS               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE9)
#define VAL_POLICY_AES_MODE_GCM               (VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE10)

#define VAL_POLICY_3DES_MODE_ECB              (VAL_POLICY_ALGO_CIPHER_TRIPLE_DES|VAL_POLICY_MODE1)
#define VAL_POLICY_3DES_MODE_CBC              (VAL_POLICY_ALGO_CIPHER_TRIPLE_DES|VAL_POLICY_MODE2)

#define VAL_POLICY_CHACHA20_ENCRYPT           (VAL_POLICY_ALGO_CIPHER_CHACHA20)
#define VAL_POLICY_CHACHA20_AEAD              (VAL_POLICY_ALGO_CIPHER_CHACHA20|VAL_POLICY_POLY1305)

#define VAL_POLICY_SM4_MODE_ECB               (VAL_POLICY_ALGO_CIPHER_SM4|VAL_POLICY_MODE1)
#define VAL_POLICY_SM4_MODE_CBC               (VAL_POLICY_ALGO_CIPHER_SM4|VAL_POLICY_MODE2)
#define VAL_POLICY_SM4_MODE_CTR               (VAL_POLICY_ALGO_CIPHER_SM4|VAL_POLICY_MODE4)

#define VAL_POLICY_ARIA_MODE_ECB              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE1)
#define VAL_POLICY_ARIA_MODE_CBC              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE2)
#define VAL_POLICY_ARIA_MODE_CTR              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE4)
#define VAL_POLICY_ARIA_MODE_CTR32            (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE4)
#define VAL_POLICY_ARIA_MODE_ICM              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE5)
#define VAL_POLICY_ARIA_MODE_CCM              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE7|VAL_POLICY_CMAC)
#define VAL_POLICY_ARIA_MODE_GCM              (VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_MODE10)

/** Asset policies related to Algorithm/cipher/MAC operations */
#define VAL_POLICY_MAC_GENERATE               0x0000000004000000ULL
#define VAL_POLICY_MAC_VERIFY                 0x0000000008000000ULL
#define VAL_POLICY_ENCRYPT                    0x0000000010000000ULL
#define VAL_POLICY_DECRYPT                    0x0000000020000000ULL

/** Asset policies related to temporary values
 *  Note that the VAL_POLICY_TEMP_MAC should be used for intermediate
 *  hash digest as well. */
#define VAL_POLICY_TEMP_IV                    0x0001000000000000ULL
#define VAL_POLICY_TEMP_COUNTER               0x0002000000000000ULL
#define VAL_POLICY_TEMP_MAC                   0x0004000000000000ULL
#define VAL_POLICY_TEMP_AUTH_STATE            0x0010000000000000ULL

/** Asset policy related to monotonic counter */
#define VAL_POLICY_MONOTONIC                  0x0000000100000000ULL

/** Asset policies related to key derive functionality */
#define VAL_POLICY_TRUSTED_ROOT_KEY           0x0000000200000000ULL
#define VAL_POLICY_TRUSTED_KEY_DERIVE         0x0000000400000000ULL
#define VAL_POLICY_KEY_DERIVE                 0x0000000800000000ULL

/** Asset policies related to AES key wrap functionality\n
 *  Note: Must be combined with operations bits */
#define VAL_POLICY_TRUSTED_WRAP               0x0000001000000000ULL
#define VAL_POLICY_AES_WRAP                   0x0000002000000000ULL

/** Asset policies related to PK operations */
#define VAL_POLICY_PUBLIC_KEY                 0x0000000080000000ULL
#define VAL_POLICY_PK_RSA_OAEP_WRAP           0x0000004000000000ULL
#define VAL_POLICY_PK_RSA_PKCS1_SIGN          0x0000020000000000ULL
#define VAL_POLICY_PK_RSA_PSS_SIGN            0x0000040000000000ULL
#define VAL_POLICY_PK_DSA_SIGN                0x0000080000000000ULL
#define VAL_POLICY_PK_ECC_ECDSA_SIGN          0x0000100000000000ULL
#define VAL_POLICY_PK_DH_KEY                  0x0000200000000000ULL
#define VAL_POLICY_PK_ECDH_KEY                0x0000400000000000ULL
#define VAL_POLICY_PUBLIC_KEY_PARAM           0x0000800000000000ULL

#define VAL_POLICY_PK_ECC_ELGAMAL_ENC         (VAL_POLICY_PK_ECC_ECDSA_SIGN|VAL_POLICY_PK_ECDH_KEY)

/** Asset policies related to Authentication */
#define VAL_POLICY_EMMC_AUTH_KEY              0x0400000000000000ULL
#define VAL_POLICY_AUTH_KEY                   0x8000000000000000ULL

/** Asset policies related to the domain */
#define VAL_POLICY_SOURCE_NON_SECURE          0x0100000000000000ULL
#define VAL_POLICY_CROSS_DOMAIN               0x0200000000000000ULL

/** Asset policies related to general purpose data that can or must be used
 *  in an operation */
#define VAL_POLICY_PRIVATE_DATA               0x0800000000000000ULL
#define VAL_POLICY_PUBLIC_DATA                0x1000000000000000ULL

/** Asset policies related to export functionality */
#define VAL_POLICY_EXPORT                     0x2000000000000000ULL
#define VAL_POLICY_TRUSTED_EXPORT             0x4000000000000000ULL

/** Asset Number, refers to a non-volatile Asset number.\n
 *  The AssetNumber is system specific and is used for identifying a Static
 *  Asset or (Static) Public Data object. */
typedef uint16_t ValAssetNumber_t;

#endif
