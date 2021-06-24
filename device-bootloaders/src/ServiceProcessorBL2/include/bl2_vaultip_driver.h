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
#ifndef __BL2_VAULTIP_DRIVER_H__
#define __BL2_VAULTIP_DRIVER_H__

#include "bl2_crypto.h"
#include "hal_vaultip_static_assets.h"
#include "hal_vaultip_sw.h"
#include "error.h"

/*! \fn uint8_t* Vault_Initialize(void)
    \brief Interface to initialize ValutIP
    \param None
    \returns Status indicating success or negative error
*/
int Vault_Initialize(void);

/*! \fn uint8_t* Vault_Command_Issue (uint32_t command, uint32_t payload)
    \brief Interface to issue command to ValutIP
    \param req request with required parameters
    \returns Status indicating success or negative error
*/
int Vault_Command_Issue(void *req);
int Vault_Initialize(void);
int vaultip_drv_self_test(void);
int vaultip_drv_get_system_information(uint32_t identity,
                                       VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info);
int vaultip_drv_register_read(uint32_t identity, bool incremental_read, uint32_t number,
                              const uint32_t *address, uint32_t *result);
int vaultip_drv_register_write(uint32_t identity, bool incremental_write, uint32_t number,
                               const uint32_t *mask, const uint32_t *address,
                               const uint32_t *value);
int vaultip_drv_trng_configuration(uint32_t identity);
int vaultip_drv_trng_get_random_number(uint32_t identity, void *dst, uint16_t size, bool raw);
int vaultip_drv_provision_huk(uint32_t coid);
int vaultip_drv_reset(uint32_t identity);

int vaultip_drv_hash(uint32_t identity, HASH_ALG_t hash_alg, const void *msg, size_t msg_size,
                     uint8_t *hash);
int vaultip_drv_hash_update(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                            const void *msg, size_t msg_size, bool init);
int vaultip_drv_hash_final(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                           const void *msg, size_t msg_size, bool init, size_t total_msg_length,
                           uint8_t *hash);

int vaultip_drv_mac_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                             const void *msg, size_t msg_size, uint8_t *mac);
int vaultip_drv_mac_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                           const void *msg, size_t msg_size, const uint8_t *mac);
int vaultip_drv_mac_update(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                           uint32_t key_asset_id, const void *msg, size_t msg_size, bool init);
int vaultip_drv_mac_final_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                                   uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                                   size_t msg_size, size_t total_msg_size, bool init, uint8_t *mac);
int vaultip_drv_mac_final_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                                 uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                                 size_t msg_size, size_t total_msg_size, bool init,
                                 const uint8_t *mac);

int vaultip_drv_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                                size_t data_size);
int vaultip_drv_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                                size_t data_size);

int vaultip_drv_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32,
                             VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings,
                             uint32_t lifetime, uint32_t *asset_id);
int vaultip_drv_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void *data,
                                     uint32_t data_size);
int vaultip_drv_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id,
                                  const uint8_t *key_expansion_IV, uint32_t key_expansion_IV_length,
                                  const uint8_t *associated_data, uint32_t associated_data_size,
                                  const uint8_t *salt, uint32_t salt_size);
int vaultip_drv_asset_delete(uint32_t identity, uint32_t asset_id);
int vaultip_drv_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number,
                                    uint32_t *asset_id, uint32_t *data_length);

int vaultip_drv_public_data_read(uint32_t identity, uint32_t asset_id, uint8_t *data_buffer,
                                 uint32_t data_buffer_size, uint32_t *data_size);
int vaultip_drv_monotonic_counter_read(uint32_t identity, uint32_t asset_id,
                                       uint8_t *counter_buffer, uint32_t counter_buffer_size,
                                       uint32_t *data_size);
int vaultip_drv_monotonic_counter_increment(uint32_t identity, uint32_t asset_id);
int vaultip_drv_otp_data_write(uint32_t identity, uint32_t asset_number, uint32_t policy_number,
                               bool CRC, const void *input_data, size_t input_data_length,
                               const void *associated_data, size_t associated_data_length);

int vaultip_drv_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity,
                                        uint32_t public_key_asset_id,
                                        uint32_t curve_parameters_asset_id,
                                        uint32_t temp_message_digest_asset_id, const void *message,
                                        uint32_t message_size, uint32_t hash_data_length,
                                        const void *sig_data_address, uint32_t sig_data_size);
int vaultip_drv_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity,
                                          uint32_t public_key_asset_id,
                                          uint32_t temp_message_digest_asset_id,
                                          const void *message, uint32_t message_size,
                                          uint32_t hash_data_length, const void *sig_data_address,
                                          uint32_t sig_data_size, uint32_t salt_length);

int vaultip_drv_clock_switch(uint32_t identity, uint32_t token);

#endif
