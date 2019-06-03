#ifndef __BL1_CRYPTO_H__
#define __BL1_CRYPTO_H__

#include <stdlib.h>
#include "esperanto_public_key_data.h"

typedef struct CRYPTO_HASH_CONTEXT_s {
    HASH_ALG_t hash_alg;
    uint32_t temp_digest_asset_id;
    bool init_done;
} CRYPTO_HASH_CONTEXT_t;

int crypto_verify_public_key_params(const PUBLIC_KEY_t * public_key);
int crypto_verify_signature_params(const PUBLIC_SIGNATURE_t * signature);

int crypto_hash(HASH_ALG_t hash_alg, const void * msg, size_t msg_size, uint8_t * hash);
int crypto_hash_init(CRYPTO_HASH_CONTEXT_t * hash_context, HASH_ALG_t hash_alg);
int crypto_hash_update(CRYPTO_HASH_CONTEXT_t * hash_context, const void * msg, size_t msg_size);
int crypto_hash_final(CRYPTO_HASH_CONTEXT_t * hash_context, const void * msg, size_t msg_size, size_t total_msg_length, uint8_t * hash);
int crypto_hash_abort(CRYPTO_HASH_CONTEXT_t * hash_context);

int crypto_verify_pk_signature(const PUBLIC_KEY_t * public_key, const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size);

#endif
