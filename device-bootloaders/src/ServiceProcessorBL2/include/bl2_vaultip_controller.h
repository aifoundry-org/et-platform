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
/*! \file bl2_vaultip_controller.h
    \brief A C header that defines the VaultIP controller's public interfaces.
*/
/***********************************************************************/

#ifndef __BL2_VAULTIP_CONTROLLER_H__
#define __BL2_VAULTIP_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

#include "vaultip_sw.h"
#include "vaultip_static_assets.h"

/*! \fn int vaultip_self_test(void)
    \brief This function starts a self test for vaultip controller
    \param None
    \return The function call status, pass/fail.
*/
int vaultip_self_test(void);

/*! \fn int vaultip_get_system_information(uint32_t identity,
                                   VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info)
    \brief This function return system information
    \param identity system ID
    \param system_info returned pointer to system information
    \return The function call status, pass/fail.
*/
int vaultip_get_system_information(uint32_t v,
                                   VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info);

/*! \fn int vaultip_register_read(uint32_t identity, bool incremental_read, uint32_t number,
                          const uint32_t *address, uint32_t *result)
    \brief This function reads from vaultip registers
    \param identity system ID
    \param incremental_read read a number of registers by incrementing address
    \param number number of registers to read
    \param address address of register to read
    \param result pointer to result containing value
    \return The function call status, pass/fail.
*/                                   
int vaultip_register_read(uint32_t identity, bool incremental_read, uint32_t number,
                          const uint32_t *address, uint32_t *result);

/*! \fn int vaultip_register_write(uint32_t identity, bool incremental_write, uint32_t number,
                           const uint32_t *mask, const uint32_t *address, const uint32_t *value)
    \brief This function writes data into vault ip registers
    \param identity system ID
    \param incremental_read read a number of registers by incrementing address
    \param number number of registers to read
    \param address address of register to read
    \param result pointer to result containing value
    \return The function call status, pass/fail.
*/
int vaultip_register_write(uint32_t identity, bool incremental_write, uint32_t number,
                           const uint32_t *mask, const uint32_t *address, const uint32_t *value);

/*! \fn int vaultip_trng_configuration(uint32_t identity)
    \brief This function used to configure trng
    \param identity system ID
    \return The function call status, pass/fail.
*/
int vaultip_trng_configuration(uint32_t identity);

/*! \fn int vaultip_trng_get_random_number(void *dst, uint16_t size, bool raw)
    \brief This function used get random number form vaultip
    \param identity system ID
    \return The function call status, pass/fail.
*/
int vaultip_trng_get_random_number(void *dst, uint16_t size, bool raw);

/*! \fn int vaultip_provision_huk(uint32_t coid)
    \brief This function reurns static asset huk
    \param identity system ID
    \return The function call status, pass/fail.
*/
int vaultip_provision_huk(uint32_t coid);

/*! \fn int vaultip_reset(uint32_t identity)
    \brief This function resets vaultip controller
    \param identity system ID
    \return The function call status, pass/fail.
*/
int vaultip_reset(uint32_t identity);

/*! \fn vaultip_hash(uint32_t identity, HASH_ALG_t hash_alg, const void *msg, size_t msg_size,
                 uint8_t *hash)
    \brief This function initializes vaultip hash
    \param identity system ID
    \param hash_alg hashing algoeithm to initialize
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param hash pointer to hash context
    \return The function call status, pass/fail.
*/
int vaultip_hash(uint32_t identity, HASH_ALG_t hash_alg, const void *msg, size_t msg_size,
                 uint8_t *hash);

/*! \fn int vaultip_hash_update(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                        const void *msg, size_t msg_size, bool init)
    \brief This function updates vaultip hash
    \param identity system ID
    \param hash_alg hashing algoeithm to initialize
    \param digest_asset_id digest_asset_id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \return The function call status, pass/fail.
*/
int vaultip_hash_update(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                        const void *msg, size_t msg_size, bool init);

/*! \fn int vaultip_hash_final(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                        const void *msg, size_t msg_size, bool init)
    \brief This function updates vaultip hash
    \param identity system ID
    \param hash_alg hashing algoeithm to initialize
    \param digest_asset_id digest_asset_id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param total_msg_length total msg length 
    \param hash pointer to hash context
    \return The function call status, pass/fail.
*/
int vaultip_hash_final(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                       const void *msg, size_t msg_size, bool init, size_t total_msg_length,
                       uint8_t *hash);

/*! \fn int vaultip_mac_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                         const void *msg, size_t msg_size, uint8_t *mac)
    \brief This function generates MAC
    \param identity system ID
    \param mac_alg algorithm to generate MAC
    \param key_asset_id asset key id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param mac pointer to mac context
    \return The function call status, pass/fail.
*/
int vaultip_mac_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                         const void *msg, size_t msg_size, uint8_t *mac);

/*! \fn int vaultip_mac_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                       const void *msg, size_t msg_size, const uint8_t *mac)
    \brief This function used to verify MAC
    \param identity system ID
    \param mac_alg algorithm to generate MAC
    \param key_asset_id asset key id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param mac pointer to mac context
    \return The function call status, pass/fail.
*/
int vaultip_mac_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                       const void *msg, size_t msg_size, const uint8_t *mac);

/*! \fn int vaultip_mac_update(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                       uint32_t key_asset_id, const void *msg, size_t msg_size, bool init)
    \brief This function used to verify MAC
    \param identity system ID
    \param mac_alg algorithm to generate MAC
    \param mac_asset_id mac asset key id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param mac pointer to mac context
    \return The function call status, pass/fail.
*/
int vaultip_mac_update(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                       uint32_t key_asset_id, const void *msg, size_t msg_size, bool init);

/*! \fn int vaultip_mac_final_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                               uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                               size_t msg_size, size_t total_msg_size, bool init, uint8_t *mac)
    \brief This function used to finalize MAC generation process
    \param identity system ID
    \param mac_alg algorithm to generate MAC
    \param mac_asset_id mac asset key id
    \param key_asset_id key asset key id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param total_msg_size total msg length 
    \param mac pointer to mac context
    \return The function call status, pass/fail.
*/
int vaultip_mac_final_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                               uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                               size_t msg_size, size_t total_msg_size, bool init, uint8_t *mac);

/*! \fn int vaultip_mac_final_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                             uint32_t key_asset_id, const void *msg, size_t msg_size,
                             size_t total_msg_size, bool init, const uint8_t *mac)
    \brief This function used to verify MAC
    \param identity system ID
    \param mac_alg algorithm to generate MAC
    \param mac_asset_id mac asset key id
    \param key_asset_id key asset key id
    \param msg pointer to data containing hash
    \param msg_size size of msg data
    \param init flag to ste initialization
    \param total_msg_size total msg length 
    \param mac pointer to mac context
    \return The function call status, pass/fail.
*/
int vaultip_mac_final_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                             uint32_t key_asset_id, const void *msg, size_t msg_size,
                             size_t total_msg_size, bool init, const uint8_t *mac);

/*! \fn int vaultip_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size)
    \brief This function used to encrypt data using AES algorithm
    \param identity system ID
    \param key_asset_id key asset key id
    \param IV 
    \param data data to be encrypted
    \param data_size size of data
    \return The function call status, pass/fail.
*/
int vaultip_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size);

/*! \fn int vaultip_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size)
    \brief This function used to decrypt data using AES algorithm
    \param identity system ID
    \param key_asset_id key asset key id
    \param IV 
    \param data data to be encrypted
    \param data_size size of data
    \return The function call status, pass/fail.
*/
int vaultip_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size);

/*! \fn int vaultip_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32,
                         VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings,
                         uint32_t lifetime, uint32_t *asset_id)
    \brief This function used to create an asset in vaultip
    \param identity system ID
    \param policy_31_00 policy for creating asset
    \param policy_63_32 policy for creating asset
    \param other_settings settings other then policy
    \param lifetime lifetime of asset
    \param asset_id identity of asset
    \return The function call status, pass/fail.
*/
int vaultip_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32,
                         VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings,
                         uint32_t lifetime, uint32_t *asset_id);

/*! \fn int vaultip_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void *data,
                                 uint32_t data_size)
    \brief This function used to load plain text 
    \param identity system ID
    \param asset_id identity of asset
    \param data data to be loaded
    \param data_size size of data
    \return The function call status, pass/fail.
*/
int vaultip_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void *data,
                                 uint32_t data_size);

/*! \fn int vaultip_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id,
                              const uint8_t *key_expansion_IV, uint32_t key_expansion_IV_length,
                              const uint8_t *associated_data, uint32_t associated_data_size,
                              const uint8_t *salt, uint32_t salt_size)
    \brief This function used to load asset data
    \param identity system ID
    \param asset_id identity of asset
    \param kdk_asset_id kdk asset id
    \param key_expansion_IV
    \param key_expansion_IV_length
    \param associated_data pointer to associated data
    \param associated_data_size associated data size
    \param salt pointer to salt
    \param salt_size 
    \return The function call status, pass/fail.
*/
int vaultip_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id,
                              const uint8_t *key_expansion_IV, uint32_t key_expansion_IV_length,
                              const uint8_t *associated_data, uint32_t associated_data_size,
                              const uint8_t *salt, uint32_t salt_size);

/*! \fn int vaultip_asset_delete(uint32_t identity, uint32_t asset_id)
    \brief This function used to delete asset in vaultip
    \param identity system ID
    \param asset_id identity of asset
    \return The function call status, pass/fail.
*/
int vaultip_asset_delete(uint32_t identity, uint32_t asset_id);

/*! \fn int vaultip_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number,
                                uint32_t *asset_id, uint32_t *data_length)
    \brief This function used to search for asset in vaultip
    \param identity system ID
    \param asset_id pointer to searched identity of asset
    \param asset_number number of asset
    \param data_length length of asset returned
    \return The function call status, pass/fail.
*/
int vaultip_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number,
                                uint32_t *asset_id, uint32_t *data_length);

/*! \fn int vaultip_public_data_read(uint32_t identity, uint32_t asset_id, uint8_t *data_buffer,
                             uint32_t data_buffer_size, uint32_t *data_size)
    \brief This function used to read data from vaultip
    \param identity system ID
    \param asset_id pointer to searched identity of asset
    \param data_buffer buffer containing data
    \param data_size length of buffer
    \param data_size length of data
    \return The function call status, pass/fail.
*/
int vaultip_public_data_read(uint32_t identity, uint32_t asset_id, uint8_t *data_buffer,
                             uint32_t data_buffer_size, uint32_t *data_size);

/*! \fn int vaultip_monotonic_counter_read(uint32_t identity, uint32_t asset_id, uint8_t *counter_buffer,
                                   uint32_t counter_buffer_size, uint32_t *data_size)
    \brief This function used to read counter value from vaultip
    \param identity system ID
    \param asset_id pointer to searched identity of asset
    \param counter_buffer buffer containing counter data
    \param counter_buffer_size buffer size
    \param data_size length of data
    \return The function call status, pass/fail.
*/
int vaultip_monotonic_counter_read(uint32_t identity, uint32_t asset_id, uint8_t *counter_buffer,
                                   uint32_t counter_buffer_size, uint32_t *data_size);

/*! \fn int vaultip_monotonic_counter_increment(uint32_t identity, uint32_t asset_id)
    \brief This function used to read monotonic counter value from vaultip
    \param identity system ID
    \param asset_id pointer to searched identity of asset
    \return The function call status, pass/fail.
*/
int vaultip_monotonic_counter_increment(uint32_t identity, uint32_t asset_id);

/*! \fn int vaultip_otp_data_write(uint32_t identity, uint32_t asset_number, uint32_t policy_number,
                           bool CRC, const void *input_data, size_t input_data_length,
                           const void *associated_data, size_t associated_data_length)
    \brief This function used to write data to OTP
    \param identity system ID
    \param asset_number asset number to write
    \param policy_number policy number to write
    \param input_data pointer to data to be written
    \param input_data_length length of data
    \param associated_data associated data with command
    \param associated_data_length associated data length
    \return The function call status, pass/fail.
*/
int vaultip_otp_data_write(uint32_t identity, uint32_t asset_number, uint32_t policy_number,
                           bool CRC, const void *input_data, size_t input_data_length,
                           const void *associated_data, size_t associated_data_length);

/*! \fn int vaultip_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity,
                                    uint32_t public_key_asset_id,
                                    uint32_t curve_parameters_asset_id,
                                    uint32_t temp_message_digest_asset_id, const void *message,
                                    uint32_t message_size, uint32_t hash_data_length,
                                    const void *sig_data_address, uint32_t sig_data_size)
    \brief This function used to verify ecdsa public key
    \param identity system ID
    \param curve_id curve identity
    \param public_key_asset_id public key of asset id
    \param curve_parameters_asset_id curve asset id
    \param temp_message_digest_asset_id temp message asset id
    \param message message containing data
    \param message_size message
    \param hash_data_length hash data length
    \param sig_data_address sig data address
    \param sig_data_size sig data length
    \return The function call status, pass/fail.
*/
int vaultip_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity,
                                    uint32_t public_key_asset_id,
                                    uint32_t curve_parameters_asset_id,
                                    uint32_t temp_message_digest_asset_id, const void *message,
                                    uint32_t message_size, uint32_t hash_data_length,
                                    const void *sig_data_address, uint32_t sig_data_size);

/*! \fn int vaultip_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity,
                                      uint32_t public_key_asset_id,
                                      uint32_t temp_message_digest_asset_id, const void *message,
                                      uint32_t message_size, uint32_t hash_data_length,
                                      const void *sig_data_address, uint32_t sig_data_size,
                                      uint32_t salt_length)
    \brief This function used to verify rsa public key
    \param identity system ID
    \param modulus_size modulus size
    \param public_key_asset_id public key of asset id
    \param temp_message_digest_asset_id temp message asset id
    \param message message containing data
    \param message_size message
    \param hash_data_length hash data length
    \param sig_data_address sig data address
    \param sig_data_size sig data length
    \param salt_length salt length
    \return The function call status, pass/fail.
*/
int vaultip_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity,
                                      uint32_t public_key_asset_id,
                                      uint32_t temp_message_digest_asset_id, const void *message,
                                      uint32_t message_size, uint32_t hash_data_length,
                                      const void *sig_data_address, uint32_t sig_data_size,
                                      uint32_t salt_length);


/*! \fn int vaultip_clock_switch(uint32_t identity, uint32_t token)
    \brief This function used to switch clock
    \param identity system ID
    \param token vaultip token
    \return The function call status, pass/fail.
*/
int vaultip_clock_switch(uint32_t identity, uint32_t token);

#endif
