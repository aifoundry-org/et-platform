#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "bl2_task_priorities.h"

#include "vaultip_hw.h"
#include "vaultip_sw.h"
#include "vaultip_sw_asset.h"
#include "bl2_flash_fs.h"

#include "bl2_vaultip_controller.h"
#include "bl2_vaultip_driver.h"

#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define VAULTIP_DRIVER_TASK_STACK_SIZE     4096
#define VAULTIP_DRIVER_REQUEST_QUEUE_SIZE  4
#define VAULTIP_DRIVER_RESPONSE_QUEUE_SIZE 4

typedef enum VAULTIP_DRIVER_REQUEST_e {
    VAULTIP_DRIVER_REQUEST_INVALID,
    VAULTIP_DRIVER_SELF_TEST,
    VAULTIP_DRIVER_GET_SYSTEM_INFORMATION,
    VAULTIP_DRIVER_REGISTER_READ,
    VAULTIP_DRIVER_REGISTER_WRITE,
    VAULTIP_DRIVER_TRNG_CONFIGURATION,
    VAULTIP_DRIVER_TRNG_GET_RANDOM_NUMBER,
    VAULTIP_DRIVER_PROVISION_HUK,
    VAULTIP_DRIVER_RESET,

    VAULTIP_DRIVER_HASH,
    VAULTIP_DRIVER_HASH_UPDATE,
    VAULTIP_DRIVER_HASH_FINAL,

    VAULTIP_DRIVER_MAC_GENERATE,
    VAULTIP_DRIVER_MAC_VERIFY,
    VAULTIP_DRIVER_MAC_UPDATE,
    VAULTIP_DRIVER_MAC_FINAL_GENERATE,
    VAULTIP_DRIVER_MAC_FINAL_VERIFY,

    VAULTIP_DRIVER_AES_CBC_ENCRYPT,
    VAULTIP_DRIVER_AES_CBC_DECRYPT,

    VAULTIP_DRIVER_ASSET_CREATE,
    VAULTIP_DRIVER_ASSET_LOAD_PLAINTEXT,
    VAULTIP_DRIVER_ASSET_LOAD_DERIVE,
    VAULTIP_DRIVER_ASSET_DELETE,
    VAULTIP_DRIVER_STATIC_ASSET_SEARCH,

    VAULTIP_DRIVER_PUBLIC_DATA_READ,
    VAULTIP_DRIVER_MONOTONIC_COUNTER_READ,
    VAULTIP_DRIVER_MONOTONIC_COUNTER_INCREMENT,
    VAULTIP_DRIVER_OTP_DATA_WRITE,

    VAULTIP_DRIVER_PUBLIC_KEY_ECDSA_VERIFY,
    VAULTIP_DRIVER_PUBLIC_KEY_RSA_PSS_VERIFY,

    VAULTIP_DRIVER_CLOCK_SWITCH,

    _VAULTIP_DRIVER_REQUEST_COUNT_
} VAULTIP_DRIVER_REQUEST_t;

typedef struct VAULTIP_DRIVER_REQUEST_MESSAGE_s {
    uint64_t id;
    VAULTIP_DRIVER_REQUEST_t request;
    union {
        struct {
            uint32_t identity;
            VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info;
        } get_system_information;
        struct {
            uint32_t identity;
            bool incremental_read;
            uint32_t number;
            const uint32_t *address;
            uint32_t *result;
        } register_read;
        struct {
            uint32_t identity;
            bool incremental_write;
            uint32_t number;
            const uint32_t *mask;
            const uint32_t *address;
            const uint32_t *value;
        } register_write;
        struct {
            uint32_t identity;
        } trng_configuration;
        struct {
            uint32_t identity;
            void *destination;
            uint16_t size;
            bool raw;
        } get_random_number;
        struct {
            uint32_t coid;
        } provision_huk;
        struct {
            uint32_t identity;
        } reset;

        struct {
            uint32_t identity;
            HASH_ALG_t hash_alg;
            const void *msg;
            size_t msg_size;
            uint8_t *hash;
        } hash;
        struct {
            uint32_t identity;
            HASH_ALG_t hash_alg;
            uint32_t digest_asset_id;
            const void *msg;
            size_t msg_size;
            bool init;
        } hash_update;
        struct {
            uint32_t identity;
            HASH_ALG_t hash_alg;
            uint32_t digest_asset_id;
            const void *msg;
            size_t msg_size;
            bool init;
            size_t total_msg_length;
            uint8_t *hash;
        } hash_final;

        struct {
            uint32_t identity;
            ESPERANTO_MAC_TYPE_t mac_alg;
            uint32_t key_asset_id;
            const void *msg;
            size_t msg_size;
            uint8_t *mac;
        } mac_generate;
        struct {
            uint32_t identity;
            ESPERANTO_MAC_TYPE_t mac_alg;
            uint32_t key_asset_id;
            const void *msg;
            size_t msg_size;
            const uint8_t *mac;
        } mac_verify;
        struct {
            uint32_t identity;
            ESPERANTO_MAC_TYPE_t mac_alg;
            uint32_t mac_asset_id;
            uint32_t key_asset_id;
            const void *msg;
            size_t msg_size;
            bool init;
        } mac_update;
        struct {
            uint32_t identity;
            ESPERANTO_MAC_TYPE_t mac_alg;
            uint32_t mac_asset_id;
            uint32_t key_asset_id;
            const void *msg;
            size_t msg_size;
            size_t total_msg_size;
            bool init;
            uint8_t *mac;
        } mac_final_generate;
        struct {
            uint32_t identity;
            ESPERANTO_MAC_TYPE_t mac_alg;
            uint32_t mac_asset_id;
            uint32_t key_asset_id;
            const void *msg;
            size_t msg_size;
            size_t total_msg_size;
            bool init;
            const uint8_t *mac;
        } mac_final_verify;

        struct {
            uint32_t identity;
            uint32_t key_asset_id;
            uint8_t *IV;
            void *data;
            size_t data_size;
        } aes_cbc_encrypt;
        struct {
            uint32_t identity;
            uint32_t key_asset_id;
            uint8_t *IV;
            void *data;
            size_t data_size;
        } aes_cbc_decrypt;

        struct {
            uint32_t identity;
            uint32_t policy_31_00;
            uint32_t policy_63_32;
            VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings;
            uint32_t lifetime;
            uint32_t *asset_id;
        } asset_create;
        struct {
            uint32_t identity;
            uint32_t asset_id;
            const void *data;
            uint32_t data_size;
        } asset_load_plaintext;
        struct {
            uint32_t identity;
            uint32_t asset_id;
            uint32_t kdk_asset_id;
            const uint8_t *key_expansion_IV;
            uint32_t key_expansion_IV_length;
            const uint8_t *associated_data;
            uint32_t associated_data_size;
            const uint8_t *salt;
            uint32_t salt_size;
        } asset_load_derive;
        struct {
            uint32_t identity;
            uint32_t asset_id;
        } asset_delete;
        struct {
            uint32_t identity;
            VAULTIP_STATIC_ASSET_ID_t asset_number;
            uint32_t *asset_id;
            uint32_t *data_length;
        } static_asset_search;

        struct {
            uint32_t identity;
            uint32_t asset_id;
            uint8_t *data_buffer;
            uint32_t data_buffer_size;
            uint32_t *data_size;
        } public_data_read;
        struct {
            uint32_t identity;
            uint32_t asset_id;
            uint8_t *counter_buffer;
            uint32_t counter_buffer_size;
            uint32_t *data_size;
        } monotonic_counter_read;
        struct {
            uint32_t identity;
            uint32_t asset_id;
        } monotonic_counter_increment;
        struct {
            uint32_t identity;
            uint32_t asset_number;
            uint32_t policy_number;
            bool CRC;
            const void *input_data;
            size_t input_data_length;
            const void *associated_data;
            size_t associated_data_length;
        } otp_data_write;

        struct {
            EC_KEY_CURVE_ID_t curve_id;
            uint32_t identity;
            uint32_t public_key_asset_id;
            uint32_t curve_parameters_asset_id;
            uint32_t temp_message_digest_asset_id;
            const void *message;
            uint32_t message_size;
            uint32_t hash_data_length;
            const void *sig_data_address;
            uint32_t sig_data_size;
        } public_key_ecdsa_verify;
        struct {
            uint32_t modulus_size;
            uint32_t identity;
            uint32_t public_key_asset_id;
            uint32_t temp_message_digest_asset_id;
            const void *message;
            uint32_t message_size;
            uint32_t hash_data_length;
            const void *sig_data_address;
            uint32_t sig_data_size;
            uint32_t salt_length;
        } public_key_rsa_pss_verify;

        struct {
            uint32_t identity;
            uint32_t token;
        } clock_switch;
    } args;
} VAULTIP_DRIVER_REQUEST_MESSAGE_t;

typedef struct VAULTIP_DRIVER_RESPONSE_MESSAGE_s {
    uint64_t id;
    VAULTIP_DRIVER_REQUEST_t request;
    int status_code;
} VAULTIP_DRIVER_RESPONSE_MESSAGE_t;

static QueueHandle_t gs_vaultip_driver_reqeust_queue;
static uint8_t
    gs_vaultip_driver_reqeust_queue_storage_buffer[VAULTIP_DRIVER_REQUEST_QUEUE_SIZE *
                                                   sizeof(VAULTIP_DRIVER_REQUEST_MESSAGE_t)];
static StaticQueue_t gs_vaultip_driver_reqeust_queue_buffer;

static QueueHandle_t gs_vaultip_driver_response_queue;
static uint8_t
    gs_vaultip_driver_response_queue_storage_buffer[VAULTIP_DRIVER_RESPONSE_QUEUE_SIZE *
                                                    sizeof(VAULTIP_DRIVER_RESPONSE_MESSAGE_t)];
static StaticQueue_t gs_vaultip_driver_response_queue_buffer;

static TaskHandle_t gs_vaultip_driver_task_handle;
static StackType_t gs_vaultip_driver_task_stack_buffer[VAULTIP_DRIVER_TASK_STACK_SIZE];
static StaticTask_t gs_vaultip_driver_task_buffer;

static uint64_t gs_next_request_id;

static uint64_t get_next_request_id(void)
{
    return ++gs_next_request_id;
}

static void vaultip_driver_task(void *pvParameters)
{
    (void)pvParameters;
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req_msg;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp_msg;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1) {
        if (pdTRUE != xQueueReceive(gs_vaultip_driver_reqeust_queue, &req_msg, portMAX_DELAY)) {
            Log_Write(LOG_LEVEL_ERROR, "vaultip_driver_task:  xQueueReceive() failed!\r\n");
            continue;
        }

        rsp_msg.id = req_msg.id;
        rsp_msg.request = req_msg.request;

        switch (req_msg.request) {
        case VAULTIP_DRIVER_SELF_TEST:
            rsp_msg.status_code = vaultip_self_test();
            break;
        case VAULTIP_DRIVER_GET_SYSTEM_INFORMATION:
            rsp_msg.status_code =
                vaultip_get_system_information(req_msg.args.get_system_information.identity,
                                               req_msg.args.get_system_information.system_info);
            break;
        case VAULTIP_DRIVER_REGISTER_READ:
            rsp_msg.status_code = vaultip_register_read(req_msg.args.register_read.identity,
                                                        req_msg.args.register_read.incremental_read,
                                                        req_msg.args.register_read.number,
                                                        req_msg.args.register_read.address,
                                                        req_msg.args.register_read.result);
            break;
        case VAULTIP_DRIVER_REGISTER_WRITE:
            rsp_msg.status_code = vaultip_register_write(
                req_msg.args.register_read.identity, req_msg.args.register_write.incremental_write,
                req_msg.args.register_write.number, req_msg.args.register_write.mask,
                req_msg.args.register_write.address, req_msg.args.register_write.value);
            break;
        case VAULTIP_DRIVER_TRNG_CONFIGURATION:
            rsp_msg.status_code =
                vaultip_trng_configuration(req_msg.args.trng_configuration.identity);
            break;
        case VAULTIP_DRIVER_TRNG_GET_RANDOM_NUMBER:
            rsp_msg.status_code = vaultip_trng_get_random_number(
                req_msg.args.get_random_number.destination, req_msg.args.get_random_number.size,
                req_msg.args.get_random_number.raw);
            break;
        case VAULTIP_DRIVER_PROVISION_HUK:
            rsp_msg.status_code = vaultip_provision_huk(req_msg.args.provision_huk.coid);
            break;
        case VAULTIP_DRIVER_RESET:
            rsp_msg.status_code = vaultip_reset(req_msg.args.reset.identity);
            break;

        case VAULTIP_DRIVER_HASH:
            rsp_msg.status_code = vaultip_hash(req_msg.args.hash.identity,
                                               req_msg.args.hash.hash_alg, req_msg.args.hash.msg,
                                               req_msg.args.hash.msg_size, req_msg.args.hash.hash);
            break;
        case VAULTIP_DRIVER_HASH_UPDATE:
            rsp_msg.status_code = vaultip_hash_update(
                req_msg.args.hash.identity, req_msg.args.hash_update.hash_alg,
                req_msg.args.hash_update.digest_asset_id, req_msg.args.hash_update.msg,
                req_msg.args.hash_update.msg_size, req_msg.args.hash_update.init);
            break;
        case VAULTIP_DRIVER_HASH_FINAL:
            rsp_msg.status_code = vaultip_hash_final(
                req_msg.args.hash.identity, req_msg.args.hash_final.hash_alg,
                req_msg.args.hash_final.digest_asset_id, req_msg.args.hash_final.msg,
                req_msg.args.hash_final.msg_size, req_msg.args.hash_final.init,
                req_msg.args.hash_final.total_msg_length, req_msg.args.hash_final.hash);
            break;

        case VAULTIP_DRIVER_MAC_GENERATE:
            rsp_msg.status_code = vaultip_mac_generate(
                req_msg.args.mac_generate.identity, req_msg.args.mac_generate.mac_alg,
                req_msg.args.mac_generate.key_asset_id, req_msg.args.mac_generate.msg,
                req_msg.args.mac_generate.msg_size, req_msg.args.mac_generate.mac);
            break;
        case VAULTIP_DRIVER_MAC_VERIFY:
            rsp_msg.status_code = vaultip_mac_verify(
                req_msg.args.mac_verify.identity, req_msg.args.mac_verify.mac_alg,
                req_msg.args.mac_verify.key_asset_id, req_msg.args.mac_verify.msg,
                req_msg.args.mac_verify.msg_size, req_msg.args.mac_verify.mac);
            break;
        case VAULTIP_DRIVER_MAC_UPDATE:
            rsp_msg.status_code = vaultip_mac_update(
                req_msg.args.mac_update.identity, req_msg.args.mac_update.mac_alg,
                req_msg.args.mac_update.mac_asset_id, req_msg.args.mac_update.key_asset_id,
                req_msg.args.mac_update.msg, req_msg.args.mac_update.msg_size,
                req_msg.args.mac_update.init);
            break;
        case VAULTIP_DRIVER_MAC_FINAL_GENERATE:
            rsp_msg.status_code = vaultip_mac_final_generate(
                req_msg.args.mac_final_generate.identity, req_msg.args.mac_final_generate.mac_alg,
                req_msg.args.mac_final_generate.mac_asset_id,
                req_msg.args.mac_final_generate.key_asset_id, req_msg.args.mac_final_generate.msg,
                req_msg.args.mac_final_generate.msg_size,
                req_msg.args.mac_final_generate.total_msg_size,
                req_msg.args.mac_final_generate.init, req_msg.args.mac_final_generate.mac);
            break;
        case VAULTIP_DRIVER_MAC_FINAL_VERIFY:
            rsp_msg.status_code = vaultip_mac_final_verify(
                req_msg.args.mac_final_verify.identity, req_msg.args.mac_final_verify.mac_alg,
                req_msg.args.mac_final_verify.mac_asset_id,
                req_msg.args.mac_final_verify.key_asset_id, req_msg.args.mac_final_verify.msg,
                req_msg.args.mac_final_verify.msg_size,
                req_msg.args.mac_final_verify.total_msg_size, req_msg.args.mac_final_verify.init,
                req_msg.args.mac_final_verify.mac);
            break;

        case VAULTIP_DRIVER_AES_CBC_ENCRYPT:
            rsp_msg.status_code = vaultip_aes_cbc_encrypt(req_msg.args.aes_cbc_encrypt.identity,
                                                          req_msg.args.aes_cbc_encrypt.key_asset_id,
                                                          req_msg.args.aes_cbc_encrypt.IV,
                                                          req_msg.args.aes_cbc_encrypt.data,
                                                          req_msg.args.aes_cbc_encrypt.data_size);
            break;
        case VAULTIP_DRIVER_AES_CBC_DECRYPT:
            rsp_msg.status_code = vaultip_aes_cbc_decrypt(req_msg.args.aes_cbc_decrypt.identity,
                                                          req_msg.args.aes_cbc_decrypt.key_asset_id,
                                                          req_msg.args.aes_cbc_decrypt.IV,
                                                          req_msg.args.aes_cbc_decrypt.data,
                                                          req_msg.args.aes_cbc_decrypt.data_size);
            break;

        case VAULTIP_DRIVER_ASSET_CREATE:
            rsp_msg.status_code = vaultip_asset_create(
                req_msg.args.asset_create.identity, req_msg.args.asset_create.policy_31_00,
                req_msg.args.asset_create.policy_63_32, req_msg.args.asset_create.other_settings,
                req_msg.args.asset_create.lifetime, req_msg.args.asset_create.asset_id);
            break;
        case VAULTIP_DRIVER_ASSET_LOAD_PLAINTEXT:
            rsp_msg.status_code = vaultip_asset_load_plaintext(
                req_msg.args.asset_load_plaintext.identity,
                req_msg.args.asset_load_plaintext.asset_id, req_msg.args.asset_load_plaintext.data,
                req_msg.args.asset_load_plaintext.data_size);
            break;
        case VAULTIP_DRIVER_ASSET_LOAD_DERIVE:
            rsp_msg.status_code = vaultip_asset_load_derive(
                req_msg.args.asset_load_derive.identity, req_msg.args.asset_load_derive.asset_id,
                req_msg.args.asset_load_derive.kdk_asset_id,
                req_msg.args.asset_load_derive.key_expansion_IV,
                req_msg.args.asset_load_derive.key_expansion_IV_length,
                req_msg.args.asset_load_derive.associated_data,
                req_msg.args.asset_load_derive.associated_data_size,
                req_msg.args.asset_load_derive.salt, req_msg.args.asset_load_derive.salt_size);
            break;
        case VAULTIP_DRIVER_ASSET_DELETE:
            rsp_msg.status_code = vaultip_asset_delete(req_msg.args.asset_delete.identity,
                                                       req_msg.args.asset_delete.asset_id);
            break;
        case VAULTIP_DRIVER_STATIC_ASSET_SEARCH:
            rsp_msg.status_code =
                vaultip_static_asset_search(req_msg.args.static_asset_search.identity,
                                            req_msg.args.static_asset_search.asset_number,
                                            req_msg.args.static_asset_search.asset_id,
                                            req_msg.args.static_asset_search.data_length);
            break;

        case VAULTIP_DRIVER_PUBLIC_DATA_READ:
            rsp_msg.status_code = vaultip_public_data_read(
                req_msg.args.public_data_read.identity, req_msg.args.public_data_read.asset_id,
                req_msg.args.public_data_read.data_buffer,
                req_msg.args.public_data_read.data_buffer_size,
                req_msg.args.public_data_read.data_size);
            break;
        case VAULTIP_DRIVER_MONOTONIC_COUNTER_READ:
            rsp_msg.status_code = vaultip_monotonic_counter_read(
                req_msg.args.monotonic_counter_read.identity,
                req_msg.args.monotonic_counter_read.asset_id,
                req_msg.args.monotonic_counter_read.counter_buffer,
                req_msg.args.monotonic_counter_read.counter_buffer_size,
                req_msg.args.monotonic_counter_read.data_size);
            break;
        case VAULTIP_DRIVER_MONOTONIC_COUNTER_INCREMENT:
            rsp_msg.status_code = vaultip_monotonic_counter_increment(
                req_msg.args.monotonic_counter_increment.identity,
                req_msg.args.monotonic_counter_increment.asset_id);
            break;
        case VAULTIP_DRIVER_OTP_DATA_WRITE:
            rsp_msg.status_code = vaultip_otp_data_write(
                req_msg.args.otp_data_write.identity, req_msg.args.otp_data_write.asset_number,
                req_msg.args.otp_data_write.policy_number, req_msg.args.otp_data_write.CRC,
                req_msg.args.otp_data_write.input_data,
                req_msg.args.otp_data_write.input_data_length,
                req_msg.args.otp_data_write.associated_data,
                req_msg.args.otp_data_write.associated_data_length);
            break;

        case VAULTIP_DRIVER_PUBLIC_KEY_ECDSA_VERIFY:
            rsp_msg.status_code = vaultip_public_key_ecdsa_verify(
                req_msg.args.public_key_ecdsa_verify.curve_id,
                req_msg.args.public_key_ecdsa_verify.identity,
                req_msg.args.public_key_ecdsa_verify.public_key_asset_id,
                req_msg.args.public_key_ecdsa_verify.curve_parameters_asset_id,
                req_msg.args.public_key_ecdsa_verify.temp_message_digest_asset_id,
                req_msg.args.public_key_ecdsa_verify.message,
                req_msg.args.public_key_ecdsa_verify.message_size,
                req_msg.args.public_key_ecdsa_verify.hash_data_length,
                req_msg.args.public_key_ecdsa_verify.sig_data_address,
                req_msg.args.public_key_ecdsa_verify.sig_data_size);
            break;
        case VAULTIP_DRIVER_PUBLIC_KEY_RSA_PSS_VERIFY:
            rsp_msg.status_code = vaultip_public_key_rsa_pss_verify(
                req_msg.args.public_key_rsa_pss_verify.modulus_size,
                req_msg.args.public_key_rsa_pss_verify.identity,
                req_msg.args.public_key_rsa_pss_verify.public_key_asset_id,
                req_msg.args.public_key_rsa_pss_verify.temp_message_digest_asset_id,
                req_msg.args.public_key_rsa_pss_verify.message,
                req_msg.args.public_key_rsa_pss_verify.message_size,
                req_msg.args.public_key_rsa_pss_verify.hash_data_length,
                req_msg.args.public_key_rsa_pss_verify.sig_data_address,
                req_msg.args.public_key_rsa_pss_verify.sig_data_size,
                req_msg.args.public_key_rsa_pss_verify.salt_length);
            break;
        case VAULTIP_DRIVER_CLOCK_SWITCH:
            rsp_msg.status_code = vaultip_clock_switch(req_msg.args.clock_switch.identity,
                                                       req_msg.args.clock_switch.token);
            break;

        default:
            Log_Write(LOG_LEVEL_ERROR, "vaultip_driver_task: invalid or not supported request code %u!\r\n",
                   req_msg.request);
            rsp_msg.status_code = -1;
        }

        if (pdTRUE != xQueueSend(gs_vaultip_driver_response_queue, &rsp_msg, portMAX_DELAY)) {
            Log_Write(LOG_LEVEL_ERROR, "vaultip_driver_task:  xQueueSend() failed!\r\n");
            continue;
        }
    }
}

int Vault_Initialize(void)
{
    gs_next_request_id = 0;

    gs_vaultip_driver_reqeust_queue = xQueueCreateStatic(
        VAULTIP_DRIVER_REQUEST_QUEUE_SIZE, sizeof(VAULTIP_DRIVER_REQUEST_MESSAGE_t),
        gs_vaultip_driver_reqeust_queue_storage_buffer, &gs_vaultip_driver_reqeust_queue_buffer);
    if (NULL == gs_vaultip_driver_reqeust_queue) {
        Log_Write(LOG_LEVEL_ERROR, "Vault_Initialize:  xQueueCreateStatic(request_queue) failed!\r\n");
        return -1;
    }

    gs_vaultip_driver_response_queue = xQueueCreateStatic(
        VAULTIP_DRIVER_RESPONSE_QUEUE_SIZE, sizeof(VAULTIP_DRIVER_RESPONSE_MESSAGE_t),
        gs_vaultip_driver_response_queue_storage_buffer, &gs_vaultip_driver_response_queue_buffer);
    if (NULL == gs_vaultip_driver_response_queue) {
        Log_Write(LOG_LEVEL_ERROR, "Vault_Initialize:  xQueueCreateStatic(response_queue) failed!\r\n");
        return -1;
    }

    gs_vaultip_driver_task_handle =
        xTaskCreateStatic(vaultip_driver_task, "VAULTIP_DRV_TASK", VAULTIP_DRIVER_TASK_STACK_SIZE,
                          NULL, // pvParameters
                          VAULTIP_DRIVER_TASK_PRIORITY, gs_vaultip_driver_task_stack_buffer,
                          &gs_vaultip_driver_task_buffer);
    if (NULL == gs_vaultip_driver_task_handle) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_init:  xTaskCreateStatic() failed!\r\n");
        return -1;
    }

    return 0;
}

static int queue_request_and_wait_for_response(const VAULTIP_DRIVER_REQUEST_MESSAGE_t *req_msg,
                                               VAULTIP_DRIVER_RESPONSE_MESSAGE_t *rsp_msg)
{
    if (pdTRUE != xQueueSend(gs_vaultip_driver_reqeust_queue, req_msg, portMAX_DELAY)) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  xQueueSend() failed!\r\n");
        return -1;
    }

    if (pdTRUE != xQueueReceive(gs_vaultip_driver_response_queue, rsp_msg, portMAX_DELAY)) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  xQueueSend() failed!\r\n");
        return -1;
    }

    if (req_msg->id != rsp_msg->id) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  id mismatch!\r\n");
        return -1;
    }

    if (req_msg->request != rsp_msg->request) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  request mismatch!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_self_test(void)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_SELF_TEST;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_self_test: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_self_test: vaultip_drv_get_system_information() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_get_system_information(uint32_t identity,
                                       VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_GET_SYSTEM_INFORMATION;
    req.args.get_system_information.identity = identity;
    req.args.get_system_information.system_info = system_info;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_get_system_information: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_get_system_information: vaultip_drv_get_system_information() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_register_read(uint32_t identity, bool incremental_read, uint32_t number,
                              const uint32_t *address, uint32_t *result)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_REGISTER_READ;
    req.args.register_read.identity = identity;
    req.args.register_read.incremental_read = incremental_read;
    req.args.register_read.number = number;
    req.args.register_read.address = address;
    req.args.register_read.result = result;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_register_read: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_register_read: vaultip_drv_register_read() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_register_write(uint32_t identity, bool incremental_write, uint32_t number,
                               const uint32_t *mask, const uint32_t *address, const uint32_t *value)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_REGISTER_WRITE;
    req.args.register_write.identity = identity;
    req.args.register_write.incremental_write = incremental_write;
    req.args.register_write.number = number;
    req.args.register_write.mask = mask;
    req.args.register_write.address = address;
    req.args.register_write.value = value;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_register_write: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_register_write: vaultip_drv_register_write() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_trng_configuration(uint32_t identity)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_TRNG_CONFIGURATION;
    req.args.trng_configuration.identity = identity;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_trng_configuration: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_trng_configuration: vaultip_drv_trng_configuration() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_trng_get_random_number(uint32_t identity, void *dst, uint16_t size, bool raw)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_TRNG_GET_RANDOM_NUMBER;
    req.args.get_random_number.identity = identity;
    req.args.get_random_number.destination = dst;
    req.args.get_random_number.size = size;
    req.args.get_random_number.raw = raw;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_trng_get_random_number: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_trng_get_random_number: vaultip_drv_trng_get_random_number() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_provision_huk(uint32_t coid)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_PROVISION_HUK;
    req.args.provision_huk.coid = coid;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_provision_huk: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_provision_huk: vaultip_drv_provision_huk() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_reset(uint32_t identity)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_RESET;
    req.args.reset.identity = identity;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_reset: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_reset: vaultip_drv_reset() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_hash(uint32_t identity, HASH_ALG_t hash_alg, const void *msg, size_t msg_size,
                     uint8_t *hash)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_HASH;
    req.args.hash.identity = identity;
    req.args.hash.hash_alg = hash_alg;
    req.args.hash.msg = msg;
    req.args.hash.msg_size = msg_size;
    req.args.hash.hash = hash;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash: vaultip_drv_hash() failed!\r\n");
        return -1;
    }

    return 0;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash: vaultip_drv_hash() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_hash_update(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                            const void *msg, size_t msg_size, bool init)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_HASH_UPDATE;
    req.args.hash_update.identity = identity;
    req.args.hash_update.hash_alg = hash_alg;
    req.args.hash_update.digest_asset_id = digest_asset_id;
    req.args.hash_update.msg = msg;
    req.args.hash_update.msg_size = msg_size;
    req.args.hash_update.init = init;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash_update: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash_update: vaultip_hash_update() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_hash_final(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                           const void *msg, size_t msg_size, bool init, size_t total_msg_length,
                           uint8_t *hash)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_HASH_FINAL;
    req.args.hash_final.identity = identity;
    req.args.hash_final.hash_alg = hash_alg;
    req.args.hash_final.digest_asset_id = digest_asset_id;
    req.args.hash_final.msg = msg;
    req.args.hash_final.msg_size = msg_size;
    req.args.hash_final.init = init;
    req.args.hash_final.total_msg_length = total_msg_length;
    req.args.hash_final.hash = hash;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash_final: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_hash_final: vaultip_hash_final() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_mac_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                             const void *msg, size_t msg_size, uint8_t *mac)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MAC_GENERATE;
    req.args.mac_generate.identity = identity;
    req.args.mac_generate.mac_alg = mac_alg;
    req.args.mac_generate.key_asset_id = key_asset_id;
    req.args.mac_generate.msg = msg;
    req.args.mac_generate.msg_size = msg_size;
    req.args.mac_generate.mac = mac;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_generate: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_generate: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_mac_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                           const void *msg, size_t msg_size, const uint8_t *mac)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MAC_VERIFY;
    req.args.mac_verify.identity = identity;
    req.args.mac_verify.mac_alg = mac_alg;
    req.args.mac_verify.key_asset_id = key_asset_id;
    req.args.mac_verify.msg = msg;
    req.args.mac_verify.msg_size = msg_size;
    req.args.mac_verify.mac = mac;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_verify: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_verify: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_mac_update(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                           uint32_t key_asset_id, const void *msg, size_t msg_size, bool init)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MAC_UPDATE;
    req.args.mac_update.identity = identity;
    req.args.mac_update.mac_alg = mac_alg;
    req.args.mac_update.mac_asset_id = mac_asset_id;
    req.args.mac_update.key_asset_id = key_asset_id;
    req.args.mac_update.msg = msg;
    req.args.mac_update.msg_size = msg_size;
    req.args.mac_update.init = init;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_update: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_update: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_mac_final_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                                   uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                                   size_t msg_size, size_t total_msg_size, bool init, uint8_t *mac)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MAC_FINAL_GENERATE;
    req.args.mac_final_generate.identity = identity;
    req.args.mac_final_generate.mac_alg = mac_alg;
    req.args.mac_final_generate.mac_asset_id = mac_asset_id;
    req.args.mac_final_generate.key_asset_id = key_asset_id;
    req.args.mac_final_generate.msg = msg;
    req.args.mac_final_generate.msg_size = msg_size;
    req.args.mac_final_generate.total_msg_size = total_msg_size;
    req.args.mac_final_generate.init = init;
    req.args.mac_final_generate.mac = mac;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_final_generate: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_final_generate: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_mac_final_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                                 uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                                 size_t msg_size, size_t total_msg_size, bool init,
                                 const uint8_t *mac)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MAC_FINAL_VERIFY;
    req.args.mac_final_verify.identity = identity;
    req.args.mac_final_verify.mac_alg = mac_alg;
    req.args.mac_final_verify.mac_asset_id = mac_asset_id;
    req.args.mac_final_verify.key_asset_id = key_asset_id;
    req.args.mac_final_verify.msg = msg;
    req.args.mac_final_verify.msg_size = msg_size;
    req.args.mac_final_verify.total_msg_size = total_msg_size;
    req.args.mac_final_verify.init = init;
    req.args.mac_final_verify.mac = mac;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_final_verify: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_mac_final_verify: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                                size_t data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_AES_CBC_ENCRYPT;
    req.args.aes_cbc_encrypt.identity = identity;
    req.args.aes_cbc_encrypt.key_asset_id = key_asset_id;
    req.args.aes_cbc_encrypt.IV = IV;
    req.args.aes_cbc_encrypt.data = data;
    req.args.aes_cbc_encrypt.data_size = data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_aes_cbc_encrypt: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_aes_cbc_encrypt: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                                size_t data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_AES_CBC_DECRYPT;
    req.args.aes_cbc_decrypt.identity = identity;
    req.args.aes_cbc_decrypt.key_asset_id = key_asset_id;
    req.args.aes_cbc_decrypt.IV = IV;
    req.args.aes_cbc_decrypt.data = data;
    req.args.aes_cbc_decrypt.data_size = data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_aes_cbc_decrypt: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_aes_cbc_decrypt: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32,
                             VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings,
                             uint32_t lifetime, uint32_t *asset_id)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_ASSET_CREATE;
    req.args.asset_create.identity = identity;
    req.args.asset_create.policy_31_00 = policy_31_00;
    req.args.asset_create.policy_63_32 = policy_63_32;
    req.args.asset_create.other_settings = other_settings;
    req.args.asset_create.lifetime = lifetime;
    req.args.asset_create.asset_id = asset_id;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_create: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_create: vaultip_asset_create() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void *data,
                                     uint32_t data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_ASSET_LOAD_PLAINTEXT;
    req.args.asset_load_plaintext.identity = identity;
    req.args.asset_load_plaintext.asset_id = asset_id;
    req.args.asset_load_plaintext.data = data;
    req.args.asset_load_plaintext.data_size = data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_asset_load_plaintext: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_load_plaintext: vaultip_asset_load_plaintext() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id,
                                  const uint8_t *key_expansion_IV, uint32_t key_expansion_IV_length,
                                  const uint8_t *associated_data, uint32_t associated_data_size,
                                  const uint8_t *salt, uint32_t salt_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_ASSET_LOAD_DERIVE;
    req.args.asset_load_derive.identity = identity;
    req.args.asset_load_derive.asset_id = asset_id;
    req.args.asset_load_derive.kdk_asset_id = kdk_asset_id;
    req.args.asset_load_derive.key_expansion_IV = key_expansion_IV;
    req.args.asset_load_derive.key_expansion_IV_length = key_expansion_IV_length;
    req.args.asset_load_derive.associated_data = associated_data;
    req.args.asset_load_derive.associated_data_size = associated_data_size;
    req.args.asset_load_derive.salt = salt;
    req.args.asset_load_derive.salt_size = salt_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_load_derive: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_load_derive: vaultip_asset_load_derive() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_asset_delete(uint32_t identity, uint32_t asset_id)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_ASSET_DELETE;
    req.args.asset_delete.identity = identity;
    req.args.asset_delete.asset_id = asset_id;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_delete: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_asset_delete: vaultip_asset_delete() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number,
                                    uint32_t *asset_id, uint32_t *data_length)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_STATIC_ASSET_SEARCH;
    req.args.static_asset_search.identity = identity;
    req.args.static_asset_search.asset_number = asset_number;
    req.args.static_asset_search.asset_id = asset_id;
    req.args.static_asset_search.data_length = data_length;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_static_asset_search: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_static_asset_search: vaultip_static_asset_search() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_public_data_read(uint32_t identity, uint32_t asset_id, uint8_t *data_buffer,
                                 uint32_t data_buffer_size, uint32_t *data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_PUBLIC_DATA_READ;
    req.args.public_data_read.identity = identity;
    req.args.public_data_read.asset_id = asset_id;
    req.args.public_data_read.data_buffer = data_buffer;
    req.args.public_data_read.data_buffer_size = data_buffer_size;
    req.args.public_data_read.data_size = data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_public_data_read: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_public_data_read: vaultip_static_asset_search() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_monotonic_counter_read(uint32_t identity, uint32_t asset_id,
                                       uint8_t *counter_buffer, uint32_t counter_buffer_size,
                                       uint32_t *data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MONOTONIC_COUNTER_READ;
    req.args.monotonic_counter_read.identity = identity;
    req.args.monotonic_counter_read.asset_id = asset_id;
    req.args.monotonic_counter_read.counter_buffer = counter_buffer;
    req.args.monotonic_counter_read.counter_buffer_size = counter_buffer_size;
    req.args.monotonic_counter_read.data_size = data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_monotonic_counter_read: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_monotonic_counter_read: vaultip_static_asset_search() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_monotonic_counter_increment(uint32_t identity, uint32_t asset_id)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_MONOTONIC_COUNTER_INCREMENT;
    req.args.monotonic_counter_increment.identity = identity;
    req.args.monotonic_counter_increment.asset_id = asset_id;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_monotonic_counter_increment: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_monotonic_counter_increment: vaultip_static_asset_search() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_otp_data_write(uint32_t identity, uint32_t asset_number, uint32_t policy_number,
                               bool CRC, const void *input_data, size_t input_data_length,
                               const void *associated_data, size_t associated_data_length)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_OTP_DATA_WRITE;
    req.args.otp_data_write.identity = identity;
    req.args.otp_data_write.asset_number = asset_number;
    req.args.otp_data_write.policy_number = policy_number;
    req.args.otp_data_write.CRC = CRC;
    req.args.otp_data_write.input_data = input_data;
    req.args.otp_data_write.input_data_length = input_data_length;
    req.args.otp_data_write.associated_data = associated_data;
    req.args.otp_data_write.associated_data_length = associated_data_length;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_otp_data_write: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_otp_data_write: vaultip_static_asset_search() failed!\r\n");
        return -1;
    }

    return 0;
}

int vaultip_drv_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity,
                                        uint32_t public_key_asset_id,
                                        uint32_t curve_parameters_asset_id,
                                        uint32_t temp_message_digest_asset_id, const void *message,
                                        uint32_t message_size, uint32_t hash_data_length,
                                        const void *sig_data_address, uint32_t sig_data_size)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_PUBLIC_KEY_ECDSA_VERIFY;
    req.args.public_key_ecdsa_verify.curve_id = curve_id;
    req.args.public_key_ecdsa_verify.identity = identity;
    req.args.public_key_ecdsa_verify.public_key_asset_id = public_key_asset_id;
    req.args.public_key_ecdsa_verify.curve_parameters_asset_id = curve_parameters_asset_id;
    req.args.public_key_ecdsa_verify.temp_message_digest_asset_id = temp_message_digest_asset_id;
    req.args.public_key_ecdsa_verify.message = message;
    req.args.public_key_ecdsa_verify.message_size = message_size;
    req.args.public_key_ecdsa_verify.hash_data_length = hash_data_length;
    req.args.public_key_ecdsa_verify.sig_data_address = sig_data_address;
    req.args.public_key_ecdsa_verify.sig_data_size = sig_data_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_public_key_ecdsa_verify: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_public_key_ecdsa_verify: vaultip_public_key_ecdsa_verify() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity,
                                          uint32_t public_key_asset_id,
                                          uint32_t temp_message_digest_asset_id,
                                          const void *message, uint32_t message_size,
                                          uint32_t hash_data_length, const void *sig_data_address,
                                          uint32_t sig_data_size, uint32_t salt_length)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_PUBLIC_KEY_RSA_PSS_VERIFY;
    req.args.public_key_rsa_pss_verify.modulus_size = modulus_size;
    req.args.public_key_rsa_pss_verify.identity = identity;
    req.args.public_key_rsa_pss_verify.public_key_asset_id = public_key_asset_id;
    req.args.public_key_rsa_pss_verify.temp_message_digest_asset_id = temp_message_digest_asset_id;
    req.args.public_key_rsa_pss_verify.message = message;
    req.args.public_key_rsa_pss_verify.message_size = message_size;
    req.args.public_key_rsa_pss_verify.hash_data_length = hash_data_length;
    req.args.public_key_rsa_pss_verify.sig_data_address = sig_data_address;
    req.args.public_key_rsa_pss_verify.sig_data_size = sig_data_size;
    req.args.public_key_rsa_pss_verify.salt_length = salt_length;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_public_key_rsa_pss_verify: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_drv_public_key_rsa_pss_verify: vaultip_public_key_rsa_pss_verify() failed!\r\n");
        return -1;
    }

    return 0;
}
int vaultip_drv_clock_switch(uint32_t identity, uint32_t token)
{
    VAULTIP_DRIVER_REQUEST_MESSAGE_t req;
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = VAULTIP_DRIVER_CLOCK_SWITCH;
    req.args.clock_switch.identity = identity;
    req.args.clock_switch.token = token;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_clock_switch: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_clock_switch: vaultip_public_key_rsa_pss_verify() failed!\r\n");
        return -1;
    }

    return 0;
}

int Vault_Command_Issue(void *req)
{
    VAULTIP_DRIVER_RESPONSE_MESSAGE_t rsp;

    /* send command to driver and wait for response*/
    if (0 != queue_request_and_wait_for_response((VAULTIP_DRIVER_REQUEST_MESSAGE_t *)req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "vaultip_drv_clock_switch: queue_request_and_wait_for_response() failed!\r\n");
        return ERROR_VAULTIP_COMMAND_FAILED;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "Vault_Command_Issue: invalid response!\r\n");
        return rsp.status_code;
    }

    return 0;
}

#pragma GCC pop_options
