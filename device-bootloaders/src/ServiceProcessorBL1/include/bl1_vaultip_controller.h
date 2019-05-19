/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL1_VAULTIP_CONTROLLER_H__
#define __BL1_VAULTIP_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

#include "vaultip_sw.h"
#include "vaultip_static_assets.h"

int vaultip_test_initial_state(void);
void dump_vip_regs(void);
int vaultip_verify_firmware_header(const uint32_t * firmware, uint32_t firmware_size_in_bytes);
int vaultip_load_firmware(const uint32_t * firmware, uint32_t length_in_bytes);
int vaultip_send_input_token(const VAULTIP_INPUT_TOKEN_t * input_token);
int vaultip_read_output_token(VAULTIP_OUTPUT_TOKEN_t * output_token, uint32_t timeout);
int vaultip_get_system_information(VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t * system_info);
int vaultip_register_read(bool incremental_read, uint32_t number, const uint32_t * address, uint32_t * result);
int vaultip_register_write(bool incremental_write, uint32_t number, const uint32_t * mask, const uint32_t * address, const uint32_t * value);
int vaultip_trng_configuration(void);
int vaultip_provision_huk(void);
int vaultip_reset(void);

int vaultip_hash(HASH_ALG_t hash_alg, const void * msg, size_t msg_size, uint8_t * hash);
int vaultip_hash_init(HASH_ALG_t hash_alg, uint32_t digest_asset_id, const void * msg, size_t msg_size);
int vaultip_hash_update(HASH_ALG_t hash_alg, uint32_t digest_asset_id, const void * msg, size_t msg_size);
int vaultip_hash_final(HASH_ALG_t hash_alg, uint32_t digest_asset_id, const void * msg, size_t msg_size, size_t total_msg_length, uint8_t * hash);

int vaultip_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32, VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings, uint32_t lifetime, uint32_t * asset_id);
int vaultip_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void * data, uint32_t data_size);
int vaultip_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id, const uint8_t * key_expansion_IV, uint32_t key_expansion_IV_length, const uint8_t * associated_data, uint32_t associated_data_size, const uint8_t * salt, uint32_t salt_size);
int vaultip_asset_delete(uint32_t identity, uint32_t asset_id);
int vaultip_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number, uint32_t * asset_id, uint32_t * data_length);
int vaultip_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity, uint32_t public_key_asset_id, uint32_t curve_parameters_asset_id, uint32_t temp_message_digest_asset_id, const void * message, uint32_t message_size, uint32_t hash_data_length, const void * sig_data_address, uint32_t sig_data_size);
int vaultip_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity, uint32_t public_key_asset_id, uint32_t temp_message_digest_asset_id, const void * message, uint32_t message_size, uint32_t hash_data_length, const void * sig_data_address, uint32_t sig_data_size, uint32_t salt_length);

#endif
