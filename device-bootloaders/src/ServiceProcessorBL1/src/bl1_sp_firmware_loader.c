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

#include <string.h>
#include "printx.h"

#include "bl1_main.h"
#include "bl1_sp_certificates.h"
#include "bl1_flash_fs.h"
#include "bl1_sp_certificates.h"
#include "bl1_sp_firmware_loader.h"
#include "sp_otp.h"
#include "constant_memory_compare.h"
#include "bl1_crypto.h"
#include "key_derivation_data.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

static uint8_t gs_sp_bl2_kdk_derivation_data[] = SP_BL2_KDK_DERIVATION_DATA;
static uint8_t gs_sp_bl2_mac_derivation_data[] = SP_BL2_MAC_DERIVATION_DATA;
static uint8_t gs_sp_bl2_enc_derivation_data[] = SP_BL2_ENC_DERIVATION_DATA;

static uint32_t gs_sp_bl2_kdk;
static bool gs_sp_bl2_kdk_created;
static uint32_t gs_sp_bl2_mack;
static bool gs_sp_bl2_mack_created;
static uint32_t gs_sp_bl2_enck;
static bool gs_sp_bl2_enck_created;
static uint8_t gs_sp_bl2_IV[16];
static CRYPTO_AES_CONTEXT_t gs_sp_bl2_aes_context;
static bool gs_sp_bl2_aes_context_created;

static bool gs_vaultip_disabled;
static bool gs_ignore_signatures;

static int verify_bl2_image_file_header(ESPERANTO_IMAGE_FILE_HEADER_t *sp_bl2_file_header,
                                        uint32_t sp_bl2_size)
{
    if (NULL == sp_bl2_file_header)
    {
        return -1;
    }

    if (CURRENT_EXEC_FILE_HEADER_TAG != sp_bl2_file_header->info.file_header_tag)
    {
        printx("verify_bl2_image_file_header: invalid file header tag (%08x)!\n",
               sp_bl2_file_header->info.file_header_tag);
        return -1;
    }

    if (CURRENT_EXEC_IMAGE_HEADER_TAG !=
        sp_bl2_file_header->info.image_info_and_signaure.info.public_info.header_tag)
    {
        printx("verify_bl2_image_file_header: invalid image header tag (%08x)!\n",
               sp_bl2_file_header->info.image_info_and_signaure.info.public_info.header_tag);
        return -1;
    }

    if (ESPERANTO_IMAGE_TYPE_SP_BL2 !=
        sp_bl2_file_header->info.image_info_and_signaure.info.public_info.image_type)
    {
        printx("verify_bl2_image_file_header: not a SP BL1 image type! (%08x)!\n",
               sp_bl2_file_header->info.image_info_and_signaure.info.public_info.image_type);
        return -1;
    }

    if ((sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) +
         sp_bl2_file_header->info.image_info_and_signaure.info.public_info.code_and_data_size) !=
        sp_bl2_size)
    {
        printx("verify_bl2_image_file_header: invalid file size!\n");
        return -1;
    }

    if (gs_vaultip_disabled)
    {
        MESSAGE_INFO("BL2 CRT IGN\n");
    }
    else
    {
        if (0 != verify_bl2_certificate(&(sp_bl2_file_header->info.signing_certificate)))
        {
            printx("SP BL2 certificate is not valid!\n");
            return -1;
        }
        else
        {
            printx("SP BL2 certificate OK!\n");
        }
    }

    if (0 !=
        (sp_bl2_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED))
    {
        // derive the KDK key

        if (0 != crypto_derive_kdk_key(gs_sp_bl2_kdk_derivation_data,
                                       sizeof(gs_sp_bl2_kdk_derivation_data), &gs_sp_bl2_kdk))
        {
            printx("verify_bl2_image_file_header: crypto_derive_kdk_key() failed!\n");
            return -1;
        }
        gs_sp_bl2_kdk_created = true;

        // derive the MAC key

        if (0 != crypto_derive_mac_key(sp_bl2_file_header->info.mac_type, gs_sp_bl2_kdk,
                                       gs_sp_bl2_mac_derivation_data,
                                       sizeof(gs_sp_bl2_mac_derivation_data), &gs_sp_bl2_mack))
        {
            printx("verify_bl2_image_file_header: crypto_derive_mack_key() failed!\n");
            return -1;
        }
        gs_sp_bl2_mack_created = true;

        // verify the MAC of the header

        if (0 != crypto_mac_verify(sp_bl2_file_header->info.mac_type, gs_sp_bl2_mack,
                                   &(sp_bl2_file_header->info), sizeof(sp_bl2_file_header->info),
                                   sp_bl2_file_header->MAC))
        {
            printx("verify_bl2_image_file_header: crypto_mac_verify() failed!\n");
            return -1;
        }

        // derive the ENC key

        if (0 != crypto_derive_enc_key(gs_sp_bl2_kdk, gs_sp_bl2_enc_derivation_data,
                                       sizeof(gs_sp_bl2_enc_derivation_data), &gs_sp_bl2_enck))
        {
            printx("verify_bl2_image_file_header: crypto_derive_enck_key() failed!\n");
            return -1;
        }
        gs_sp_bl2_enck_created = true;

        // decrypt the header

        memcpy(gs_sp_bl2_IV, sp_bl2_file_header->info.encryption_IV, 16);
        if (0 != crypto_aes_decrypt_init(&gs_sp_bl2_aes_context, gs_sp_bl2_enck, gs_sp_bl2_IV))
        {
            printx("verify_bl2_image_file_header: crypto_aes_decrypt_init() failed!\n");
            return -1;
        }
        gs_sp_bl2_aes_context_created = true;

        if (0 != crypto_aes_decrypt_update(
                     &gs_sp_bl2_aes_context,
                     &(sp_bl2_file_header->info.image_info_and_signaure.info.secret_info),
                     sizeof(sp_bl2_file_header->info.image_info_and_signaure.info.secret_info)))
        {
            printx("verify_bl2_image_file_header: crypto_aes_decrypt_update() failed!\n");
            return -1;
        }
    }

    if (gs_vaultip_disabled || gs_ignore_signatures)
    {
        MESSAGE_INFO("BL2 SIG IGN\n");
        return 0;
    }
    else
    {
        if (0 !=
            crypto_verify_pk_signature(
                &(sp_bl2_file_header->info.signing_certificate.certificate_info.subject_public_key),
                &(sp_bl2_file_header->info.image_info_and_signaure.info_signature),
                &(sp_bl2_file_header->info.image_info_and_signaure.info),
                sizeof(sp_bl2_file_header->info.image_info_and_signaure.info)))
        {
            printx("bl2_firmware signature is not valid!\n");
            return -1;
        }
        else
        {
            printx("bl2_firmware signature is OK!\n");
        }
    }

    return 0;
}

static int load_bl2_code_and_data(const ESPERANTO_IMAGE_FILE_HEADER_t *sp_bl2_file_header)
{
    uint32_t code_and_data_hash_size;
    uint32_t load_offset;
    union
    {
        uint64_t u64;
        struct
        {
            uint32_t lo;
            uint32_t hi;
        };
    } load_address;
    uint32_t region_no = 0;
    uint8_t hash[64];
    CRYPTO_HASH_CONTEXT_t hash_context;
    bool hash_context_initialized = false;
    CRYPTO_HASH_CONTEXT_t encrypted_hash_context;
    bool encrypted_hash_context_initialized = false;
    size_t total_length = 0;

    const ESPERANTO_IMAGE_INFO_t *image_info =
        &(sp_bl2_file_header->info.image_info_and_signaure.info);

    switch (image_info->public_info.code_and_data_hash_algorithm)
    {
        case HASH_ALG_SHA2_256:
            code_and_data_hash_size = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
            code_and_data_hash_size = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
            code_and_data_hash_size = 512 / 8;
            break;
        default:
            printx("load_bl2_code_and_data: Invalid hash algorithm!\n");
            return -1;
    }

    if (0 !=
        (sp_bl2_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED))
    {
        if (0 != crypto_hash_init(&encrypted_hash_context,
                                  image_info->public_info.code_and_data_hash_algorithm))
        {
            printx("load_bl2_code_and_data: crypto_hash_init(e) failed!\n");
            return -1;
        }
        encrypted_hash_context_initialized = true;
    }

    if (!gs_vaultip_disabled)
    {
        if (0 !=
            crypto_hash_init(&hash_context, image_info->public_info.code_and_data_hash_algorithm))
        {
            printx("load_bl2_code_and_data: crypto_hash_init() failed!\n");
            return -1;
        }
        hash_context_initialized = true;
    }

    for (region_no = 0; region_no < image_info->secret_info.load_regions_count; region_no++)
    {
        load_offset = (uint32_t)(sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) +
                                 image_info->secret_info.load_regions[region_no].region_offset);
        load_address.lo = image_info->secret_info.load_regions[region_no].load_address_lo;
        load_address.hi = image_info->secret_info.load_regions[region_no].load_address_hi;
        //printx("Region %u: load=0x%x, addr=0x%x, fsize=0x%x, msize=0x%x\n", region_no, load_offset,
        //       load_address.lo, image_info->secret_info.load_regions[region_no].load_size,
        //       image_info->secret_info.load_regions[region_no].memory_size);

        if (image_info->secret_info.load_regions[region_no].load_size > 0)
        {
            if (0 != flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_SP_BL2, load_offset,
                                        (void *)load_address.u64,
                                        image_info->secret_info.load_regions[region_no].load_size))
            {
                printx("load_bl2_code_and_data: flash_fs_read_file(code) failed!\n");
                goto CLEANUP_ON_ERROR;
            }
            //printx("loaded 0x%x bytes at 0x%08x\n",
            //       image_info->secret_info.load_regions[region_no].load_size, load_address.u64);

            if (!gs_vaultip_disabled)
            {
                if (0 != (sp_bl2_file_header->info.file_header_flags &
                          ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED))
                {
                    // hash encrypted data
                    if (0 != crypto_hash_update(
                                 &encrypted_hash_context, (void *)load_address.u64,
                                 image_info->secret_info.load_regions[region_no].load_size))
                    {
                        printx("load_bl2_code_and_data: crypto_hash_update() failed!\n");
                        goto CLEANUP_ON_ERROR;
                    }

                    // decrypt data
                    if (0 != crypto_aes_decrypt_update(
                                 &gs_sp_bl2_aes_context, (void *)load_address.u64,
                                 image_info->secret_info.load_regions[region_no].load_size))
                    {
                        printx("load_bl2_code_and_data: crypto_aes_decrypt_update() failed!\n");
                        goto CLEANUP_ON_ERROR;
                    }
                }

                if (0 !=
                    crypto_hash_update(&hash_context, (void *)load_address.u64,
                                       image_info->secret_info.load_regions[region_no].load_size))
                {
                    printx("load_bl2_code_and_data: crypto_hash_update() failed!\n");
                    goto CLEANUP_ON_ERROR;
                }
            }
            total_length = total_length + image_info->secret_info.load_regions[region_no].load_size;
        }

        if (image_info->secret_info.load_regions[region_no].memory_size >
            image_info->secret_info.load_regions[region_no].load_size)
        {
            memset((void *)(load_address.u64 +
                            image_info->secret_info.load_regions[region_no].load_size),
                   0,
                   image_info->secret_info.load_regions[region_no].memory_size -
                       image_info->secret_info.load_regions[region_no].load_size);
            //printx("erased 0x%x bytes\n",
            //       image_info->secret_info.load_regions[region_no].memory_size -
            //           image_info->secret_info.load_regions[region_no].load_size);
        }
    }

    if (0 !=
        (sp_bl2_file_header->info.file_header_flags & ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED))
    {
        if (0 != crypto_hash_final(&encrypted_hash_context, NULL, 0, total_length, hash))
        {
            printx("load_bl2_code_and_data: crypto_hash_final(p) failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        encrypted_hash_context_initialized = false;

        if (0 != constant_time_memory_compare(hash,
                                              sp_bl2_file_header->info.encrypted_code_and_data_hash,
                                              code_and_data_hash_size))
        {
            printx("load_bl2_code_and_data: encrypted code+data hash mismatch!\n");
            goto CLEANUP_ON_ERROR;
        }

        if (0 != crypto_aes_decrypt_final(&gs_sp_bl2_aes_context, NULL, 0, NULL))
        {
            printx("load_bl2_code_and_data: crypto_aes_decrypt_final() failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        gs_sp_bl2_aes_context_created = false;
    }

    if (gs_vaultip_disabled || gs_ignore_signatures)
    {
        MESSAGE_INFO("BL2 HASH IGN\n");
    }
    else
    {
        if (0 != crypto_hash_final(&hash_context, NULL, 0, total_length, hash))
        {
            printx("load_bl2_code_and_data: crypto_hash_final() failed!\n");
            goto CLEANUP_ON_ERROR;
        }
        hash_context_initialized = false;

        if (0 != constant_time_memory_compare(hash, image_info->public_info.code_and_data_hash,
                                              code_and_data_hash_size))
        {
            printx("load_bl2_code_and_data: code+data hash mismatch!\n");
            goto CLEANUP_ON_ERROR;
        }
    }

    return 0;

CLEANUP_ON_ERROR:
    for (uint32_t n = 0; n <= region_no; n++)
    {
        load_address.lo = image_info->secret_info.load_regions[n].load_address_lo;
        load_address.hi = image_info->secret_info.load_regions[n].load_address_hi;
        memset((void *)load_address.u64, 0, image_info->secret_info.load_regions[n].memory_size);
    }

    if (encrypted_hash_context_initialized && (0 != crypto_hash_abort(&encrypted_hash_context)))
    {
        printx("load_bl2_code_and_data: crypto_hash_abort(e) failed!\n");
    }
    if (hash_context_initialized && (0 != crypto_hash_abort(&hash_context)))
    {
        printx("load_bl2_code_and_data: crypto_hash_abort(p) failed!\n");
    }
    return -1;
}

int load_bl2_firmware(void)
{
    int rv;
    uint32_t sp_bl2_size;
    SERVICE_PROCESSOR_BL1_DATA_t *bl1_data = get_service_processor_bl1_data();

    gs_sp_bl2_kdk_created = false;
    gs_sp_bl2_mack_created = false;
    gs_sp_bl2_enck_created = false;
    gs_sp_bl2_aes_context_created = false;

    gs_vaultip_disabled = is_vaultip_disabled();
    gs_ignore_signatures = false;

    if (0 != sp_otp_get_signatures_check_chicken_bit(&gs_ignore_signatures))
    {
        gs_ignore_signatures = false;
    }

    // load the SP BL2 image
    if (0 != flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_SP_BL2, &sp_bl2_size))
    {
        printx("load_bl2_firmware: flash_fs_get_file_size(header) failed!\n");
        return -1;
    }
    if (sp_bl2_size < sizeof(ESPERANTO_IMAGE_FILE_HEADER_t))
    {
        printx("load_bl2_firmware: SP_BL2 image file too small!");
        return -1;
    }
    if (0 != flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_SP_BL2, 0, &(bl1_data->sp_bl2_header),
                                sizeof(bl1_data->sp_bl2_header)))
    {
        printx("load_bl2_firmware: flash_fs_read_file(SP_BL2) failed!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != verify_bl2_image_file_header(&bl1_data->sp_bl2_header, sp_bl2_size))
    {
        printx("load_bl2_firmware: verify_bl2_image_file_header() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != load_bl2_code_and_data(&bl1_data->sp_bl2_header))
    {
        printx("load_bl2_firmware: load_bl2_code_and_data() failed!\n");
        rv = -1;
        goto DONE;
    }

    printx("load_bl2_firmware: Loaded SP_BL2 firmware.\n");
    rv = 0;

DONE:
    if (gs_sp_bl2_aes_context_created &&
        (0 != crypto_aes_decrypt_final(&gs_sp_bl2_aes_context, NULL, 0, NULL)))
    {
        printx("load_bl2_firmware: crypto_aes_decrypt_final() failed!\n");
    }
    if (gs_sp_bl2_enck_created && (0 != crypto_delete_key(gs_sp_bl2_enck)))
    {
        printx("load_bl2_firmware: crypto_delete_key(enck) failed!\n");
        rv = -1;
    }
    if (gs_sp_bl2_mack_created && (0 != crypto_delete_key(gs_sp_bl2_mack)))
    {
        printx("load_bl2_firmware: crypto_delete_key(mack) failed!\n");
        rv = -1;
    }
    if (gs_sp_bl2_kdk_created && (0 != crypto_delete_key(gs_sp_bl2_kdk)))
    {
        printx("load_bl2_firmware: crypto_delete_key(kdk) failed!\n");
        rv = -1;
    }

    if (0 != rv)
    {
        memset(&(bl1_data->sp_bl2_header), 0, sizeof(bl1_data->sp_bl2_header));
        memset(&(gs_sp_bl2_IV), 0, sizeof(gs_sp_bl2_IV));
    }

    return rv;
}
