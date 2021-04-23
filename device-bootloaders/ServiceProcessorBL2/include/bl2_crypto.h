#ifndef __BL2_CRYPTO_H__
#define __BL2_CRYPTO_H__

#include <stdlib.h>

#include "esperanto_signed_image_format/public_key_data.h"
#include "vaultip_static_assets.h"

typedef struct CRYPTO_HASH_CONTEXT_s {
    HASH_ALG_t hash_alg;
    uint32_t temp_digest_asset_id;
    bool init_done;
} CRYPTO_HASH_CONTEXT_t;

typedef struct CRYPTO_AES_CONTEXT_s {
    uint32_t aes_key_asset_id;
    uint8_t IV[16];
} CRYPTO_AES_CONTEXT_t;

int crypto_init(uint32_t vaultip_coid_set);

int crypto_verify_public_key_params(const PUBLIC_KEY_t *public_key);
int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t *signature);

int crypto_hash(HASH_ALG_t hash_alg, const void *msg, size_t msg_size, uint8_t *hash);

int crypto_hash_init(CRYPTO_HASH_CONTEXT_t *hash_context, HASH_ALG_t hash_alg);
int crypto_hash_update(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size);
int crypto_hash_final(CRYPTO_HASH_CONTEXT_t *hash_context, const void *msg, size_t msg_size,
                      size_t total_msg_length, uint8_t *hash);
int crypto_hash_abort(CRYPTO_HASH_CONTEXT_t *hash_context);

int crypto_derive_kdk_key(const void *derivation_data, size_t derivation_data_size,
                          uint32_t *kdk_key);
int crypto_derive_mac_key(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t kdk_key,
                          const void *derivation_data, size_t derivation_data_size,
                          uint32_t *mac_key);
int crypto_derive_enc_key(uint32_t kdk_key, const void *derivation_data,
                          size_t derivation_data_size, uint32_t *enc_key);
int crypto_delete_key(uint32_t key);

int crypto_mac_verify(ESPERANTO_MAC_TYPE_t mac_alg, const uint32_t mack_key, const void *data,
                      size_t data_size, const void *mac);

int crypto_aes_decrypt_init(CRYPTO_AES_CONTEXT_t *aes_context, const uint32_t enck_key,
                            const uint8_t *IV);
int crypto_aes_decrypt_update(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size);
int crypto_aes_decrypt_final(CRYPTO_AES_CONTEXT_t *aes_context, void *data, size_t data_size,
                             uint8_t *IV);

int crypto_verify_pk_signature(const PUBLIC_KEY_t *public_key, const PUBLIC_SIGNATURE_t *signature,
                               const void *data, size_t data_size);

int crypto_load_public_key_hash_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                         size_t buffer_size, uint32_t *hash_size);
int crypto_load_monotonic_counter_from_otp(VAULTIP_STATIC_ASSET_ID_t static_asset_id, void *buffer,
                                           size_t buffer_size, uint32_t *counter_size);

int crypto_get_sp_bl1_monotonic_counter_value(uint32_t *value);
int crypto_get_pcie_cfg_data_monotonic_counter_value(uint32_t *value);
int crypto_get_sp_bl2_monotonic_counter_value(uint32_t *value);
int crypto_get_machine_minion_monotonic_counter_value(uint32_t *value);
int crypto_get_master_minion_monotonic_counter_value(uint32_t *value);
int crypto_get_worker_minion_monotonic_counter_value(uint32_t *value);
int crypto_get_maxion_bl1_monotonic_counter_value(uint32_t *value);
uint32_t get_rom_identity(void);

#endif
