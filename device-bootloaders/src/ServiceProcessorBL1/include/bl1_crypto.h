#ifndef __BL1_CRYPTO_H__
#define __BL1_CRYPTO_H__

#include <stdlib.h>

int crypto_verify_public_key_params(const PUBLIC_KEY_t * public_key);
int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t * signature);

int crypto_hash(HASH_ALG_t hash_alg, const void * msg, size_t msg_size, uint8_t * hash);
int crypto_hash_init(HASH_ALG_t hash_alg, uint32_t temp_digest_asset_id, const void * msg, size_t msg_size);
int crypto_hash_update(HASH_ALG_t hash_alg, uint32_t temp_digest_asset_id, const void * msg, size_t msg_size);
int crypto_hash_final(HASH_ALG_t hash_alg, uint32_t temp_digest_asset_id, const void * msg, size_t msg_size, size_t total_msg_length, uint8_t * hash);

int crypto_verify_pk_signature(const PUBLIC_KEY_t * public_key, const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size);

#endif
