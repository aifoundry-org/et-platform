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
/*! \file bl2_crypto.h
    \brief A C header that defines the crypto service's
    public interfaces. These interfaces provide services using which
    the host can use cryptographic functionality.
*/
/***********************************************************************/

#ifndef __BL2_CRYPTO_H__
#define __BL2_CRYPTO_H__

#include <stdlib.h>

#include "esperanto_signed_image_format/public_key_data.h"
#include "hal_vaultip_static_assets.h"

/*!
 * @struct typedef struct CRYPTO_HASH_CONTEXT_s
 * @brief context control block for crypto
 */
typedef struct CRYPTO_HASH_CONTEXT_s
{
    HASH_ALG_t hash_alg;
    uint32_t temp_digest_asset_id;
    bool init_done;
} CRYPTO_HASH_CONTEXT_t;

/*!
 * @struct typedef struct CRYPTO_AES_CONTEXT_s
 * @brief AES context control block for crypto
 */
typedef struct CRYPTO_AES_CONTEXT_s
{
    uint32_t aes_key_asset_id;
    uint8_t IV[16];
} CRYPTO_AES_CONTEXT_t;

/*! \fn int crypto_init(uint32_t vaultip_coid_set)
    \brief This function initializes the crypto subsystem
    \param vaultip_coid_set value of vaultip coid set
    \return Status indicating success or negative error
*/

int crypto_init(uint32_t vaultip_coid_set);

/*! \fn int crypto_verify_public_key_params(const PUBLIC_KEY_t *public_key)
    \brief This function validates public key paramteres with respect to
           crypto algorithm used
    \param public_key control block of public key
    \return Status indicating success or negative error
*/

int crypto_verify_public_key_params(const PUBLIC_KEY_t *public_key);

/*! \fn int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t *signature)
    \brief This function verify signature parameters which includes hash algorithm
           and key type with it's size,
    \param signature control block of public key
    \return Status indicating success or negative error
*/

int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t *signature);

/*! \fn int crypto_hash(HASH_ALG_t hash_alg, const void *msg, size_t msg_size, uint8_t *hash)
    \brief This function retrieves driver hash
    \param hash_alg hashing algorithm type
    \param msg pointer to message cb
    \param msg_size size of msg cb
    \param hash retrieved hash from driver
    \return Status indicating success or negative error
*/

int crypto_hash(HASH_ALG_t hash_alg, const void *msg, size_t msg_size, uint8_t *hash);

/*! \fn int crypto_hash_init(CRYPTO_HASH_CONTEXT_t *hash_context, HASH_ALG_t hash_alg)
    \brief This function initializes hashing algorithm
    \param hash_context pointer to hashing context
    \param hash_alg hashing algorithm type
    \return Status indicating success or negative error
*/

int crypto_hash_init(CRYPTO_HASH_CONTEXT_t *hash_context, HASH_ALG_t hash_alg);

/*! \fn int crypto_hash_update(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size)
    \brief This function updates hashing algorithm
    \param hash_context pointer to hashing context
    \param msg pointer to message cb
    \param msg_size size of msg cb
    \return Status indicating success or negative error
*/

int crypto_hash_update(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size);

/*! \fn int crypto_hash_final(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size,
                      size_t total_msg_length, uint8_t *hash)
    \brief This function finalizes hashing update
    \param hash_context pointer to hashing context
    \param msg pointer to message cb
    \param msg_size size of msg cb
    \param total_msg_length total length of message
    \param hash pointer to hash
    \return Status indicating success or negative error
*/

int crypto_hash_final(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size,
                      size_t total_msg_length, uint8_t *hash);

/*! \fn int crypto_hash_abort(CRYPTO_HASH_CONTEXT_t *hash_context)
    \brief This function aborts hashing update
    \param hash_context pointer to hashing context
    \return Status indicating success or negative error
*/

int crypto_hash_abort(CRYPTO_HASH_CONTEXT_t *hash_context);

/*! \fn int crypto_derive_kdk_key(const void *derivation_data, size_t derivation_data_size,
                          uint32_t *kdk_key)
    \brief This function creates and derives KDK key
    \param derivation_data pointer derivation data
    \param derivation_data_size pointer derivation data size
    \param kdk_key pointer to kdk key
    \return Status indicating success or negative error
*/

int crypto_derive_kdk_key(const void *derivation_data, size_t derivation_data_size,
                          uint32_t *kdk_key);

/*! \fn int crypto_derive_mac_key(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t kdk_key,
                          const void *derivation_data, size_t derivation_data_size,
                          uint32_t *mac_key)
    \brief This function creates and derives MAC key
    \param mac_alg MAC algorithm type
    \param kdk_key KDK key data
    \param derivation_data pointer derivation data
    \param derivation_data_size pointer derivation data size
    \param mac_key pointer to mac key
    \return Status indicating success or negative error
*/

int crypto_derive_mac_key(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t kdk_key,
                          const void *derivation_data, size_t derivation_data_size,
                          uint32_t *mac_key);

/*! \fn int crypto_derive_enc_key(uint32_t kdk_key, const void *derivation_data,
                          size_t derivation_data_size, uint32_t *enc_key)
    \brief This function creates and derives enc key
    \param kdk_key KDK key data
    \param derivation_data pointer derivation data
    \param derivation_data_size pointer derivation data size
    \param enc_key pointer to enc key
    \return Status indicating success or negative error
*/

int crypto_derive_enc_key(uint32_t kdk_key, const void *derivation_data,
                          size_t derivation_data_size, uint32_t *enc_key);

/*! \fn int crypto_delete_key(uint32_t key)
    \brief This function deletes encryption key
    \param key key to be deleted
    \return Status indicating success or negative error
*/

int crypto_delete_key(uint32_t key);

/*! \fn int crypto_mac_verify(ESPERANTO_MAC_TYPE_t mac_alg, const uint32_t mack_key, const void *data,
                      size_t data_size, const void *mac)
    \brief This function varifies MAC key
    \param mac_alg MAC algorithm type
    \param mack_key MAC encryption key
    \param data pointer to data buffer
    \param data_size size of data
    \param mac pointer to mac context
    \return Status indicating success or negative error
*/

int crypto_mac_verify(ESPERANTO_MAC_TYPE_t mac_alg, const uint32_t mack_key, const void *data,
                      size_t data_size, const void *mac);

/*! \fn int crypto_aes_decrypt_init(CRYPTO_AES_CONTEXT_t *aes_context, const uint32_t enck_key,
                            const uint8_t *IV)
    \brief This function initializes AES decryption engine
    \param aes_context pointer to AES context
    \param enck_key encryption key
    \param IV pointer IV
    \return Status indicating success or negative error
*/

int crypto_aes_decrypt_init(CRYPTO_AES_CONTEXT_t *aes_context, const uint32_t enck_key,
                            const uint8_t *IV);

/*! \fn int crypto_aes_decrypt_update(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size)
    \brief This function updates AES decryption engine
    \param aes_context pointer to AES context
    \param data data pointer
    \param data_size size of data
    \return Status indicating success or negative error
*/

int crypto_aes_decrypt_update(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size);

/*! \fn int crypto_aes_decrypt_update(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size)
    \brief This function finalizes AES decryption engine
    \param aes_context pointer to AES context
    \param data data pointer
    \param data_size size of data
    \return Status indicating success or negative error
*/

int crypto_aes_decrypt_final(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size,
                             uint8_t *IV);

/*! \fn int crypto_verify_pk_signature(const PUBLIC_KEY_t *public_key, const PUBLIC_SIGNATURE_t *signature,
                               const void *data, size_t data_size)
    \brief This function verifies pk signature
    \param public_key pointer to AES context
    \param signature public signature
    \param data data pointer
    \param data_size size of data
    \return Status indicating success or negative error
*/

int crypto_verify_pk_signature(const PUBLIC_KEY_t *public_key, const PUBLIC_SIGNATURE_t *signature,
                               const void *data, size_t data_size);

/*! \fn int crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                         size_t buffer_size, uint32_t *hash_size)
    \brief This function loads public key hash from otp
    \param static_asset_id asset id
    \param buffer data pointer
    \param buffer_size buffer size
    \param hash_size hash size
    \return Status indicating success or negative error
*/

int crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                         size_t buffer_size, uint32_t *hash_size);

/*! \fn int crypto_load_monotonic_counter_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                           size_t buffer_size, uint32_t *counter_size)
    \brief This function loads monotonic counter value from otp
    \param static_asset_id asset id
    \param buffer data pointer
    \param buffer_size buffer size
    \param counter_size returned counter value
    \return Status indicating success or negative error
*/

int crypto_load_monotonic_counter_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                           size_t buffer_size, uint32_t *counter_size);

/*! \fn int crypto_get_sp_bl1_monotonic_counter_value(uint32_t *value)
    \brief This function get bootloader 1 monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_sp_bl1_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_pcie_cfg_data_monotonic_counter_value(uint32_t *value)
    \brief This function get pcie configuration monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_pcie_cfg_data_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_sp_bl2_monotonic_counter_value(uint32_t *value)
    \brief This function get bootloader 2 monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_sp_bl2_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_machine_minion_monotonic_counter_value(uint32_t *value)
    \brief This function get machine minion monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_machine_minion_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_master_minion_monotonic_counter_value(uint32_t *value)
    \brief This function get master minion monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_master_minion_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_worker_minion_monotonic_counter_value(uint32_t *value)
    \brief This function get worker minion monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_worker_minion_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_maxion_bl1_monotonic_counter_value(uint32_t *value)
    \brief This function get maxion bl1 minion monotonic counter value
    \param value returned counter value
    \return Status indicating success or negative error
*/

int crypto_get_maxion_bl1_monotonic_counter_value(uint32_t *value);

/*! \fn int crypto_get_maxion_bl1_monotonic_counter_value(uint32_t *value)
    \brief This function returns ROM identity
    \param None
    \return Status indicating success or negative error
*/

uint32_t get_rom_identity(void);

#endif
