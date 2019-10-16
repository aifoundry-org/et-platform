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

#include "serial.h"
#include <string.h>
#include <stdio.h>

#include "bl2_main.h"
#include "bl2_flashfs_driver.h"
#include "bl2_crypto.h"
#include "bl2_certificates.h"
#include "bl2_firmware_loader.h"
#include "bl2_sp_otp.h"
#include "constant_memory_compare.h"

#include "key_derivation_data.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

// uncomment the following line to ignore HASH check results using VaultIP
// #define IGNORE_HASH

static const uint8_t gs_machine_minion_kdk_derivation_data[] = MACHINE_MINION_KDK_DERIVATION_DATA;
static const uint8_t gs_machine_minion_mac_derivation_data[] = MACHINE_MINION_MAC_DERIVATION_DATA;
static const uint8_t gs_machine_minion_enc_derivation_data[] = MACHINE_MINION_ENC_DERIVATION_DATA;

static const uint8_t gs_master_minion_kdk_derivation_data[] = MASTER_MINION_KDK_DERIVATION_DATA;
static const uint8_t gs_master_minion_mac_derivation_data[] = MASTER_MINION_MAC_DERIVATION_DATA;
static const uint8_t gs_master_minion_enc_derivation_data[] = MASTER_MINION_ENC_DERIVATION_DATA;

static const uint8_t gs_worker_minion_kdk_derivation_data[] = WORKER_MINION_KDK_DERIVATION_DATA;
static const uint8_t gs_worker_minion_mac_derivation_data[] = WORKER_MINION_MAC_DERIVATION_DATA;
static const uint8_t gs_worker_minion_enc_derivation_data[] = WORKER_MINION_ENC_DERIVATION_DATA;

static uint32_t gs_kdk;
static bool gs_kdk_created;
static uint32_t gs_mack;
static bool gs_mack_created;
static uint32_t gs_enck;
static bool gs_enck_created;
static uint8_t gs_IV[16];
static CRYPTO_AES_CONTEXT_t gs_aes_context;
static bool gs_aes_context_created;

static bool gs_vaultip_disabled;
static bool gs_ignore_signatures;

static int get_kdk_derivation_data(const ESPERANTO_IMAGE_TYPE_t image_type, const uint8_t ** kdk_data, size_t * kdk_data_size, const uint8_t ** mac_data, size_t * mac_data_size, const uint8_t ** enc_data, size_t * enc_data_size) {
    switch (image_type) {
    case ESPERANTO_IMAGE_TYPE_MACHINE_MINION:
        *kdk_data = gs_machine_minion_kdk_derivation_data;
        *kdk_data_size = sizeof(gs_machine_minion_kdk_derivation_data);
        *mac_data = gs_machine_minion_mac_derivation_data;
        *mac_data_size = sizeof(gs_machine_minion_mac_derivation_data);
        *enc_data = gs_machine_minion_enc_derivation_data;
        *enc_data_size = sizeof(gs_machine_minion_enc_derivation_data);
        break;
    case ESPERANTO_IMAGE_TYPE_MASTER_MINION:
        *kdk_data = gs_master_minion_kdk_derivation_data;
        *kdk_data_size = sizeof(gs_master_minion_kdk_derivation_data);
        *mac_data = gs_master_minion_mac_derivation_data;
        *mac_data_size = sizeof(gs_master_minion_mac_derivation_data);
        *enc_data = gs_master_minion_enc_derivation_data;
        *enc_data_size = sizeof(gs_master_minion_enc_derivation_data);
        break;
    case ESPERANTO_IMAGE_TYPE_WORKER_MINION:
        *kdk_data = gs_worker_minion_kdk_derivation_data;
        *kdk_data_size = sizeof(gs_worker_minion_kdk_derivation_data);
        *mac_data = gs_worker_minion_mac_derivation_data;
        *mac_data_size = sizeof(gs_worker_minion_mac_derivation_data);
        *enc_data = gs_worker_minion_enc_derivation_data;
        *enc_data_size = sizeof(gs_worker_minion_enc_derivation_data);
        break;
    default:
        return -1;
    }

    return 0;
}

static int verify_image_file_header(const ESPERANTO_IMAGE_TYPE_t image_type, ESPERANTO_IMAGE_FILE_HEADER_t * image_file_header, uint32_t image_file_size) {
    const uint8_t * kdk_derivation_data;
    size_t kdk_derivation_data_size;
    const uint8_t * mack_derivation_data;
    size_t mack_derivation_data_size;
    const uint8_t * enck_derivation_data;
    size_t enck_derivation_data_size;

    if (NULL == image_file_header) {
        return -1;
    }

    if (0 != get_kdk_derivation_data(image_type, &kdk_derivation_data, &kdk_derivation_data_size, &mack_derivation_data, &mack_derivation_data_size, &enck_derivation_data, &enck_derivation_data_size)) {
        return -1;
    }

    if (CURRENT_EXEC_FILE_HEADER_TAG != image_file_header->info.file_header_tag) {
        printf("verify_image_file_header: invalid file header tag (%08x)!\n", image_file_header->info.file_header_tag);
        return -1;
    }

    if (CURRENT_EXEC_IMAGE_HEADER_TAG != image_file_header->info.image_info_and_signaure.info.public_info.header_tag) {
        printf("verify_image_file_header: invalid image header tag (%08x)!\n", image_file_header->info.image_info_and_signaure.info.public_info.header_tag);
        return -1;
    }

    if (image_type != image_file_header->info.image_info_and_signaure.info.public_info.image_type) {
        printf("verify_image_file_header: Unexpected image type! (%08x, expected %08x)!\n", image_file_header->info.image_info_and_signaure.info.public_info.image_type, image_type);
        return -1;
    }

    if ((sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) + image_file_header->info.image_info_and_signaure.info.public_info.code_and_data_size) != image_file_size) {
        printf("verify_image_file_header: invalid file size!\n");
        return -1;
    }

    if (gs_vaultip_disabled) {
        MESSAGE_INFO("Image CRT IGN\n");
    } else {
        if (0 != verify_esperanto_image_certificate(image_type, &(image_file_header->info.signing_certificate))) {
            printf("verify_image_file_header: image certificate is not valid!\n");
            return -1;
        } else {
            printf("Image certificate OK!\n");
        }
    }

    if (0 != (image_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED)) {
        // derive the KDK key

        if (0 != crypto_derive_kdk_key(kdk_derivation_data, kdk_derivation_data_size, &gs_kdk)) {
            printf("verify_image_file_header: crypto_derive_kdk_key() failed!\n");
            return -1;
        }
        gs_kdk_created = true;

        // derive the MAC key

        if (0 != crypto_derive_mac_key(image_file_header->info.mac_type, gs_kdk, mack_derivation_data, mack_derivation_data_size, &gs_mack)) {
            printf("verify_image_file_header: crypto_derive_mack_key() failed!\n");
            return -1;
        }
        gs_mack_created = true;

        // verify the MAC of the header

        if (0 != crypto_mac_verify(image_file_header->info.mac_type, gs_mack, &(image_file_header->info), sizeof(image_file_header->info), image_file_header->MAC)) {
            printf("verify_image_file_header: crypto_mac_verify() failed!\n");
            return -1;
        }

        // derive the ENC key

        if (0 != crypto_derive_enc_key(gs_kdk, enck_derivation_data, enck_derivation_data_size, &gs_enck)) {
            printf("verify_image_file_header: crypto_derive_enck_key() failed!\n");
            return -1;
        }
        gs_enck_created = true;

        // decrypt the header

        memcpy(gs_IV, image_file_header->info.encryption_IV, 16);
        if (0 != crypto_aes_decrypt_init(&gs_aes_context, gs_enck, gs_IV)) {
            printf("verify_image_file_header: crypto_aes_decrypt_init() failed!\n");
            return -1;
        }
        gs_aes_context_created = true;

        if (0 != crypto_aes_decrypt_update(&gs_aes_context, &(image_file_header->info.image_info_and_signaure.info.secret_info), sizeof(image_file_header->info.image_info_and_signaure.info.secret_info))) {
            printf("verify_image_file_header: crypto_aes_decrypt_update() failed!\n");
            return -1;
        }
    }

    if (gs_vaultip_disabled || gs_ignore_signatures) {
        MESSAGE_INFO("Image SIG IGN\n");
        return 0;
    } else {
        if (0 != crypto_verify_pk_signature(&(image_file_header->info.signing_certificate.certificate_info.subject_public_key), 
                                            &(image_file_header->info.image_info_and_signaure.info_signature),
                                            &(image_file_header->info.image_info_and_signaure.info), 
                                            sizeof(image_file_header->info.image_info_and_signaure.info))) {
            printf("firmware signature is not valid!\n");
            return -1;
        } else {
            printf("firmware signature is OK!\n");
        }
    }

    return 0;
}

#define CACHED_DDR_ADDR     0x8000000000lu
#define UNCACHED_DDR_ADDR   0xC000000000lu
#define DDR_SIZE            0x4000000000lu

static int remap_load_address(uint64_t * address, uint64_t size) {
    uint64_t address_end;

    if (0 == size) {
        return -1;
    }

    address_end = *address + size;
    if (address_end < *address) {
        return -1; // integer overflow
    }

    if (*address >= CACHED_DDR_ADDR && address_end <= (CACHED_DDR_ADDR + DDR_SIZE)) {
        *address = *address | 0x4000000000lu; // remap to uncached region
        return 0;
    }

    if (*address >= UNCACHED_DDR_ADDR && address_end <= (UNCACHED_DDR_ADDR + DDR_SIZE)) {
        return 0;
    }

    return -1;
}

static int load_image_code_and_data( ESPERANTO_FLASH_REGION_ID_t region_id, const ESPERANTO_IMAGE_FILE_HEADER_t * image_file_header) {
    uint32_t code_and_data_hash_size;
    uint32_t load_offset;
    union {
        uint64_t u64;
        struct {
            uint32_t lo;
            uint32_t hi;
        };
    } load_address;
    uint32_t n;
    uint32_t region_no = 0;
#ifndef IGNORE_HASH
    CRYPTO_HASH_CONTEXT_t hash_context;
    bool hash_context_initialized = false;
#endif
    uint8_t hash[64];
    CRYPTO_HASH_CONTEXT_t encrypted_hash_context;
    bool encrypted_hash_context_initialized = false;
    size_t total_length = 0;

    const ESPERANTO_IMAGE_INFO_t * image_info = &(image_file_header->info.image_info_and_signaure.info);

    switch (image_info->public_info.code_and_data_hash_algorithm) {
    case HASH_ALG_SHA2_256:
        code_and_data_hash_size = 256/8;
        break;
    case HASH_ALG_SHA2_384:
        code_and_data_hash_size = 384/8;
        break;
    case HASH_ALG_SHA2_512:
        code_and_data_hash_size = 512/8;
        break;
    default:
        printf("load_image_code_and_data: Invalid hash algorithm!\n");
        return -1;
    }

    if (0 != (image_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED)) {
        if (0 != crypto_hash_init(&encrypted_hash_context, image_info->public_info.code_and_data_hash_algorithm)) {
            printf("load_image_code_and_data: crypto_hash_init(e) failed!\n");
            return -1;
        }
        encrypted_hash_context_initialized = true;
    }

#ifndef IGNORE_HASH
    if (!gs_vaultip_disabled) {
        if (0 != crypto_hash_init(&hash_context, image_info->public_info.code_and_data_hash_algorithm)) {
            printf("load_image_code_and_data: crypto_hash_init() failed!\n");
            return -1;
        }
        hash_context_initialized = true;
    }
#endif

    for (region_no = 0; region_no < image_info->secret_info.load_regions_count; region_no++) {
        load_offset = (uint32_t)(sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) + image_info->secret_info.load_regions[region_no].region_offset);
        load_address.lo = image_info->secret_info.load_regions[region_no].load_address_lo;
        load_address.hi = image_info->secret_info.load_regions[region_no].load_address_hi;
        if (remap_load_address(&load_address.u64, image_info->secret_info.load_regions[region_no].memory_size)) {
            printf("Invalid region %u: load=0x%x, addr=0x%lx, fsize=0x%x, msize=0x%x\n", region_no, load_offset, load_address.u64, image_info->secret_info.load_regions[region_no].load_size, image_info->secret_info.load_regions[region_no].memory_size);
            goto CLEANUP_ON_ERROR;
        }
        printf("Region %u: load=0x%x, addr=0x%lx, fsize=0x%x, msize=0x%x\n", region_no, load_offset, load_address.u64, image_info->secret_info.load_regions[region_no].load_size, image_info->secret_info.load_regions[region_no].memory_size);

        if (image_info->secret_info.load_regions[region_no].load_size > 0) {
            if (0 != flashfs_drv_read_file(region_id, load_offset, (void*)load_address.u64, image_info->secret_info.load_regions[region_no].load_size)) {
                printf("load_image_code_and_data: flashfs_drv_read_file(code) failed!\n");
                goto CLEANUP_ON_ERROR;
            }
            printf("loaded 0x%x bytes at 0x%08lx\n", image_info->secret_info.load_regions[region_no].load_size, load_address.u64);

            if (!gs_vaultip_disabled) {
                if (0 != (image_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED)) {
                    // hash encrypted data
                    if (0 != crypto_hash_update(&encrypted_hash_context, (void*)load_address.u64, image_info->secret_info.load_regions[region_no].load_size)) {
                        printf("load_image_code_and_data: crypto_hash_update() failed!\n");
                        goto CLEANUP_ON_ERROR;
                    }

                    // decrypt data
                    if (0 != crypto_aes_decrypt_update(&gs_aes_context, (void*)load_address.u64, image_info->secret_info.load_regions[region_no].load_size)) {
                        printf("load_image_code_and_data: crypto_aes_decrypt_update() failed!\n");
                        goto CLEANUP_ON_ERROR;
                    }
                }

#ifndef IGNORE_HASH
                if (0 != crypto_hash_update(&hash_context, (void*)load_address.u64, image_info->secret_info.load_regions[region_no].load_size)) {
                    printf("load_image_code_and_data: crypto_hash_update() failed!\n");
                    goto CLEANUP_ON_ERROR;
                }
#endif
            }
            total_length = total_length + image_info->secret_info.load_regions[region_no].load_size;
        }

        if (image_info->secret_info.load_regions[region_no].memory_size > image_info->secret_info.load_regions[region_no].load_size) {
            memset((void*)(load_address.u64 + image_info->secret_info.load_regions[region_no].load_size), 0, image_info->secret_info.load_regions[region_no].memory_size - image_info->secret_info.load_regions[region_no].load_size);
            printf("erased 0x%x bytes\n", image_info->secret_info.load_regions[region_no].memory_size - image_info->secret_info.load_regions[region_no].load_size);
        }
    }

    if (0 != (image_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED)) {
        if (0 != crypto_hash_final(&encrypted_hash_context, NULL, 0, total_length, hash)) {
            printf("load_image_code_and_data: crypto_hash_final(p) failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        encrypted_hash_context_initialized = false;

        if (0 != constant_time_memory_compare(hash, image_file_header->info.encrypted_code_and_data_hash, code_and_data_hash_size)) {
            printf("load_image_code_and_data: encrypted code+data hash mismatch!\n");
            goto CLEANUP_ON_ERROR;
        }
    
        if (0 != crypto_aes_decrypt_final(&gs_aes_context, NULL, 0, NULL)) {
            printf("load_bl1_code_and_data: crypto_aes_decrypt_final() failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        gs_aes_context_created = false;
    }

#ifndef IGNORE_HASH
    if (gs_vaultip_disabled || gs_ignore_signatures) {
        MESSAGE_INFO("Image HASH IGN\n");
    } else {
        if (0 != crypto_hash_final(&hash_context, NULL, 0, total_length, hash)) {
            printf("load_image_code_and_data: crypto_hash_final() failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        hash_context_initialized = false;

        if (0 != constant_time_memory_compare(hash, image_info->public_info.code_and_data_hash, code_and_data_hash_size)) {
            printf("load_image_code_and_data: code+data hash mismatch!\n");
            goto CLEANUP_ON_ERROR;
        }
    }
#endif

    return 0;

CLEANUP_ON_ERROR:
    for (n = 0; n <= region_no; n++) {
        load_address.lo = image_info->secret_info.load_regions[n].load_address_lo;
        load_address.hi = image_info->secret_info.load_regions[n].load_address_hi;
        memset((void*)load_address.u64, 0, image_info->secret_info.load_regions[n].memory_size);
    }

    if (encrypted_hash_context_initialized) {
        if (0 != crypto_hash_abort(&encrypted_hash_context)) {
            printf("load_image_code_and_data: crypto_hash_abort(e) failed!\n");
        }
    }
#ifndef IGNORE_HASH
    if (hash_context_initialized) {
        if (0 != crypto_hash_abort(&hash_context)) {
            printf("load_image_code_and_data: crypto_hash_abort(p) failed!\n");
        }
    }
#endif
    return -1;
}
int load_firmware(const ESPERANTO_IMAGE_TYPE_t image_type) {
    int rv;
    uint32_t image_file_size;
    ESPERANTO_FLASH_REGION_ID_t region_id;
    const char * image_name;
    ESPERANTO_IMAGE_FILE_HEADER_t * image_file_header;
    SERVICE_PROCESSOR_BL2_DATA_t * bl2_data = get_service_processor_bl2_data();

    gs_kdk_created = false;
    gs_mack_created = false;
    gs_enck_created = false;
    gs_aes_context_created = false;

    gs_vaultip_disabled = is_vaultip_disabled();
    gs_ignore_signatures = false;

    if (0 != sp_otp_get_signatures_check_chicken_bit(&gs_ignore_signatures)) {
        gs_ignore_signatures = false;
    }

    switch (image_type) {
    // case ESPERANTO_IMAGE_TYPE_SP_BL1:
    //     designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_BL1_CA;
    //     region_id = ESPERANTO_FLASH_REGION_ID_SP_BL1;
    //     break;
    // case ESPERANTO_IMAGE_TYPE_SP_BL2:
    //     designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_BL2_CA;
    //     region_id = ESPERANTO_FLASH_REGION_ID_SP_BL2;
    //     break;
    case ESPERANTO_IMAGE_TYPE_MACHINE_MINION:
        region_id = ESPERANTO_FLASH_REGION_ID_MACHINE_MINION;
        image_name = "MACHINE_MINION";
        image_file_header = &(bl2_data->machine_minion_header);
        break;
    case ESPERANTO_IMAGE_TYPE_MASTER_MINION:
        region_id = ESPERANTO_FLASH_REGION_ID_MASTER_MINION;
        image_name = "MASTER_MINION";
        image_file_header = &(bl2_data->master_minion_header);
        break;
    case ESPERANTO_IMAGE_TYPE_WORKER_MINION:
        region_id = ESPERANTO_FLASH_REGION_ID_WORKER_MINION;
        image_name = "WORKER_MINION";
        image_file_header = &(bl2_data->worker_minion_header);
        break;
    case ESPERANTO_IMAGE_TYPE_MAXION_BL1:
        region_id = ESPERANTO_FLASH_REGION_ID_MAXION_BL1;
        image_file_header = &(bl2_data->maxion_bl1_header);
        image_name = "MAXION_BL1";
        break;
    default:
        printf("load_firmware: invalid image type!\n");
        return -1;
    }

    // load the image
    if (0 != flashfs_drv_get_file_size(region_id, &image_file_size)) {
        printf("load_firmware: flashfs_drv_get_file_size(%s) failed!\n", image_name);
        return -1;
    }
    if (image_file_size < sizeof(ESPERANTO_IMAGE_FILE_HEADER_t)) {
        printf("load_firmware: %s image file too small!\n", image_name);
        return -1;
    }
    if (0 != flashfs_drv_read_file(region_id, 0, image_file_header, sizeof(ESPERANTO_IMAGE_FILE_HEADER_t))) {
        printf("load_firmware: flashfs_drv_read_file(%s header) failed!\n", image_name);
        rv = -1;
        goto DONE;
    }
    printf("Loaded %s header...\n", image_name);

    if (0 != verify_image_file_header(image_type, image_file_header, image_file_size)) {
        printf("load_firmware: verify_image_file_header() failed!\n");
        rv = -1;
        goto DONE;
    }
    printf("Verified %s header...\n", image_name);

    if (0 != load_image_code_and_data(region_id, image_file_header)) {
        printf("load_firmware: load_image_code_and_data() failed!\n");
        rv = -1;
        goto DONE;
    }

    printf("load_firmware: Loaded %s firmware.\n", image_name);
    rv = 0;

DONE:
    if (gs_aes_context_created) {
        if (0 != crypto_aes_decrypt_final(&gs_aes_context, NULL, 0, NULL)) {
            printf("load_firmware: crypto_aes_decrypt_final() failed!\n");
        }
    }
    if (gs_enck_created) {
        if (0 != crypto_delete_key(gs_enck)) {
            printf("load_firmware: crypto_delete_key(enck) failed!\n");
            rv = -1;
        }
    }
    if (gs_mack_created) {
        if (0 != crypto_delete_key(gs_mack)) {
            printf("load_firmware: crypto_delete_key(mack) failed!\n");
            rv = -1;
        }
    }
    if (gs_kdk_created) {
        if (0 != crypto_delete_key(gs_kdk)) {
            printf("load_firmware: crypto_delete_key(kdk) failed!\n");
            rv = -1;
        }
    }

    if (0 != rv) {
        memset(image_file_header, 0, sizeof(ESPERANTO_IMAGE_FILE_HEADER_t));
        memset(&(gs_IV), 0, sizeof(gs_IV));
    }

    return rv;
}
